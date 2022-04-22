#!/usr/bin/env bash

MKL_ROOT=/opt/intel/mkl
KALDI_ROOT=deps/kaldi

mkdir -p $KALDI_ROOT/build && cd $KALDI_ROOT/build
MKLROOT=$MKL_ROOT cmake -DMATHLIB=MKL  -DCMAKE_INSTALL_PREFIX=../dist  ..
cmake --build . --target install -- -j40
cd -

echo "Done."

