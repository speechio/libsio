#!/usr/bin/env bash
git submodule add --depth 1 https://github.com/kaldi-asr/kaldi  deps/kaldi
git submodule add --depth 1 https://github.com/abseil/abseil-cpp  deps/abseil-cpp
git submodule add --depth 1 https://github.com/kpu/kenlm  deps/kenlm
