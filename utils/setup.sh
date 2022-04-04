#!/usr/bin/env bash

## setup abseil
utils/setup_abseil.sh

## setup kaldi
export KALDI_ROOT=/home/speechio/work/kaldi
export MKL_ROOT=/opt/intel/mkl
utils/setup_kaldi.sh

## setup libtorch
utils/setup_libtorch.sh

## KenLM
#pip3 install https://github.com/kpu/kenlm/archive/master.zip
utils/setup_kenlm.sh

