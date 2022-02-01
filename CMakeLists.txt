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
# libtorch playground
add_executable(torchscript torchscript.cc)
target_link_libraries(torchscript ${TORCH_LIBRARIES})


# KenLM (libkenlm)
include_directories(${CMAKE_SOURCE_DIR}/deps/kenlm)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DKENLM_MAX_ORDER=6")
add_library(kenlm SHARED IMPORTED GLOBAL)
set_target_properties(kenlm PROPERTIES IMPORTED_LOCATION ${CMAKE_SOURCE_DIR}/deps/kenlm/libkenlm.so)


# libsio
include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory(src)


# bin
add_executable(stt stt.cc)
target_link_libraries(stt sio ${TORCH_LIBRARIES} ${KALDI_LIBRARIES} ${ABSL_LIBRARIES})

