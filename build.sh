#!/bin/bash

set -e

rm -rf build
mkdir build
cd build

echo ""
echo "*************"
echo "cmake .."
echo ""
cmake ..

echo ""
echo "*************"
echo "make -j"
echo ""
make -j

echo ""
echo "*************"
echo "altus-tracker"
echo ""
# ./altus-tracker
./altus-tracker --file test --throttle
