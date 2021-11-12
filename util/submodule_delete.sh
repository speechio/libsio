path_to_submodule=libsio/deps/abseil-cpp

# Remove the submodule entry from .git/config
git submodule deinit -f $path_to_submodule

# Remove the submodule directory from the superproject's .git/modules directory
rm -rf .git/modules/$path_to_submodule

# Remove the entry in .gitmodules and remove the submodule directory located at path/to/submodule
git rm -f $path_to_submodule

