#include "server.h"
#include "httplib.h"
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <chrono>

CacheServer::CacheServer(std::shared_ptr<GCSCache> cache, const QosConfig& qos_config, int port)
    : cache_(cache), qos_config_(qos_config), port_(port) {}

    
void CacheServer::run() {
    httplib::Server svr;

    svr.Get("/object", [&](const httplib::Request& req, httplib::Response& res) {
        auto parsed = parse_request_params(req, res);
        if (!parsed) return;
    
        auto [bucket, file, client_id] = *parsed;
    
        lock_and_initialize_client(client_id);
        if (!check_concurrency_limit(client_id, res)) return;
    
        try {
            auto client = cache_->get_client(qos_config_);
            auto file_path = client->get_object(file, bucket);
            stream_file_to_client(client_id, file_path, res);
        } catch (const std::exception& e) {
            res.status = HttpStatus::InternalServerError;
            res.set_content(e.what(), "text/plain");
        }
        decrement_concurrency(client_id);
    });

    svr.listen("0.0.0.0", port_);
}

bool CacheServer::check_concurrency_limit(const std::string& client_id, httplib::Response& res) {
    auto& state = client_states_[client_id];
    std::lock_guard<std::mutex> lock(state.mutex);

    if (state.active_requests >= qos_config_.max_concurrent_requests) {
        res.status = HttpStatus::TooManyRequests;
        res.set_content("Too Many Requests", "text/plain");
        LOG(WARNING) << "[REQ] HttpStatus::TooManyRequests for client_id=" << client_id << "\n";
        return false;
    }

    state.active_requests++;
    LOG(INFO) << "[REQ] client_id=" << client_id << ", active_requests=" << state.active_requests << "\n";
    return true;
}

bool CacheServer::check_bandwidth_limit(const std::string& client_id, size_t bytes) {
    auto& state = client_states_[client_id];
    std::lock_guard<std::mutex> lock(state.mutex);

    auto now = std::chrono::steady_clock::now();
    if (now - state.last_reset >= std::chrono::seconds(1)) {
        state.bytes_sent_this_sec = 0;
        state.last_reset = now;
    }

    if (state.bytes_sent_this_sec + bytes > qos_config_.max_bandwidth_bps) {
        LOG(WARNING) << "[REQ] client_id=" << client_id << ", bandwidth limit reached\n";
        return false;
    }

    state.bytes_sent_this_sec += bytes;
    // too verbose
    // LOG(INFO) << "[REQ] client_id=" << client_id << ", bytes_sent_this_sec=" << state.bytes_sent_this_sec << "\n";
    return true;
}

void CacheServer::stream_file_to_client(const std::string& client_id,
                                        const std::string& file_path,
                                        httplib::Response& res) {
    auto ifs = std::make_shared<std::ifstream>(file_path, std::ios::binary);
    if (!ifs || !ifs->is_open()) {
        LOG(ERROR) << "Failed to open file: " << file_path << "\n";
        throw std::runtime_error("Failed to open file"); // perhaps not the best for production? 
    }
    LOG(INFO) << "[REQ] Streaming file: " << file_path << " to client_id=" << client_id << "\n";
    res.set_chunked_content_provider("application/octet-stream",
        [this, client_id, ifs](size_t offset, httplib::DataSink& sink) mutable {
            cache_->ram_tracker().wait_and_reserve(Constants::StreamBufferSize);
            char buffer[Constants::StreamBufferSize];
            ifs->read(buffer, sizeof(buffer));
            size_t bytes_read = ifs->gcount();

            if (bytes_read > 0) {
                while (!check_bandwidth_limit(client_id, bytes_read)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(Constants::BandwidthRetryDelay));
                }
                sink.write(buffer, bytes_read);
            }

            if (ifs->eof()) {
                sink.done();
                decrement_concurrency(client_id);
            }
            cache_->ram_tracker().release(Constants::StreamBufferSize);
            return true;
        });
    LOG(INFO) << "[REQ] HttpStatus::Ok for client_id=" << client_id << "\n";
}

void CacheServer::lock_and_initialize_client(const std::string& id) {
    std::lock_guard<std::mutex> lock(state_mutex_);
    if (client_states_.find(id) == client_states_.end()) {
        client_states_.emplace(std::piecewise_construct,
                               std::forward_as_tuple(id),
                               std::forward_as_tuple());
    }
}

void CacheServer::decrement_concurrency(const std::string& id) {
    auto& state = client_states_[id];
    std::lock_guard<std::mutex> lock(state.mutex);
    if (state.active_requests > 0) state.active_requests--;
    LOG(INFO) << "[REQ] client_id=" << id << ", active_requests=" << state.active_requests << "\n";
}

std::optional<req_params>
CacheServer::parse_request_params(const httplib::Request& req, httplib::Response& res) {
    if (!req.has_param("bucket") || !req.has_param("file") || !req.has_param("client_id")) {
        LOG(WARNING) << "Missing one or more required query parameters: 'bucket', 'file', 'client_id'\n";
        res.status = HttpStatus::BadRequest;
        res.set_content("Missing one or more required query parameters: 'bucket', 'file', 'client_id'", "text/plain");
        return std::nullopt;
    }

    std::string bucket = req.get_param_value("bucket");
    std::string file = req.get_param_value("file");
    std::string client_id = req.get_param_value("client_id");

    return std::make_tuple(bucket, file, client_id);
}
