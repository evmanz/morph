#pragma once
#include "config.h"
#include <string>

struct CliConfig {
    Config config;
    QosConfig qos;
    std::string file_name;
};

CliConfig parse_cli(int argc, char* argv[]);

