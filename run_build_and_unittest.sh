rm -f build/src/unittest
rm -f build/stt
rm -f build/torchscript
#python torchscript.py

cmake -S . -B build
cmake --build build -j 40
SIO_VERBOSITY=DEBUG  build/src/unittest
#SIO_VERBOSITY=INFO build/stt

#build/torchscript

