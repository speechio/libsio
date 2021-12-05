cmake_minimum_required(VERSION 3.5)

project(sio LANGUAGES CXX VERSION 0.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

## ADD THIRD-PARTY DEPENDENCIES
# gtest
include(FetchContent)
FetchContent_Declare(
  gtest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(gtest)

# abseil
add_subdirectory(deps/abseil-cpp)

# LIBRARY
include_directories(.)
add_subdirectory(sio)

# BINARY
add_executable(stt stt.cc)
target_link_libraries(stt sio absl::strings absl::str_format absl::type_traits absl::flat_hash_map)
