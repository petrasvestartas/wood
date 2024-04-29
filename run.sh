#!/bin/bash
echo "BASH install_ubuntu start!"

# Set the current directory to where the script is located
cd "$(dirname "$0")"

# Clone the repository only if it doesn't already exist
if [ ! -d "../wood" ]; then
    git clone --depth 1 https://github.com/petrasvestartas/wood.git
fi

# Navigate to the cmake directory and create a build directory
cd cmake/build

# Build the wood code after making changes (e.g., in main.cpp)
cd "$(dirname "$0")"/cmake/build
cmake -B . -S .. -DGET_LIBS=OFF -DBUILD_MY_PROJECTS=ON -DCOMPILE_LIBS=OFF -DRELEASE_DEBUG=ON -DCMAKE_BUILD_TYPE="Release" -G "Unix Makefiles" && make -j$(sysctl -n hw.ncpu)
make VERBOSE=1 && ../build/wood
cd ../..

echo "BASH install_ubuntu end!"
