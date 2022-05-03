cmake_minimum_required(VERSION 3.14)

project(sio LANGUAGES CXX VERSION 0.1)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

#add_compile_options(-W -Wall -g)

# GTest
include(FetchContent)
FetchContent_Declare(gtest
    URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(gtest)


# Abseil (libabsl)
add_subdirectory(deps/abseil-cpp)

set(ABSL_LIBRARIES
    absl::base
    absl::memory
    absl::strings
    absl::str_format
    absl::type_traits
    absl::flat_hash_map
)


# Kaldi (libkaldi)
set(KALDI_CMAKE_DIST ${CMAKE_SOURCE_DIR}/deps/kaldi/dist)

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
    kaldi-ivector
    kaldi-feat
    kaldi-fstext
    kaldi-lm
    kaldi-lat
    kaldi-decoder
    #kaldi-cudadecoder
    #kaldi-cudafeat
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

    if (CMAKE_SYSTEM_NAME STREQUAL Linux)
        set_target_properties(${x} PROPERTIES IMPORTED_LOCATION ${KALDI_CMAKE_DIST}/lib64/lib${x}.so)
    elseif(CMAKE_SYSTEM_NAME STREQUAL Darwin)
        set_target_properties(${x} PROPERTIES IMPORTED_LOCATION ${KALDI_CMAKE_DIST}/lib/lib${x}.dylib)
    else()
        message(FATAL_ERROR "Unsupported platform, need to be Linux/Darwin.")
    endif()

endforeach()


# Torch (libtorch)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} deps/libtorch)
find_package(Torch REQUIRED)


# KenLM (libkenlm)
# refer to: https://github.com/kpu/kenlm/blob/master/compile_query_only.sh#L22
# to get following source list
set(KENLM_ROOT deps/kenlm)
add_library(kenlm SHARED
    # util module
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

    # lm module
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
target_include_directories(kenlm PUBLIC ${KENLM_ROOT})
target_compile_definitions(kenlm PUBLIC -DKENLM_MAX_ORDER=6)
set(KENLM_LIBRARIES kenlm)


# libsioxx
add_library(sioxx INTERFACE)
target_include_directories(sioxx INTERFACE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/deps
    ${KALDI_CMAKE_DIST}/include ${KALDI_CMAKE_DIST}/include/kaldi # needed here because Kaldi is not imported through cmake
)
target_link_libraries(sioxx INTERFACE ${KENLM_LIBRARIES} ${TORCH_LIBRARIES} ${KALDI_LIBRARIES} ${ABSL_LIBRARIES})
#target_compile_options(sioxx INTERFACE -fsanitize=address)
#target_link_options(sioxx INTERFACE -fsanitize=address)


# libsioxx unittest
add_executable(unittest
    src/sio/error_test.cc
    src/sio/logger_test.cc
    src/sio/check_test.cc
    src/sio/linked_list_test.cc
    src/sio/allocator_test.cc
    src/sio/audio_test.cc
    src/sio/feature_extractor_test.cc
    src/sio/dbg_test.cc
    src/sio/struct_loader_test.cc
    src/sio/fst_test.cc
    src/sio/language_model_test.cc
)
target_link_libraries(unittest gtest_main sioxx)


# libsio
add_library(sio STATIC
    src/sio.cc
)
target_include_directories(sio
    PUBLIC ${CMAKE_SOURCE_DIR}/src
)
target_link_libraries(sio PRIVATE sioxx)


# stt bin
add_executable(sio_stt sio_stt.cc)
target_link_libraries(sio_stt sioxx)

