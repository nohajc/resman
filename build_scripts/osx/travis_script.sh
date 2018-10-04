#!/bin/bash

mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=/tmp/clang-llvm-libs ../rescomp
make -j4
