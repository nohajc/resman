#!/bin/bash

mkdir -p build
chmod 777 build

DOCKER_COMMAND="cd /home/dev/resman/build && cmake -DCMAKE_BUILD_TYPE=Release -DLINKING=static ../rescomp && make -j4"

if [ -n "$TRAVIS_TAG" ]; then
   DOCKER_COMMAND="$DOCKER_COMMAND && make package"
fi

docker run --rm -v $PWD:/home/dev/resman -it nohajc/void-llvm-clang-dev /bin/bash -c "$DOCKER_COMMAND"
