#pragma once
#include "gcs_cache.h"
#include <memory>
#include <string>
#include <unordered_map>
#include <mutex>
#include "httplib.h"
#include "constants.h"
#include <glog/logging.h>

struct ClientState {
    std::mutex mutex;
    size_t active_requests = 0;
    size_t bytes_sent_this_sec = 0;
    std::chrono::steady_clock::time_point last_reset = std::chrono::steady_clock::now();
};

class CacheServer {
public:
    CacheServer(std::shared_ptr<GCSCache> cache, const QosConfig& qos_config, int port);
    void run();

private:
    std::shared_ptr<GCSCache> cache_;
    QosConfig qos_config_;
    int port_;

    std::mutex state_mutex_;
    std::unordered_map<std::string, ClientState> client_states_;

    bool get_client_id(const httplib::Request& req, httplib::Response& res, std::string& client_id_out);
    bool check_concurrency_limit(const std::string& client_id, httplib::Response& res);
    bool check_bandwidth_limit(const std::string& client_id, size_t bytes);
    void stream_file_to_client(const std::string& client_id, const std::string& file_path, httplib::Response& res);
    void lock_and_initialize_client(const std::string& id);
    void decrement_concurrency(const std::string& id);
};
