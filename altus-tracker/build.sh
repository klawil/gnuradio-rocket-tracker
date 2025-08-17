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
# ./altus-tracker --file test
# ./altus-tracker --file ../data.cfile
# ./altus-tracker

# ./altus-tracker --file ../data_2.5MHz_435.8Mcenter_436.55Maltus.cfile --center_freq 435800000 --sample_rate 2500000
./altus-tracker --file ../data_2.5MHz_435.975Mcenter_436.55Maltus.cfile --center_freq 435975000 --sample_rate 2500000
