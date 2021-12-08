cmake_minimum_required(VERSION 3.14)

project(sio LANGUAGES CXX VERSION 0.1)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

# gtest
include(FetchContent)
FetchContent_Declare(
  gtest
  URL https://github.com/google/googletest/archive/609281088cfefc76f9d0ce82e1ff6c30cc3591e5.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(gtest)

# abseil
include_directories(deps/abseil-cpp)
add_subdirectory(deps/abseil-cpp)

# kaldi
set(KALDI_CMAKE_DIST "${CMAKE_SOURCE_DIR}/deps/kaldi_dist")
include_directories(${KALDI_CMAKE_DIST}/include)       # OpenFst
include_directories(${KALDI_CMAKE_DIST}/include/kaldi) # Kaldi

set(KALDI_LIBS
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

foreach(x ${KALDI_LIBS})
    add_library(${x} SHARED IMPORTED GLOBAL)
    set_target_properties(${x} PROPERTIES IMPORTED_LOCATION ${KALDI_CMAKE_DIST}/lib64/lib${x}.so)
endforeach()

# libsio
include_directories(${CMAKE_SOURCE_DIR})
add_subdirectory(sio)

# bin
add_executable(stt stt.cc)
target_link_libraries(stt sio ${KALDI_LIBS} absl::strings absl::str_format absl::type_traits absl::flat_hash_map)

