#include "cache.h"
#include "http_client.h"
#include <filesystem>

GCSCache::GCSCache(const Config& config)
    : config_(config), current_cache_size_(0) {
    std::filesystem::create_directories(config.cache_dir);
}

std::string GCSCache::get_or_fetch_object(const std::string& key) {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    std::string local_path = config_.cache_dir + "/" + key;

    if (is_cached(key)) {
        lru_keys_.erase(cache_index_[key]);
        lru_keys_.push_front(key);
        cache_index_[key] = lru_keys_.begin();
        return local_path;
    }

    std::string fetched_file_path = fetch_from_gcs(key);
    if (fetched_file_path.empty()) return "";

    size_t file_size = std::filesystem::file_size(fetched_file_path);
    evict_if_needed(file_size);

    // Rename (now safe, same filesystem)
    std::filesystem::rename(fetched_file_path, local_path);

    lru_keys_.push_front(key);
    cache_index_[key] = lru_keys_.begin();
    current_cache_size_ += file_size;

    return local_path;
}

void GCSCache::evict_if_needed(size_t incoming_file_size) {
    while (current_cache_size_ + incoming_file_size > config_.max_disk_cache_size && !lru_keys_.empty()) {
        std::string lru_key = lru_keys_.back();
        std::string file_path = config_.cache_dir + "/" + lru_key;
        size_t size_removed = std::filesystem::file_size(file_path);
        std::filesystem::remove(file_path);
        lru_keys_.pop_back();
        cache_index_.erase(lru_key);
        current_cache_size_ -= size_removed;
    }
}

bool GCSCache::is_cached(const std::string& key) {
    return cache_index_.find(key) != cache_index_.end() && std::filesystem::exists(config_.cache_dir + "/" + key);
}

std::string GCSCache::fetch_from_gcs(const std::string& key) {
    std::string url = config_.gcs_server_endpoint + "/storage/v1/b/" + config_.bucket_name +
                      "/o/" + key + "?alt=media";
    
    // Temporary path is now within cache directory:
    std::string temp_file = config_.cache_dir + "/" + key + ".tmp";

    if(HttpClient::download_file(url, temp_file)) {
        return temp_file;
    }

    // Cleanup on failure
    if (std::filesystem::exists(temp_file))
        std::filesystem::remove(temp_file);

    return "";
}