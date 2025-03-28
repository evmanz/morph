#pragma once
#include "config.h"
#include "client.h"
#include <google/cloud/storage/client.h>
#include <mutex>
#include <unordered_map>
#include <list>
#include "ram_tracker.h"

class GCSCache {
public:
    explicit GCSCache(const Config& config);

    RamTracker& ram_tracker();
    std::shared_ptr<Client> get_client(const QosConfig& qos);

    std::string get_or_fetch_object(const std::string& file, const std::string& bucket);

private:
    Config config_;
    RamTracker ram_tracker_;
    
    google::cloud::storage::Client storage_client_;

    std::mutex cache_mutex_;
    size_t current_cache_size_;
    std::unordered_map<std::string, std::list<std::string>::iterator> cache_index_;
    std::list<std::string> lru_keys_;

    void evict_if_needed(size_t incoming_file_size);
    bool is_cached(const std::string& key);
};

