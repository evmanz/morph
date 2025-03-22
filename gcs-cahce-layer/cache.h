#pragma once
#include "config.h"
#include <mutex>
#include <unordered_map>
#include <list>

class GCSCache {
public:
    explicit GCSCache(const Config& config);
    std::string get_or_fetch_object(const std::string& key);

private:
    Config config_;
    std::mutex cache_mutex_;
    size_t current_cache_size_;
    std::unordered_map<std::string, std::list<std::string>::iterator> cache_index_;
    std::list<std::string> lru_keys_;
    void evict_if_needed(size_t incoming_file_size);
    bool is_cached(const std::string& key);
    std::string fetch_from_gcs(const std::string& key);
};