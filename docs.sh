#!/bin/bash
if ! command -v doxygen &> /dev/null
then
    echo "Doxygen could not be found, installing..."
    sudo apt-get update
    sudo apt-get install doxygen
fi
cd docs
doxygen Doxyfile
cd ..
xdg-open output/index.html