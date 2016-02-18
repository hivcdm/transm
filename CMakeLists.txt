cmake_minimum_required(VERSION 3.4)
#set(CMAKE_C_COMPILER_INIT g++-5)
project (transm)

set (CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set (BOOST_INCLUDEDIR /usr/local/include)
include (/usr/local/share/cmake/Modules/ExternalProject.cmake)
include(GNUInstallDirs)
include_directories (Boost_INCLUDE_DIRS)
include_directories (/usr/local/include)


#file (GLOB_RECURSE SOURCE_FILES "../source/*.cpp")
#file (GLOB_RECURSE SOURCE_FILES "../source/*.hpp")
#list (REMOVE_ITEM SOURCE_FILES "Platform/*.*")

find_package (
	Boost 
	REQUIRED
)


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

ExternalProject_Add (
	pugixml_project 
	PREFIX ${CMAKE_SOURCE_DIR}/third-party
	GIT_REPOSITORY https://github.com/zeux/pugixml.git
	CONFIGURE_COMMAND cmake ../pugixml_project/scripts
	#BINARY_DIR obj
	#INSTALL_DIR ../obj
  	#CMAKE_ARGS -DDESTDIR=<INSTALL_DIR>
  	#CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=<lib>
    BUILD_COMMAND make pugixml COMMAND echo ${BINARTY_DIR}
    INSTALL_COMMAND ""
    LOG_BUILD 1
)

ExternalProject_Get_Property (pugixml_project source_dir)
include_directories (${source_dir}/src)
add_library (pugixml STATIC IMPORTED)
set_property (TARGET pugixml PROPERTY IMPORTED_LOCATION  ${CMAKE_SOURCE_DIR}/third-party/src/pugixml_project-build/libpugixml.a)
add_dependencies (pugixml pugixml_project)

ExternalProject_Add (
	cepac_project 
	PREFIX ${CMAKE_SOURCE_DIR}/third-party
	GIT_REPOSITORY https://github.com/hsphcdm/cepac.git
	GIT_TAG 4.4a
	#CONFIGURE_COMMAND = ""
	#BUILD_COMMAND = ""
	CONFIGURE_COMMAND premake5 --file=${CMAKE_SOURCE_DIR}/third-party/src/cepac_project/build/premake5.lua gmake
	BUILD_COMMAND make -C ${CMAKE_SOURCE_DIR}/third-party/src/cepac_project/build/gmake cepac config=release_x64
	#INSTALL_DIR ${CMAKE_BINARY_DIR}/obj
	INSTALL_COMMAND ""
)

ExternalProject_Get_Property (cepac_project source_dir)
include_directories (${source_dir}/src)
add_library (cepac STATIC IMPORTED)
set_property (TARGET cepac PROPERTY IMPORTED_LOCATION  ${CMAKE_SOURCE_DIR}/third-party/src/cepac_project/lib/libcepac.a)
add_dependencies (cepac cepac_project)


#add_library (tclap STATIC IMPORTED)
#add_library (rana STATIC IMPORTED)
#add_library (libthirdparty STATIC IMPORTED $<TARGET_OBJECTS:pugixml>)
#add_library (libthirdparty INTERFACE)
#target_link_libraries (libthirdparty INTERFACE pugixml cepac)
#add_dependencies (tclap tclap_project)
#add_dependencies (rana rana_project)
#add_dependencies (pugixml pugixml_project)



#set(LIBS ${LIBS} ${CMAKE_SOURCE_DIR}/third-party/src/pugixml_project-build)

#file (GLOB_RECURSE SOURCE_FILES "source/*.cpp")
#file (GLOB_RECURSE SOURCE_FILES "source/*.hpp")

include_directories(${CMAKE_SOURCE_DIR}/source)
#include_directories(${CMAKE_BINARY_DIR})

add_executable (transm source/main.cpp)
set_property(TARGET transm PROPERTY CXX_STANDARD 14)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -v")
#add_dependencies (transm tclap_project  rana_project)
target_link_libraries (transm pugixml cepac)
