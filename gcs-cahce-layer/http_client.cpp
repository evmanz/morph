#include "http_client.h"
#include <curl/curl.h>
#include <fstream>
#include <sstream>
#include <filesystem>

struct DownloadContext {
    std::ofstream ofs;
    bool success = true;
};

// Callback to handle data
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    auto *context = static_cast<DownloadContext*>(userp);
    if (!context->ofs.is_open()) {
        context->success = false;
        return 0;
    }
    context->ofs.write((char*)contents, size * nmemb);
    return size * nmemb;
}

bool HttpClient::download_file(const std::string& url, const std::string& dest_path) {
    CURL *curl = curl_easy_init();
    if (!curl) return false;

    DownloadContext context;
    context.ofs.open(dest_path, std::ios::binary);
    if (!context.ofs.is_open()) {
        curl_easy_cleanup(curl);
        return false;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &context);

    CURLcode res = curl_easy_perform(curl);

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    curl_easy_cleanup(curl);
    context.ofs.close();

    // Check HTTP status
    if (res != CURLE_OK || http_code != 200 || !context.success) {
        std::filesystem::remove(dest_path); // cleanup invalid file
        return false;
    }

    return true;
}
