#!/bin/bash

echo "BAT install_linux start!"

# Ensure the script is run with superuser privileges for the package installations
if [ "$(id -u)" != "0" ]; then
   echo "Please type sudo and this file name because it will first install a few libraries."
   exit 1
fi

# Install GMP and MPFR libraries
apt-get update
apt-get install -y libgmp-dev libmpfr-dev

# Set the current directory to where the script is located
cd "$(dirname "$0")"

# Clone the repository only if it doesn't already exist
if [ ! -d "../wood" ]; then
    git clone https://github.com/petrasvestartas/wood.git ../wood
fi

# Navigate to the cmake directory and create a build directory
mkdir -p cmake/build
cd cmake/build

# Step 5: Download libraries
cmake -B . -S .. -DGET_LIBS=ON -DCOMPILE_LIBS=OFF -DBUILD_MY_PROJECTS=OFF -DRELEASE_DEBUG=ON -DCGAL_DISABLE_GMP=ON -DCMAKE_BUILD_TYPE="Release" -G "Unix Makefiles" && make

# Step 6: Build 3rd-party libraries
cmake -B . -S .. -DGET_LIBS=OFF -DBUILD_MY_PROJECTS=ON -DCOMPILE_LIBS=ON -DRELEASE_DEBUG=ON -DCGAL_DISABLE_GMP=ON -DCMAKE_BUILD_TYPE="Release" -G "Unix Makefiles" && make

# Step 7: Build the wood code
cmake -B . -S .. -DGET_LIBS=OFF -DBUILD_MY_PROJECTS=ON -DCOMPILE_LIBS=OFF -DRELEASE_DEBUG=ON -DCGAL_DISABLE_GMP=ON -DCMAKE_BUILD_TYPE="Release" -G "Unix Makefiles" && make

# Step 8: Build the wood code after making changes (e.g., in main.cpp)
make VERBOSE=1 && ../build/wood

echo "BAT install_linux end!"
