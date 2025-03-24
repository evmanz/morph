#pragma once
#include <mutex>
#include <condition_variable>
#include <glog/logging.h>

class RamTracker {
public:
    explicit RamTracker(size_t max_bytes);

    // Try to reserve memory. Returns false if not enough available.
    bool try_reserve(size_t bytes);

    // Blocks until memory is available, then reserves.
    void wait_and_reserve(size_t bytes);

    // Releases memory back to the pool.
    void release(size_t bytes);

private:
    const size_t max_bytes_;
    size_t used_bytes_;

    mutable std::mutex mutex_;
    std::condition_variable cond_;
};
