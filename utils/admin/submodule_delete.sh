#!/usr/bin/env bash

submodules="deps/abseil-cpp deps/kenlm"

for m in $submodules; do
    # Remove the submodule entry from .git/config
    git submodule deinit -f $m

    # Remove the submodule directory from the project's .git/modules directory
    rm -rf .git/modules/$m

    # Remove the entry in .gitmodules and remove the submodule directory
    git rm -f $m
done

