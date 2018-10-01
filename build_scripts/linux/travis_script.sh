#!/bin/bash

mkdir -p build
chmod 777 build
docker run --rm -v $PWD:/home/dev/resman -it nohajc/void-llvm-clang-dev /bin/bash -c "cd /home/dev/resman/build && cmake -DLINKING=static ../rescomp && make -j4 && make package"
