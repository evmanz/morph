#include "ram_tracker.h"

RamTracker::RamTracker(size_t max_bytes)
    : max_bytes_(max_bytes), used_bytes_(0) {}

bool RamTracker::try_reserve(size_t bytes) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (used_bytes_ + bytes <= max_bytes_) {
        used_bytes_ += bytes;
        return true;
    }
    return false;
}

void RamTracker::wait_and_reserve(size_t bytes) {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [&] {
        return used_bytes_ + bytes <= max_bytes_;
    });
    used_bytes_ += bytes;
}

void RamTracker::release(size_t bytes) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        used_bytes_ -= bytes;
    }
    cond_.notify_one();
}

size_t RamTracker::current_usage() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return used_bytes_;
}

