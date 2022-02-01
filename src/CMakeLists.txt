# libsio
add_library(sio SHARED
    error.cc
)

# libsio unittest
add_executable(unittest 
    error_test.cc
    logger_test.cc
    check_test.cc
    linked_list_test.cc
    allocator_test.cc
    audio_test.cc
    feature_extractor_test.cc
    dbg_test.cc
    struct_loader_test.cc
    fsm_test.cc
    language_model_test.cc
)

target_link_libraries(unittest
    gtest_main
    sio
    ${TORCH_LIBRARIES}
    ${KALDI_LIBRARIES}
    ${ABSL_LIBRARIES}
)

include(GoogleTest)
gtest_discover_tests(unittest)
