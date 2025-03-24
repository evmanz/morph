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

    svr.Get(R"(/object/(\S+))", [&](const httplib::Request& req, httplib::Response& res) {
        std::string client_id;
        if (!get_client_id(req, res, client_id)) return;

        lock_and_initialize_client(client_id);
        if (!check_concurrency_limit(client_id, res)) return;

        auto client = cache_->get_client(qos_config_);
        std::string key = req.matches[1];

        try {
            auto file_path = client->get_object(key);
            stream_file_to_client(client_id, file_path, res);
        } catch (const std::exception& e) {
            decrement_concurrency(client_id);
            res.status = HttpStatus::InternalServerError;
            res.set_content(e.what(), "text/plain");
        }
    });

    svr.listen("0.0.0.0", port_);
}

bool CacheServer::check_concurrency_limit(const std::string& client_id, httplib::Response& res) {
    auto& state = client_states_[client_id];
    std::lock_guard<std::mutex> lock(state.mutex);

    if (state.active_requests >= qos_config_.max_concurrent_requests) {
        res.status = HttpStatus::TooManyRequests;
        res.set_content("Too Many Requests", "text/plain");
        return false;
    }

    state.active_requests++;
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
        return false;
    }

    state.bytes_sent_this_sec += bytes;
    return true;
}

void CacheServer::stream_file_to_client(const std::string& client_id,
                                        const std::string& file_path,
                                        httplib::Response& res) {
    auto ifs = std::make_shared<std::ifstream>(file_path, std::ios::binary);
    if (!ifs || !ifs->is_open()) throw std::runtime_error("Failed to open file");

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
}

bool CacheServer::get_client_id(const httplib::Request& req, httplib::Response& res, std::string& client_id_out) {
    if (req.has_param("client_id")) {
        client_id_out = req.get_param_value("client_id");
        return true;
    } else {
        res.status = HttpStatus::BadRequest;
        res.set_content("Missing 'client_id' query parameter", "text/plain");
        return false;
    }
}
