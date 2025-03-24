#include "cli_utils.h"
#include "cxxopts.hpp"
#include "ini.h"
#include <iostream>

struct IniConfig {
    std::string endpoint = "http://localhost:4443";
    std::string bucket = "default_bucket";
    std::string cache_dir = "./cache";
    size_t max_disk = 500;
    size_t max_ram = 64;
    size_t max_concurrency = 2;
    size_t max_bandwidth = 10;
};

static int ini_cfg_reader(void* user, const char* section, const char* name, const char* value) {
    auto* config = reinterpret_cast<IniConfig*>(user);
    std::string sec(section), key(name);

    if (sec == "gcs") {
        if (key == "endpoint") config->endpoint = value;
        else if (key == "bucket") config->bucket = value;
    } else if (sec == "cache") {
        if (key == "cache_dir") config->cache_dir = value;
        else if (key == "max_disk") config->max_disk = std::stoul(value);
        else if (key == "max_ram") config->max_ram = std::stoul(value);
    } else if (sec == "qos") {
        if (key == "max_concurrency") config->max_concurrency = std::stoul(value);
        else if (key == "max_bandwidth") config->max_bandwidth = std::stoul(value);
    }
    return 1;
}

CliConfig parse_cli(int argc, char* argv[]) {
    cxxopts::Options options("gcs_cache", "GCS Cache Client with QoS");
    options.add_options()
        ("cfg", "INI config file", cxxopts::value<std::string>())
        ("endpoint", "GCS endpoint URL", cxxopts::value<std::string>())
        ("help", "Print usage");

    auto result = options.parse(argc, argv);

    if (result.count("help")) {
        std::cout << options.help() << "\n";
        exit(1);
    }

    IniConfig ini_cfg;
    if (result.count("cfg")) {
        std::string cfg_file = result["cfg"].as<std::string>();
        if (ini_parse(cfg_file.c_str(), ini_cfg_reader, &ini_cfg) < 0) {
            std::cerr << "Failed to parse config file: " << cfg_file << "\n";
            exit(1);
        }
    }

    if (result.count("endpoint"))
        ini_cfg.endpoint = result["endpoint"].as<std::string>();

    CliConfig cli_config{
        .config = {
            .max_disk_cache_size = ini_cfg.max_disk * 1024 * 1024,
            .max_ram_usage = ini_cfg.max_ram * 1024 * 1024,
            .cache_dir = ini_cfg.cache_dir,
            .gcs_endpoint = ini_cfg.endpoint,
            .bucket_name = ini_cfg.bucket
        },
        .qos = {
            .max_concurrent_requests = ini_cfg.max_concurrency,
            .max_bandwidth_bps = ini_cfg.max_bandwidth * 1024 * 1024
        }
    };

    return cli_config;
}

