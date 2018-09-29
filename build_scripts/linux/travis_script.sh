#!/bin/bash

docker run --rm -v $PWD:/home/dev/resman -it nohajc/void-llvm-clang-dev /bin/bash -c "cd /home/dev/resman && mkdir -p build && cd build && cmake -DLINKING=static ../rescomp && make -j4"

#mkdir build && cd build
#CC=gcc-8 CXX=g++-8 cmake ../rescomp
#make -j4
