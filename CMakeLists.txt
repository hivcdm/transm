cmake_minimum_required(VERSION 3.14)
project(transm)

# maybe not needed for now but should be replaced with "transm"
set(TRANSM_BINARY ${CMAKE_PROJECT_NAME})

# Then specify all the flags we need for our compiler and linker
# specify the C++ standard
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED True)

message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

#set_property(TARGET transm PROPERTY CXX_STANDARD 14) # Proper way to require c++14 compatibility,
#set_property(TARGET transm PROPERTY CXX_STANDARD_REQUIRED ON) # but it causes issues on CentOS 7 + CMake 2.8,
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fpermissive -fno-builtin-malloc -fno-builtin-calloc -fno-builtin-realloc -fno-builtin-free") # so we just set the flag
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -march=native")

find_package(OpenMP)
if (OPENMP_FOUND)
	set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
	set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
	set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${OpenMP_EXE_LINKER_FLAGS}")
endif()

include(GNUInstallDirs)
include(ExternalProject)

# First let's see if we have what we need
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

set(Boost_USE_STATIC_LIBS ON)

#find_package(Boost
#	1.82.0
#	COMPONENTS system
#	REQUIRED
#)
include_directories(${Boost_INCLUDE_DIR})

add_definitions(-DUSE_GOOGLE_PERF_TOOLS=${USE_GOOGLE_PERF_TOOLS})
message(STATUS "Using Google Performance Tools: ${USE_GOOGLE_PERF_TOOLS}")
if (USE_GOOGLE_PERF_TOOLS)
        find_package (Gperftools REQUIRED)
endif (USE_GOOGLE_PERF_TOOLS)

set(CMAKE_FIND_FRAMEWORK NEVER) # on OSX, don't search frameworks
set(CMAKE_FIND_APPBUNDLE NEVER) # or appbundles for sqlite3
find_package (SQLITE3 REQUIRED)
include_directories(${SQLITE3_INCLUDE_DIRS})

# We only need to include the headers for rana
ExternalProject_Add(
	rana_project 
	PREFIX ${CMAKE_BINARY_DIR}/third-party
	GIT_REPOSITORY https://github.com/hsphcdm/rana.git
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(rana_project source_dir)
include_directories(${source_dir}/include)

# Same thing here, only the headers are needed

ExternalProject_Add(
	tclap_project 
	DEPENDS rana_project
	PREFIX ${CMAKE_BINARY_DIR}/third-party
	GIT_REPOSITORY https://github.com/hsphcdm/tclap.git
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(tclap_project source_dir)
include_directories(${source_dir}/include)

# This is one of the two libraries we need, good thing is it has its own CMake files

ExternalProject_Add(
	pugixml_project
	PREFIX ${CMAKE_BINARY_DIR}/third-party
	GIT_REPOSITORY https://github.com/hsphcdm/pugixml.git
	CONFIGURE_COMMAND cmake -DCMAKE_BUILD_TYPE=Release ../pugixml_project
    BUILD_COMMAND make pugixml
    INSTALL_COMMAND mv libpugixml.a ${CMAKE_SOURCE_DIR}/lib
)

ExternalProject_Get_Property(pugixml_project source_dir)
include_directories(${source_dir}/src)
add_library(pugixml STATIC IMPORTED)
set_property(TARGET pugixml PROPERTY IMPORTED_LOCATION  ${CMAKE_SOURCE_DIR}/lib/libpugixml.a)
add_dependencies(pugixml pugixml_project)

# Then there is CEPAC, we also use it as a library

ExternalProject_Add(
	cepac_project 
	PREFIX ${CMAKE_BINARY_DIR}/third-party
	GIT_REPOSITORY https://github.com/hsphcdm/cepac-svn.git
	GIT_TAG cepac-transm
	UPDATE_COMMAND ""
	CMAKE_ARGS -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
	#CONFIGURE_COMMAND cmake cepac_project
	BUILD_COMMAND make
	INSTALL_COMMAND mv libcepac.a ${CMAKE_SOURCE_DIR}/lib/ 
)

ExternalProject_Get_Property(cepac_project source_dir)
include_directories(${source_dir}/src)
add_library(cepac STATIC IMPORTED)
set_property(TARGET cepac PROPERTY IMPORTED_LOCATION  ${CMAKE_SOURCE_DIR}/lib/libcepac.a)
add_dependencies(cepac cepac_project)

# Now it is time to compile our own code

#set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# The headers are nicely referenced with relative path, we can simply include the source directory
include_directories(src)

# Then we add our source files, they are listed in our helper file
include(cmake.sources)

# We add our executable
add_executable(transm src/transm.cpp ${TRANSM_SOURCE_FILES})
add_dependencies(transm rana_project tclap_project cepac_project)

# Read and set up the version number
file(READ VERSION TRANSM_VER)
string(STRIP ${TRANSM_VER} TRANSM_VER) # remove whitespace

message("Building version ${TRANSM_VER}")
#set_property(TARGET transm PROPERTY VERSION ${TRANSM_VER}) # Better way of indicating our version number, however...
set(CMAKE_EXECUTABLE_SUFFIX "-v${TRANSM_VER}") # ...we need to use the following for compatibility with ridiculous assert in transm code

# Add a makefile target that creates a header file with the version from VERSION
add_custom_target(create-version-header 
   COMMAND ../version_header.sh ${CMAKE_SOURCE_DIR}/ ${TRANSM_VER}
   COMMENT "Creating version.h"
)
add_dependencies(transm create-version-header)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES 
	${CMAKE_SOURCE_DIR}/src/utility/version.h)

# We link our libraries and we are done
if(GOOGLE_PERFTOOLS_FOUND)
	target_link_libraries(transm ${TCMALLOC_LIBRARY} ${PROFILER_LIBRARY})
endif(GOOGLE_PERFTOOLS_FOUND)

# Boost linrary was disabled here originally: issues with FASRC
target_link_libraries(transm cepac pugixml ${SQLITE3_LIBRARIES} ${Boost_LIBRARIES})

# Building google tests

# Include GoogleTest
include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/5376968f6948923e2411081fd9372e71a59d8e77.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE) # For Windows
FetchContent_MakeAvailable(googletest)

include(CTest)
enable_testing()

# Only include test subdirectory if BUILD_TESTING is on
if (BUILD_TESTING)
    add_subdirectory(tests)
endif()



# If the build is successful we remind the user of the location of the binary
add_custom_command(TARGET transm POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
                   "Build complete. The binary is in ${EXECUTABLE_OUTPUT_PATH}.")
 
add_custom_target(clean-cmake-files
   COMMAND ${CMAKE_COMMAND} -P ${CMAKE_SOURCE_DIR}/clean-all.cmake
)

add_custom_target(tags
    COMMAND find ${CMAKE_SOURCE_DIR} -name "*.cpp" -print -or -name "*.h" -print | xargs etags --append
    COMMAND install TAGS ${CMAKE_SOURCE_DIR}/src
)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES TAGS) 

add_custom_target(cppcheck
    COMMAND which cppcheck &> /dev/null
    COMMAND -rm -rf CPPCHECK
    COMMAND mkdir CPPCHECK
    COMMAND cppcheck --enable=all --force -I${source_dir} --std=c++11 ${CMAKE_SOURCE_DIR}
			 --xml-version=2 --xml -q 2>CPPCHECK/cdm_cppcheck.xml
    COMMAND cppcheck-htmlreport --title=cdm --file=CPPCHECK/cdm_cppcheck.xml --report=CPPCHECK
)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES CPPCHECK) 
