cmake_minimum_required(VERSION 2.8)
#set(CMAKE_C_COMPILER_INIT g++-5)
project (transm)

include(GNUInstallDirs)
include(ExternalProject)

find_package (
	Boost 
	REQUIRED
)

include_directories (${Boost_INCLUDE_DIR})

# We only need to include the headers for rana
ExternalProject_Add (
	rana_project 
	PREFIX ${CMAKE_SOURCE_DIR}/third-party
	GIT_REPOSITORY https://github.com/tfussell/rana
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property (rana_project source_dir)
include_directories (${source_dir}/include)

# Same thing here, only the headers are needed
ExternalProject_Add (
	tclap_project 
	DEPENDS rana_project
	PREFIX ${CMAKE_SOURCE_DIR}/third-party
	GIT_REPOSITORY https://github.com/eile/tclap.git
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property (tclap_project source_dir)
include_directories (${source_dir}/include)

# This is one of the two libraries we need, good thing is it has its own CMake files
ExternalProject_Add (
	pugixml_project 
	PREFIX ${CMAKE_SOURCE_DIR}/third-party
	GIT_REPOSITORY https://github.com/zeux/pugixml.git
	CONFIGURE_COMMAND cmake ../pugixml_project/scripts
    BUILD_COMMAND make pugixml COMMAND printenv COMMAND echo ${Boost_INCLUDE_DIR}
    INSTALL_COMMAND mv libpugixml.a ${CMAKE_SOURCE_DIR}/lib
    LOG_BUILD 1
)

ExternalProject_Get_Property (pugixml_project source_dir)
include_directories (${source_dir}/src)
add_library (pugixml STATIC IMPORTED)
set_property (TARGET pugixml PROPERTY IMPORTED_LOCATION  lib/libpugixml.a)
add_dependencies (pugixml pugixml_project)

# Then there is CEPAC, we also use it as a library
ExternalProject_Add (
	cepac_project 
	PREFIX ${CMAKE_SOURCE_DIR}/third-party
	GIT_REPOSITORY https://github.com/hsphcdm/cepac.git
	GIT_TAG 4.4a
	CONFIGURE_COMMAND pwd COMMAND premake5 --file=../cepac_project/build/premake5.lua gmake
	BUILD_COMMAND make -C ../cepac_project/build/gmake cepac config=release_x64
	INSTALL_COMMAND pwd COMMAND mv ../cepac_project/lib/libcepac.a ${CMAKE_SOURCE_DIR}/lib
)

ExternalProject_Get_Property (cepac_project source_dir)
include_directories (${source_dir}/src)
add_library (cepac STATIC IMPORTED)
set_property (TARGET cepac PROPERTY IMPORTED_LOCATION  lib/libcepac.a)
add_dependencies (cepac cepac_project)

# Now it is time to compile our own code

set (CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# The headers are nicely referenced with relative path
include_directories(source) 

# Ideally we would update this to use target_source(), however it requires CMake 3.1+
file (GLOB_RECURSE TRANSM_SOURCE_FILES "source/*.cpp")
file (GLOB_RECURSE WINDOWS_SOURCE_FILES "source/utility/platform/windows/*")
list(REMOVE_ITEM TRANSM_SOURCE_FILES ${WINDOWS_SOURCE_FILES})

add_executable (transm source/main.cpp ${TRANSM_SOURCE_FILES})
file(READ VERSION TRANSM_VER)
# Better way of indicating our version number, however...
#set_property(TARGET transm PROPERTY VERSION ${TRANSM_VER}) 
# ...we need to use the following for compatibility with ridiculous assert in transm code
set(CMAKE_EXECUTABLE_SUFFIX "-v${TRANSM_VER}") 
#set_property(TARGET transm PROPERTY CXX_STANDARD 14) 
# The one above is better, but it causes issues on CentOS 7 + CMake 2.8
set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fpermissive")

target_link_libraries (transm pugixml cepac)
#install(TARGETS transm DESTINATION bin)

#better find for own libraries
#FIND_LIBRARY(TOURTRE_LIBRARY tourtre /absPath/libtourtre)
#TARGET_LINK_LIBRARIES(testOVAS ${TOURTRE_LIBRARY})
