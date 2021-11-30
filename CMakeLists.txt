cmake_minimum_required(VERSION 3.5)

project(sio LANGUAGES CXX VERSION 0.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# add third-party dependencies
add_subdirectory(deps/abseil-cpp)

# sio
include_directories(include src)

add_subdirectory(src)

add_executable(stt stt.cc)
target_link_libraries(stt absl::strings absl::str_format absl::type_traits absl::flat_hash_map)
