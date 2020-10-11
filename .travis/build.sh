#!/bin/sh
set -e
set -x

mkdir build
cd build
cmake ../ && make clean && make
