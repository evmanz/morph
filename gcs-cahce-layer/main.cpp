#include "google/cloud/storage/client.h"
#include <iostream>

namespace gcs = google::cloud::storage;

int main() {
    // Configure the SDK to connect to the fake-gcs-server
    auto options = gcs::ClientOptions::CreateDefaultClientOptions().value();

    // Point explicitly at your fake-gcs-server
    options.set_endpoint("http://localhost:4443"); 
    options.set_credentials(google::cloud::MakeInsecureCredentials());

    auto client = gcs::Client(options);

    std::string bucket_name = "my_bucket";
    std::string object_name = "test_object.txt";

    // Write an object
    auto writer = client.WriteObject(bucket_name, object_name);
    writer << "Hello from Google Cloud SDK + fake-gcs-server!";
    writer.Close();

    if (!writer.metadata()) {
        std::cerr << "Error writing object: " << writer.metadata().status() << "\n";
        return 1;
    }
    std::cout << "Object written successfully!\n";

    // Read the object back
    auto reader = client.ReadObject(bucket_name, object_name);
    if (!reader) {
        std::cerr << "Error reading object: " << reader.status() << "\n";
        return 1;
    }

    std::string contents{std::istreambuf_iterator<char>{reader}, {}};
    std::cout << "Object content: " << contents << "\n";

    return 0;
}
