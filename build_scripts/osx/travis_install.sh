#!/bin/bash

cd /tmp
wget http://releases.llvm.org/6.0.0/clang+llvm-6.0.0-x86_64-apple-darwin.tar.xz
tar xf clang+llvm-6.0.0-x86_64-apple-darwin.tar.xz
mv clang+llvm-6.0.0-x86_64-apple-darwin clang-llvm-libs
cd -
