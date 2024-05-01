#!/bin/bash

echo "BAT install_mac start!"

# Ensure the script is run with superuser privileges for the package installations
# /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
# brew install cmake

if [ "$(id -u)" != "0" ]; then
   echo "Please run this script with superuser privileges (sudo)."
   exit 1
fi

# Install required libraries using Homebrew
echo "run these commands first:"
echo "brew update"
echo "brew install gmp mpfr"

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
cmake -B . -S .. -DGET_LIBS=ON -DCOMPILE_LIBS=OFF -DBUILD_MY_PROJECTS=OFF -DRELEASE_DEBUG=ON -DCMAKE_BUILD_TYPE="Release" -G "Unix Makefiles" && make

# Step 6: Build 3rd-party libraries
cmake -B . -S .. -DGET_LIBS=OFF -DBUILD_MY_PROJECTS=ON -DCOMPILE_LIBS=ON -DRELEASE_DEBUG=ON -DCMAKE_BUILD_TYPE="Release" -G "Unix Makefiles" && make

# Step 7: Build the wood code
cmake -B . -S .. -DGET_LIBS=OFF -DBUILD_MY_PROJECTS=ON -DCOMPILE_LIBS=OFF -DRELEASE_DEBUG=ON -DCMAKE_BUILD_TYPE="Release" -G "Unix Makefiles" && make

# Step 8: Build the wood code after making changes (e.g., in main.cpp)
make VERBOSE=1 && ../build/wood

echo "BAT install_mac end!"
