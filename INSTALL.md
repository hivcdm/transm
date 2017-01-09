#Installing HIV-CDM

Pre-compiled binaries for several platforms are available at `https://github.com/hsphcdm/transm-user`. 

To compile the source code in this repository, simply clone this GitHub repository, navigate to the build subfolder and launch the `build.sh` script. At the moment the only platforms supported by the build system are Linux and macOS.

The following software is required in order to compile the code successfully:

- A C++14-compatible compiler (GCC5+ or recent CLANG)
- CMake 2.8 or greater
- Boost

Google Performance Tools is optional, although recommended for high-performance builds.
