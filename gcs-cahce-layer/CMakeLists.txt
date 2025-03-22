cmake_minimum_required(VERSION 3.10)
project(gcs_sdk_example)

set(CMAKE_CXX_STANDARD 17)

find_package(google_cloud_cpp_storage REQUIRED)

add_executable(gcs_sdk_example main.cpp)
target_link_libraries(gcs_sdk_example
    google-cloud-cpp::storage
)
