#pragma once
#include "gcs_cache.h"
#include <memory>

class CacheServer {
public:
    CacheServer(std::shared_ptr<GCSCache> cache, const QosConfig& qos_config, int port);
    void run();

private:
    std::shared_ptr<GCSCache> cache_;
    QosConfig qos_config_;
    int port_;
};
