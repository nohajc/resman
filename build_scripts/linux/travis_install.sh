#!/bin/bash

docker pull nohajc/void-llvm-clang-dev

#echo 'deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-6.0 main' | sudo tee -a /etc/apt/sources.list
#echo 'deb http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu trusty main' | sudo tee -a /etc/apt/sources.list
#echo 'deb http://ppa.launchpad.net/george-edison55/cmake-3.x/ubuntu trusty main' | sudo tee -a /etc/apt/sources.list
#
#sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys AF4F7421
#sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys BA9EF27F
#sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys 828AB726
#
#sudo apt update -qq
#
#sudo apt install zlib1g-dev cmake3 gcc-8 g++-8 llvm-6.0-dev libclang-6.0-dev -y
