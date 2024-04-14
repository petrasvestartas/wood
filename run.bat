@echo off
echo BAT install_windows start!

REM Set the current directory to where the batch file is located
cd /d %~dp0

REM Clone the repository only if it doesn't already exist
if not exist ..\wood git clone --depth 1 https://github.com/petrasvestartas/wood.git

REM Navigate to the cmake directory and create a build directory
cd cmake\
cd build\

REM Step 8: Build the wood code after making changes (e.g., in main.cpp)
cd /d %~dp0\cmake\build
cmake --build . -v --config Release --parallel 8 && ..\build\Release\wood.exe
cd ..\..

echo BAT install_windows end!
