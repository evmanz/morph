#include "client.h"
#include "gcs_cache.h"
#include <thread>

Client::Client(GCSCache* cache, const QosConfig& qos)
    : cache_(cache), qos_(qos), concurrent_requests_(0) {}

void Client::wait_for_slot() {
    std::unique_lock<std::mutex> lock(qos_mutex_);
    while (concurrent_requests_ >= qos_.max_concurrent_requests) {
        lock.unlock();
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        lock.lock();
    }
    ++concurrent_requests_;
}

std::string Client::get_object(const std::string& key) {
    wait_for_slot();
    auto path = cache_->get_or_fetch_object(key);
    {
        std::lock_guard<std::mutex> lock(qos_mutex_);
        --concurrent_requests_;
    }
    return path;
}

