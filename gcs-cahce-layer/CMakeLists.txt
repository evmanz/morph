cmake_minimum_required(VERSION 3.10)
project(gcs_caching_library)

set(CMAKE_CXX_STANDARD 17)

find_package(google_cloud_cpp_storage REQUIRED)

# Find glog via pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(GLOG REQUIRED libglog)

file(GLOB_RECURSE SRC_FILES "src/*.cpp" "src/*.c")

add_executable(gcs_cache ${SRC_FILES})

target_include_directories(gcs_cache PRIVATE src/include src ${GLOG_INCLUDE_DIRS})
target_link_libraries(gcs_cache google-cloud-cpp::storage stdc++fs pthread ${GLOG_LIBRARIES})
