export SIO_VERBOSITY=INFO
cmake -S . -B build
cmake --build build
build/sio/unittest
