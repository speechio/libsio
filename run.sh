export SIO_VERBOSITY=DEBUG
cmake -S . -B build
cmake --build build -j 10
build/sio/unittest
build/stt
