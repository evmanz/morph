#pragma once
#include <string>

class HttpClient {
public:
    static bool download_file(const std::string& url, const std::string& dest_path);
};