#include "gcs_cache.h"
#include <filesystem>
#include <fstream>

namespace gcs = google::cloud::storage;

GCSCache::GCSCache(const Config& config)
    : config_(config), current_cache_size_(0), ram_tracker_(config.max_ram_usage),
    storage_client_(gcs::Client(
        gcs::ClientOptions::CreateDefaultClientOptions().value()
            .set_endpoint(config.gcs_endpoint)
            .set_credentials(gcs::oauth2::CreateAnonymousCredentials()))) {
    std::filesystem::create_directories(config.cache_dir);
}

std::shared_ptr<Client> GCSCache::get_client(const QosConfig& qos) {
    return std::make_shared<Client>(this, qos);
}

std::string GCSCache::get_or_fetch_object(const std::string& file, const std::string& bucket) {
    std::lock_guard<std::mutex> lock(cache_mutex_);

    // Build path: <cache_dir>/<bucket>/<file>
    std::string bucket_dir = config_.cache_dir + "/" + bucket;
    std::string local_path = bucket_dir + "/" + file;
    std::string cache_key = bucket + "/" + file;

    LOG(INFO) << "[CACHE] Fetching Object: " << file << " form bucket: " << bucket << "\n";

    if (is_cached(cache_key)) {
        LOG(INFO) << "[CACHE] HIT: " << cache_key << "\n";
        lru_keys_.erase(cache_index_[cache_key]);
        lru_keys_.push_front(cache_key);
        cache_index_[cache_key] = lru_keys_.begin();
        return local_path;
    }

    LOG(INFO) << "[CACHE] MISS: " << cache_key << ", reading from remote store\n";
    auto reader = storage_client_.ReadObject(bucket, file);
    if (!reader) {
        LOG(ERROR) << "ReadObject error: " << file << " is not in bucket " << bucket << "\n";
        throw std::runtime_error(reader.status().message());
    }

    // Ensure bucket subdirectory exists
    std::filesystem::create_directories(bucket_dir);

    // Write to temp file first
    std::string temp_path = local_path + ".tmp";
    std::ofstream ofs(temp_path, std::ios::binary);
    ofs << reader.rdbuf();
    ofs.close();

    // Get file size and apply eviction policy
    size_t file_size = std::filesystem::file_size(temp_path);
    evict_if_needed(file_size);

    // Finalize file
    std::filesystem::rename(temp_path, local_path);

    // Update cache metadata
    lru_keys_.push_front(cache_key);
    cache_index_[cache_key] = lru_keys_.begin();
    current_cache_size_ += file_size;

    return local_path;
}

void GCSCache::evict_if_needed(size_t incoming_file_size) {
    while (current_cache_size_ + incoming_file_size > config_.max_disk_cache_size && !lru_keys_.empty()) {
        LOG(INFO) << "[CACHE] EVICTING, not enough disk space: " << current_cache_size_ << " + " << incoming_file_size << " > " << config_.max_disk_cache_size << "\n";
        std::string lru_key = lru_keys_.back();
        std::string file_path = config_.cache_dir + "/" + lru_key;
        size_t size_removed = std::filesystem::file_size(file_path);
        std::filesystem::remove(file_path);
        LOG(INFO) << "[CACHE] EVICTED: " << file_path << " (" << size_removed << " bytes)\n";
        lru_keys_.pop_back();
        cache_index_.erase(lru_key);
        current_cache_size_ -= size_removed;
    }
}

bool GCSCache::is_cached(const std::string& key) {
    return cache_index_.find(key) != cache_index_.end() && std::filesystem::exists(config_.cache_dir + "/" + key);
}

RamTracker& GCSCache::ram_tracker() {
    return ram_tracker_;
}