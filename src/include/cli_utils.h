#pragma once
#include "config.h"
#include <string>

struct CliConfig {
    Config config;
    QosConfig qos;
};

CliConfig parse_cli(int argc, char* argv[]);
