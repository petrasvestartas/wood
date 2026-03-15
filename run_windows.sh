#!/usr/bin/env bash
set -e
echo "run_windows.sh start!"
cd "$(dirname "$0")/cmake/build"
cmake --build . -v --config Release --parallel 8 && ../build/Release/wood.exe
echo "run_windows.sh end!"
