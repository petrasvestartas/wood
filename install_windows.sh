#!/usr/bin/env bash
set -e
echo "install_windows.sh start!"
cd "$(dirname "$0")"

# Clone wood repo if absent
if [ ! -d "../wood" ]; then
    git clone https://github.com/petrasvestartas/wood.git
fi

# Step 4: Get session_cpp
if [ ! -d "cmake/ext/session_cpp" ]; then
    mkdir -p cmake/ext
    git clone https://github.com/petrasvestartas/session_cpp.git cmake/ext/session_cpp
fi

mkdir -p cmake/build
cd cmake/build

# Step 5: Download 3rd-party libraries
cmake --fresh -DGET_LIBS=ON -DCOMPILE_LIBS=OFF -DBUILD_MY_PROJECTS=OFF -DRELEASE_DEBUG=ON \
  -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64 \
  -DCGAL_CMAKE_EXACT_NT_BACKEND=BOOST_BACKEND -DCGAL_DISABLE_GMP=ON \
  -DCMAKE_DISABLE_FIND_PACKAGE_GMP=ON .. && cmake --build . --config Release

# Step 6: Compile 3rd-party libraries
cmake --fresh -DGET_LIBS=OFF -DBUILD_MY_PROJECTS=ON -DCOMPILE_LIBS=ON -DRELEASE_DEBUG=ON \
  -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64 \
  -DCGAL_CMAKE_EXACT_NT_BACKEND=BOOST_BACKEND -DCGAL_DISABLE_GMP=ON \
  -DCMAKE_DISABLE_FIND_PACKAGE_GMP=ON .. && cmake --build . --config Release

# Step 7: Build wood + session_cpp
cmake --fresh -DGET_LIBS=OFF -DBUILD_MY_PROJECTS=ON -DCOMPILE_LIBS=OFF -DRELEASE_DEBUG=ON \
  -DCMAKE_BUILD_TYPE=Release -G "Visual Studio 17 2022" -A x64 \
  -DCGAL_CMAKE_EXACT_NT_BACKEND=BOOST_BACKEND -DCGAL_DISABLE_GMP=ON \
  -DCMAKE_DISABLE_FIND_PACKAGE_GMP=ON .. && cmake --build . --config Release

# Step 8: Run
cmake --build . -v --config Release --parallel 8 && ../build/Release/wood.exe
cd ../..
echo "install_windows.sh end!"
