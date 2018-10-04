#!/bin/bash

mkdir -p build
chmod 777 build

DOCKER_COMMAND="cd /home/dev/resman/build && cmake -DCMAKE_BUILD_TYPE=Release -DLINKING=static ../rescomp && make -j4"

docker run --rm -v $PWD:/home/dev/resman -it "nohajc/void-llvm-clang-dev:v6.0.1" /bin/bash -c "$DOCKER_COMMAND"
