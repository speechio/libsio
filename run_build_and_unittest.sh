rm build/src/unittest
rm build/stt
#rm build/torchscript
#python torchscript.py

cmake -S . -B build
cmake --build build -j 10
SIO_VERBOSITY=DEBUG  build/src/unittest
SIO_VERBOSITY=INFO build/stt

#build/torchscript

