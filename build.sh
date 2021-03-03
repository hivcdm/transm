#!/bin/bash
#
# You can invoke this shell script with additional command-line
# arguments, they will be passed directly to CMake. Examples:
#
# $0 -DCMAKE_BUILD_TYPE=Debug (for a debug build)
# $0 -DDEFINE_CALIB=true (for a build with calibration enabled)
# CXX=g++-5 CC=gcc-5 $0 (to use another compiler like GCC5)

# If we have no additional arguments we go for a release build
CMAKE_ARGS=()
if [[ $# -eq 0 ]]
then
	CMAKE_ARGS+=("-D CMAKE_BUILD_TYPE=Release")
	CMAKE_ARGS+=("-D USE_GOOGLE_PERF_TOOLS=true")
else
	while (($#)); do
	if [[ $1 == "--debug" ]]
	then
		CMAKE_ARGS+=("-D CMAKE_BUILD_TYPE=Debug")
	elif [[ $1 == "--no-gperftools" ]]
	then
		CMAKE_ARGS+=("-D USE_GOOGLE_PERF_TOOLS=false")
	else
		CMAKE_ARGS+=($1)
	fi
	shift
	done
fi

# Let's clear the cache first if it's there
rm -f CMakeCache.txt

# Then we call cmake and start compiling
cmake ${CMAKE_ARGS[@]} . && make
