rm -f build/unittest
rm -f build/stt

cmake -S . -B build
cmake --build build -j 40

SIO_VERBOSITY=INFO build/unittest
SIO_VERBOSITY=INFO build/stt

