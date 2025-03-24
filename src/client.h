#pragma once
#include "config.h"
#include <mutex>

class GCSCache;

class Client {
public:
    Client(GCSCache* cache, const QosConfig& qos);
    std::string get_object(const std::string& file, const std::string& bucket);

private:
    GCSCache* cache_;
    QosConfig qos_;
    std::mutex qos_mutex_;
    size_t concurrent_requests_;

    void wait_for_slot();
};

