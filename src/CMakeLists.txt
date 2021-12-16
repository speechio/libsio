# libsio
add_library(sio SHARED
  error.cc
)

# libsio unittest
add_executable(unittest 
  error_test.cc
  log_test.cc
  check_test.cc
  linked_list_test.cc
  heap_test.cc
  json_test.cc
  audio_test.cc
  feature_test.cc
  dbg_test.cc
)

target_link_libraries(unittest
  gtest_main
  sio
  absl::base
  absl::strings
  ${KALDI_LIBS}
)

include(GoogleTest)
gtest_discover_tests(unittest)
