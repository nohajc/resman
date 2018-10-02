#!/bin/bash

mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH=/tmp/clang-llvm-libs ../rescomp
make -j4
make package
