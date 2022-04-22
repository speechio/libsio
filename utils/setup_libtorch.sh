#!/usr/bin/env bash

# get download link from webpage:
#   https://pytorch.org/
#
# installation note:
#   https://pytorch.org/cppdocs/installing.html

cd deps

if [[ `uname -s` == "Linux" ]]; then
    wget https://download.pytorch.org/libtorch/nightly/cpu/libtorch-shared-with-deps-latest.zip
    unzip libtorch-shared-with-deps-latest.zip
elif [[ `uname -s` == "Darwin" ]]; then
    wget https://download.pytorch.org/libtorch/nightly/cpu/libtorch-macos-latest.zip
    unzip libtorch-macos-latest.zip
fi

cd -

echo "Done"

