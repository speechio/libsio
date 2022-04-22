#!/usr/bin/env bash

# document
#   https://pytorch.org/cppdocs/installing.html
cd deps
wget https://download.pytorch.org/libtorch/nightly/cpu/libtorch-shared-with-deps-latest.zip
unzip libtorch-shared-with-deps-latest.zip
cd -

echo "Done"

