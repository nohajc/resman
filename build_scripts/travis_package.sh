#!/bin/bash

if [ -n "$TRAVIS_TAG" ]; then
   mkdir -p dist
   cp include/resman.h dist
   cp build/rescomp dist
   cd dist
   TAR_NAME="resman-${TRAVIS_TAG}-${TRAVIS_OS_NAME}-amd64.tar.xz"
   tar cvJf $TAR_NAME *
   cd ..
   mv dist/$TAR_NAME .
   rm -r dist
fi
