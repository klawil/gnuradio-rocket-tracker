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
# ./altus-tracker -h
# ./altus-tracker -v
time ./altus-tracker --file=test
# time ./altus-tracker --file=test
# time ./altus-tracker --file=test
# time ./altus-tracker --file=test
# time ./altus-tracker --file=test

# time ./altus-tracker --custom --file=test
# time ./altus-tracker --custom --file=test
# time ./altus-tracker --custom --file=test
# time ./altus-tracker --custom --file=test
# time ./altus-tracker --custom --file=test
