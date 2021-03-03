#!/bin/bash
#
# This is a thin wrapper around the cleanup script in the root
# of the project. It will remove  all build artifacts and 
# the code of the external projects. 
# Use to clean up the build environment.

# That's all we need to do
rm -rf third-party
cmake -P clean-all.cmake
