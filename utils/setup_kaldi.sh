#!/usr/bin/env bash

export KALDI_ROOT=/home/speechio/work/kaldi
export MKL_ROOT=/opt/intel/mkl

if [ -z $KALDI_ROOT ]; then
    echo "KALDI_ROOT env variable is empty, please set it to your local kaldi repository."
    exit 1
fi

mkdir -p $KALDI_ROOT/build
cd $KALDI_ROOT/build
MKLROOT=$MKL_ROOT cmake -DMATHLIB=MKL  -DCMAKE_INSTALL_PREFIX=../dist  ..
cmake --build . --target install -- -j40
cd -

cd deps/
ln -s $KALDI_ROOT/dist kaldi_dist
cd -

echo "Done."

