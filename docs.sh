#!/bin/bash
if ! command -v doxygen &> /dev/null
then
    echo "Doxygen could not be found, installing..."
    sudo apt-get update
    sudo apt-get install doxygen
fi
cd docs
doxygen Doxyfile
sudo chown -R petras:petras output
sudo -u petras xdg-open output/index.html