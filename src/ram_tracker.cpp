#include "ram_tracker.h"

RamTracker::RamTracker(size_t max_bytes)
    : max_bytes_(max_bytes), used_bytes_(0) {}

void RamTracker::wait_and_reserve(size_t bytes) {
    std::unique_lock<std::mutex> lock(mutex_);
    cond_.wait(lock, [&] {
        return used_bytes_ + bytes <= max_bytes_;
    });
    used_bytes_ += bytes;
    // too verbose
    // LOG(INFO) << "Reserved " << bytes << " bytes, total in use: " << used_bytes_ << "\n";
}

void RamTracker::release(size_t bytes) {
    {
        std::lock_guard<std::mutex> lock(mutex_);
        used_bytes_ -= bytes;
    }
    cond_.notify_one();
    // too verbose
    // LOG(INFO) << "Released " << bytes << " bytes, total in use: " << used_bytes_ << "\n";
}
