#pragma once
#include <string>

struct QosConfig {
    size_t max_concurrent_requests;
    size_t max_bandwidth_bps;
};

struct Config {
    size_t max_disk_cache_size;
    size_t max_ram_usage;
    std::string cache_dir;
    std::string gcs_server_endpoint;
    std::string bucket_name;
    std::string auth_token;
};