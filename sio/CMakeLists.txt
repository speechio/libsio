add_library(sio STATIC
  link.cc
)

# UNITTEST
enable_testing()

add_executable(unittest 
  log_test.cc
  link_test.cc
)
target_link_libraries(unittest
  gtest_main
  sio
  absl::base
)

include(GoogleTest)
gtest_discover_tests(unittest)
