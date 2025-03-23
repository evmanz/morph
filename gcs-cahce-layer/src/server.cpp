#include "server.h"
#include "httplib.h"
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <chrono>

struct ClientState {
    std::mutex mutex;
    size_t active_requests = 0;
    size_t bytes_sent_this_sec = 0;
    std::chrono::steady_clock::time_point last_reset = std::chrono::steady_clock::now();
};

CacheServer::CacheServer(std::shared_ptr<GCSCache> cache, const QosConfig& qos_config, int port)
    : cache_(cache), qos_config_(qos_config), port_(port) {}

void CacheServer::run() {
    httplib::Server svr;
    std::mutex global_mutex;
    std::unordered_map<std::string, ClientState> client_states;

    svr.Get(R"(/object/(\S+))", [&](const httplib::Request& req, httplib::Response& res) {
        auto client_ip = req.remote_addr;

        {
            std::lock_guard<std::mutex> lock(global_mutex);
            if (client_states.find(client_ip) == client_states.end()) {
                client_states.emplace(std::piecewise_construct, 
                      std::forward_as_tuple(client_ip), 
                      std::forward_as_tuple());
            }
        }

        ClientState& state = client_states[client_ip];

        {
            std::lock_guard<std::mutex> lock(state.mutex);
            auto now = std::chrono::steady_clock::now();
            if (now - state.last_reset >= std::chrono::seconds(1)) {
                state.bytes_sent_this_sec = 0;
                state.last_reset = now;
            }

            if (state.active_requests >= qos_config_.max_concurrent_requests) {
                res.status = 429;
                res.set_content("Too Many Requests", "text/plain");
                return;
            }
            state.active_requests++;
        }

        auto client = cache_->get_client(qos_config_);
        std::string key = req.matches[1];

        try {
            auto file_path = client->get_object(key);

            std::ifstream ifs(file_path, std::ios::binary);
            if (!ifs) throw std::runtime_error("Failed to open file.");

            res.set_chunked_content_provider(
                "application/octet-stream",
                [&, client_ip, ifs = std::make_shared<std::ifstream>(std::move(ifs))](size_t offset, httplib::DataSink &sink) mutable {
                    char buffer[64 * 1024];
                    ifs->read(buffer, sizeof(buffer));
                    auto bytes_read = ifs->gcount();
            
                    ClientState& state = client_states[client_ip];
                    {
                        std::lock_guard<std::mutex> lock(state.mutex);
                        auto now = std::chrono::steady_clock::now();
                        if (now - state.last_reset >= std::chrono::seconds(1)) {
                            state.bytes_sent_this_sec = 0;
                            state.last_reset = now;
                        }
            
                        if (state.bytes_sent_this_sec + bytes_read > qos_config_.max_bandwidth_bps) {
                            std::this_thread::sleep_for(std::chrono::milliseconds(100));
                            return true;
                        }
                        state.bytes_sent_this_sec += bytes_read;
                    }
            
                    if (bytes_read > 0) {
                        sink.write(buffer, bytes_read);
                    }
            
                    if (ifs->eof()) {
                        sink.done();
                        std::lock_guard<std::mutex> lock(state.mutex);
                        state.active_requests--;
                    }
            
                    return true;
                });

        } catch (const std::exception& e) {
            std::lock_guard<std::mutex> lock(state.mutex);
            state.active_requests--;
            res.status = 500;
            res.set_content(e.what(), "text/plain");
        }
    });

    svr.listen("0.0.0.0", port_);
}

