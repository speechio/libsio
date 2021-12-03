export SIO_LOG_LEVEL=INFO
mkdir -p build && cd build
cmake .. -DBUILD_TESTING=OFF
cmake --build . --target stt
./stt
