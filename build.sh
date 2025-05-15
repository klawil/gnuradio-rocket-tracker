#!/bin/bash

set -e

rm -rf build
mkdir build
cd build
cmake ..
make -j
echo ""
echo "*************"
echo ""
./altus-tracker -h
./altus-tracker -v
./altus-tracker --file=test
