@echo off
cd docs
doxygen Doxyfile
cd ..
start docs\output\index.html
