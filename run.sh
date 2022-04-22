rm -f build/unittest
rm -f build/stt

cmake -S . -B build
cmake --build build -j 40

if [[ `uname -s` == "Darwin" ]]; then
    export DYLD_LIBRARY_PATH=/opt/intel/mkl/lib:deps/kaldi/dist/lib
fi

SIO_VERBOSITY=INFO build/unittest
SIO_VERBOSITY=INFO build/stt

