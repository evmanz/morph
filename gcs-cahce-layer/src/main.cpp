#include "cli_utils.h"
#include "gcs_cache.h"
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        CliConfig cli_config = parse_cli(argc, argv);

        GCSCache cache(cli_config.config);
        auto client = cache.get_client(cli_config.qos);

        auto file_path = client->get_object(cli_config.file_name);
        std::cout << "File cached at: " << file_path << "\n";
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}
