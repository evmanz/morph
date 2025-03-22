#include "gcs_cache.h"
#include <filesystem>
#include <fstream>

namespace gcs = google::cloud::storage;

GCSCache::GCSCache(const Config& config)
    : config_(config), current_cache_size_(0),
    storage_client_(gcs::Client(
        gcs::ClientOptions::CreateDefaultClientOptions().value()
            .set_endpoint(config.gcs_endpoint)
            .set_credentials(gcs::oauth2::CreateAnonymousCredentials()))) {
    std::filesystem::create_directories(config.cache_dir);
}

std::shared_ptr<Client> GCSCache::get_client(const QosConfig& qos) {
    return std::make_shared<Client>(this, qos);
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

    auto reader = storage_client_.ReadObject(config_.bucket_name, key);
    if (!reader) throw std::runtime_error(reader.status().message());

    std::string temp_path = local_path + ".tmp";
    std::ofstream ofs(temp_path, std::ios::binary);
    ofs << reader.rdbuf();
    ofs.close();

    size_t file_size = std::filesystem::file_size(temp_path);
    evict_if_needed(file_size);
    std::filesystem::rename(temp_path, local_path);

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

