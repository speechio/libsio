#!/usr/bin/env bash

## setup kaldi
utils/setup_kaldi.sh

## setup libtorch
utils/setup_libtorch.sh

## setup abseil & kenlm
utils/setup_submodules.sh
