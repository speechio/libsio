#!/bin/bash
git submodule add https://github.com/kpu/kenlm deps/kenlm
git submodule update --init --recursive
echo "Done."

