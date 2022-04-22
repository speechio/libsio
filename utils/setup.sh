#!/usr/bin/env bash

## pull repos from all submodules
utils/setup_submodules.sh

## setup kaldi
utils/setup_kaldi.sh

## setup libtorch
utils/setup_libtorch.sh

