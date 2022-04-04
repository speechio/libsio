#!/usr/bin/env bash

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

