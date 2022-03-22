cmake_minimum_required(VERSION 3.14)

project(sio LANGUAGES CXX VERSION 0.1)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# GTest
include(FetchContent)
FetchContent_Declare(
    gtest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(gtest)


# Abseil (libabsl)
include_directories(${CMAKE_SOURCE_DIR}/deps/abseil-cpp)
add_subdirectory(${CMAKE_SOURCE_DIR}/deps/abseil-cpp)
set(ABSL_LIBRARIES
    absl::base
    absl::memory
    absl::strings
    absl::str_format
    absl::type_traits
    absl::flat_hash_map
)


# Kaldi (libkaldi)
set(KALDI_CMAKE_DIST "${CMAKE_SOURCE_DIR}/deps/kaldi_dist")
include_directories(${KALDI_CMAKE_DIST}/include)       # OpenFst
include_directories(${KALDI_CMAKE_DIST}/include/kaldi) # Kaldi
set(KALDI_LIBRARIES
    fst
    kaldi-base
    kaldi-matrix
    kaldi-util
    kaldi-cudamatrix
    kaldi-tree
    kaldi-gmm
    kaldi-hmm
    kaldi-transform
    kaldi-sgmm2
    kaldi-ivector
    kaldi-feat
    kaldi-fstext
    kaldi-lm
    kaldi-lat
    kaldi-decoder
    kaldi-cudadecoder
    kaldi-cudafeat
    kaldi-chain
    kaldi-kws
    kaldi-nnet
    kaldi-nnet2
    kaldi-nnet3
    kaldi-rnnlm
    kaldi-online
    kaldi-online2
)
foreach(x ${KALDI_LIBRARIES})
    add_library(${x} SHARED IMPORTED GLOBAL)
    set_target_properties(${x} PROPERTIES IMPORTED_LOCATION ${KALDI_CMAKE_DIST}/lib64/lib${x}.so)
endforeach()


# Torch (libtorch)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "${CMAKE_SOURCE_DIR}/deps/libtorch")
find_package(Torch REQUIRED)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${TORCH_CXX_FLAGS}")


# KenLM (libkenlm)
# refer to:
#   https://github.com/kpu/kenlm/blob/master/compile_query_only.sh#L22
# to get following KenLM source list
set(KENLM_ROOT "${CMAKE_SOURCE_DIR}/deps/kenlm")
include_directories(${KENLM_ROOT})
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DKENLM_MAX_ORDER=6")
add_library(kenlm_util SHARED
    ${KENLM_ROOT}/util/double-conversion/bignum.cc
    ${KENLM_ROOT}/util/double-conversion/bignum-dtoa.cc
    ${KENLM_ROOT}/util/double-conversion/cached-powers.cc
    ${KENLM_ROOT}/util/double-conversion/diy-fp.cc
    ${KENLM_ROOT}/util/double-conversion/double-conversion.cc
    ${KENLM_ROOT}/util/double-conversion/fast-dtoa.cc
    ${KENLM_ROOT}/util/double-conversion/fixed-dtoa.cc
    ${KENLM_ROOT}/util/double-conversion/strtod.cc
    ${KENLM_ROOT}/util/bit_packing.cc
    ${KENLM_ROOT}/util/ersatz_progress.cc
    ${KENLM_ROOT}/util/exception.cc
    ${KENLM_ROOT}/util/file.cc
    ${KENLM_ROOT}/util/file_piece.cc
    ${KENLM_ROOT}/util/float_to_string.cc
    ${KENLM_ROOT}/util/integer_to_string.cc
    ${KENLM_ROOT}/util/mmap.cc
    ${KENLM_ROOT}/util/murmur_hash.cc
    ${KENLM_ROOT}/util/parallel_read.cc
    ${KENLM_ROOT}/util/pool.cc
    ${KENLM_ROOT}/util/read_compressed.cc
    ${KENLM_ROOT}/util/scoped.cc
    ${KENLM_ROOT}/util/spaces.cc
    ${KENLM_ROOT}/util/string_piece.cc
    ${KENLM_ROOT}/util/usage.cc
)
add_library(kenlm_lm SHARED
    ${KENLM_ROOT}/lm/bhiksha.cc
    ${KENLM_ROOT}/lm/binary_format.cc
    ${KENLM_ROOT}/lm/config.cc
    ${KENLM_ROOT}/lm/lm_exception.cc
    ${KENLM_ROOT}/lm/model.cc
    ${KENLM_ROOT}/lm/quantize.cc
    ${KENLM_ROOT}/lm/read_arpa.cc
    ${KENLM_ROOT}/lm/search_hashed.cc
    ${KENLM_ROOT}/lm/search_trie.cc
    ${KENLM_ROOT}/lm/sizes.cc
    ${KENLM_ROOT}/lm/trie.cc
    ${KENLM_ROOT}/lm/trie_sort.cc
    ${KENLM_ROOT}/lm/value_build.cc
    ${KENLM_ROOT}/lm/virtual_interface.cc
    ${KENLM_ROOT}/lm/vocab.cc
)
set(KENLM_LIBRARIES
    kenlm_util
    kenlm_lm
)


# libsio
include_directories(${CMAKE_SOURCE_DIR})
set(SIO_ROOT "${CMAKE_SOURCE_DIR}/sio")
add_library(sio SHARED
    ${SIO_ROOT}/error.cc
)
add_executable(unittest 
    ${SIO_ROOT}/error_test.cc
    ${SIO_ROOT}/logger_test.cc
    ${SIO_ROOT}/panic_test.cc
    ${SIO_ROOT}/check_test.cc
    ${SIO_ROOT}/linked_list_test.cc
    ${SIO_ROOT}/allocator_test.cc
    ${SIO_ROOT}/audio_test.cc
    ${SIO_ROOT}/feature_extractor_test.cc
    ${SIO_ROOT}/dbg_test.cc
    ${SIO_ROOT}/struct_loader_test.cc
    ${SIO_ROOT}/finite_state_machine_test.cc
    ${SIO_ROOT}/language_model_test.cc
)
target_link_libraries(unittest
    gtest_main
    sio
    ${KENLM_LIBRARIES}
    ${TORCH_LIBRARIES}
    ${KALDI_LIBRARIES}
    ${ABSL_LIBRARIES}
)
include(GoogleTest)
gtest_discover_tests(unittest)


# stt bin
add_executable(stt stt.cc)
target_link_libraries(stt sio ${TORCH_LIBRARIES} ${KALDI_LIBRARIES} ${ABSL_LIBRARIES} ${KENLM_LIBRARIES})


# libtorch playground
add_executable(torchscript torchscript.cc)
target_link_libraries(torchscript ${TORCH_LIBRARIES})

