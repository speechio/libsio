KALDI_ROOT=/home/speechio/work/kaldi
MKL_ROOT=/opt/intel/mkl

mkdir -p $KALDI_ROOT/build
cd $KALDI_ROOT/build
MKLROOT=$MKL_ROOT cmake -DMATHLIB=MKL  -DCMAKE_INSTALL_PREFIX=../dist  ..
cmake --build . --target install -- -j40
cd -

ln -s $KALDI_ROOT/dist kaldi_dist

