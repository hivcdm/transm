#!/bin/bash
#
# This is a thin wrapper around the cleanup script in the root
# of the project. It will remove  all build artifacts and 
# the code of the external projects. 
# Use to clean up the build environment.

make clean
cmake -P clean-all.cmake
