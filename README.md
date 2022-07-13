HSPH CDM Variation v4.6.0 Branch
===
This is the variation branch from master branch for changing the building behavior and verifying the code in multiple platforms.

## For Development in FASRC
### FASRC Login

A password and verification code (not the same as Harvard Key) are required to log in to FASRC. For example, ssh to the cluster can be done by:

`ssh -X sseifi@login.rc.fas.harvard.edu`

## FASRC Environment

The modules required to build and run version 4 of the model on FASRC are listed below:

` module load gcc/9.3.0-fasrc01`

`module load cmake/3.5.2-fasrc01`

`module load boost/1.63.0-fasrc01`

`module load gperftools`

`module load sqlite/3081101-fasrc01`

For linking the loaded Boost library to the compiler, the following path has to be defined. Simply execute the following lines in bash:

`export BOOST_DIR=/n/helmod/apps/centos7/Core/boost/1.63.0-fasrc01/`

`export BOOST_LIBRARYDIR=/n/helmod/apps/centos7/Core/boost/1.72.0-fasrc01/lib`

## Clone the 'transm' Repository

Clone the transm repository using your github username and password:

`git clone https://github.com/hsphcdm/transm.git`

## Building the Model

The build directory in the repository contains a script named 'build.sh' which builds the model.

In the build directory, run the command. You maybe be prompted for your github username and password again during the first build for it to clone the CEPAC repository.

`./build.sh`
