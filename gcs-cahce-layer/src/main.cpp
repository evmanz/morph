#include "cli_utils.h"
#include "server.h"
#include <memory>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        CliConfig cli_config = parse_cli(argc, argv);
        auto cache = std::make_shared<GCSCache>(cli_config.config);

        CacheServer server(cache, cli_config.qos, 8080); // Port could be made configurable
        std::cout << "Starting cache server on port 8080...\n";
        server.run();

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
