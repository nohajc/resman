#!/bin/bash

mkdir build && cd build
CC=gcc-8 CXX=g++-8 cmake ../rescomp
make -j4
