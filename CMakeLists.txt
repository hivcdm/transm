cmake_minimum_required(VERSION 2.8)
project(transm)

include(GNUInstallDirs)
include(ExternalProject)

# Switches for the compilation flags that we can trigger from the build script
set(WGOOGLE FALSE CACHE BOOL "Link to Google Performance Tools")
set(ALLSTATIC FALSE CACHE BOOL "All static build")

#This ugly bunch of lines is a hack to compile properly on Odyssey:
#they have an older version of Boost in /usr that causes a bug in
#the lib paths with cmake's find_package.
#set(Boost_NO_BOOST_CMAKE ON) 
#set(Boost_USE_STATIC_LIBS ON)
#set(BOOST_INCLUDEDIR /n/sw/fasrcsw/apps/Core/boost/1.59.0-fasrc01/include)
#set(BOOST_ROOT /n/sw/fasrcsw/apps/Core/boost/1.59.0-fasrc01/lib)
#set(Boost_NO_SYSTEM_PATHS ON)

set(Boost_USE_STATIC_LIBS ON)
find_package(
	Boost 
	COMPONENTS system filesystem
	REQUIRED
)

include_directories(${Boost_INCLUDE_DIR})

# We only need to include the headers for rana
ExternalProject_Add(
	rana_project 
	PREFIX ${CMAKE_SOURCE_DIR}/build/third-party
	GIT_REPOSITORY https://github.com/tfussell/rana
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
	PREFIX ${CMAKE_SOURCE_DIR}/build/third-party
	GIT_REPOSITORY https://github.com/eile/tclap.git
	CONFIGURE_COMMAND ""
	BUILD_COMMAND ""
    INSTALL_COMMAND ""
)

ExternalProject_Get_Property(tclap_project source_dir)
include_directories(${source_dir}/include)

# This is one of the two libraries we need, good thing is it has its own CMake files
ExternalProject_Add(
	pugixml_project
	PREFIX ${CMAKE_SOURCE_DIR}/build/third-party
	GIT_REPOSITORY https://github.com/zeux/pugixml.git
	CONFIGURE_COMMAND cmake -DCMAKE_BUILD_TYPE=Release ../pugixml_project/scripts
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
	PREFIX ${CMAKE_SOURCE_DIR}/build/third-party
	GIT_REPOSITORY https://github.com/hsphcdm/cepac.git
	GIT_TAG 4.4a
	UPDATE_COMMAND ""
	CMAKE_ARGS=-DCMAKE_BUILD_TYPE=Release
	#CONFIGURE_COMMAND cmake ../cepac_project
	BUILD_COMMAND make
	INSTALL_COMMAND mv libcepac.a ${CMAKE_SOURCE_DIR}/lib
)

ExternalProject_Get_Property(cepac_project source_dir)
include_directories(${source_dir}/src)
add_library(cepac STATIC IMPORTED)
set_property(TARGET cepac PROPERTY IMPORTED_LOCATION  ${CMAKE_SOURCE_DIR}/lib/libcepac.a)
add_dependencies(cepac cepac_project)

# Now it is time to compile our own code
set(CMAKE_BINARY_DIR ${CMAKE_SOURCE_DIR}/build)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

# The headers are nicely referenced with relative path, we can simply include the source directory
include_directories(source) 

# Ugly globbing of the source files: ideally we would update this to use target_source(), however it requires CMake 3.1+
file(GLOB_RECURSE TRANSM_SOURCE_FILES "source/*.cpp")
file(GLOB_RECURSE WINDOWS_SOURCE_FILES "source/utility/platform/windows/*")
list(REMOVE_ITEM TRANSM_SOURCE_FILES ${WINDOWS_SOURCE_FILES})

# We add our executable
add_executable(transm source/main.cpp ${TRANSM_SOURCE_FILES})

# Read and set up the version number
file(READ VERSION TRANSM_VER)
#set_property(TARGET transm PROPERTY VERSION ${TRANSM_VER}) # Better way of indicating our version number, however...
set(CMAKE_EXECUTABLE_SUFFIX "-v${TRANSM_VER}") # ...we need to use the following for compatibility with ridiculous assert in transm code

# Then specify all the flags we need for our compiler and linker
#set_property(TARGET transm PROPERTY CXX_STANDARD 14) # Proper way to require c++14 compatibility, 
#set_property(TARGET transm PROPERTY CXX_STANDARD_REQUIRED ON) # but it causes issues on CentOS 7 + CMake 2.8,
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -fpermissive") # so we just set the flag

# These can be triggered as additional arguments of the build script
if(ALLSTATIC)
	message("ALLSTATIC is true")
	set(CMAKE_EXE_LINKER_FLAGS "-static ${CMAKE_EXE_LINKER_FLAGS}") # Uncomment this to get a fully static build... Doesn't work on Mac though
endif(ALLSTATIC)

if(WGOOGLE)
	message("WGOOGLE is true")	
	set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,-lprofiler") # This is to link the Google profiler
endif(WGOOGLE)

# We link our libraries and we are done
target_link_libraries(transm cepac pugixml ${Boost_LIBRARIES})

# If the build is successful we remind the user of the location of the binary
add_custom_command(TARGET transm POST_BUILD
                   COMMAND ${CMAKE_COMMAND} -E cmake_echo_color --cyan
                   "Build complete. The binary is in ${EXECUTABLE_OUTPUT_PATH}.")