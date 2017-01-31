# Auxiliary CMake script to clean up the build environment

message ("Cleaning up the build environment...")

set(build_files ${CMAKE_BINARY_DIR}/CMakeCache.txt
                    ${CMAKE_BINARY_DIR}/cmake_install.cmake
                    ${CMAKE_BINARY_DIR}/Makefile
                    ${CMAKE_BINARY_DIR}/CMakeFiles
                    ${CMAKE_BINARY_DIR}/third-party/src
                    ${CMAKE_BINARY_DIR}/third-party/tmp
)

foreach(file ${build_files})

  if (EXISTS ${file})
     file(REMOVE_RECURSE ${file})
  endif()

endforeach(file)

message ("Done!")