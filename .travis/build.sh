#!/bin/sh
set -e
set -x

cmake -G "Unix Makefiles"
make clean && make
