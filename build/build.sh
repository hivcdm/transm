#!/bin/bash
#
# You can invoke this shell script with additional command-line
# arguments.  They will be passed directly to CMake.
#

# If we have no additional arguments we go for a release build
if [[ $# -eq 0 ]]
then
	CMAKE_ARGS="-D NDEBUG=1 -D CMAKE_BUILD_TYPE=Release"
else
	CMAKE_ARGS=$@
fi

# Let's clear the cache first if it's there
rm -f CMakeCache.txt

# Then we call cmake and start compiling
cmake $CMAKE_ARGS .. && make