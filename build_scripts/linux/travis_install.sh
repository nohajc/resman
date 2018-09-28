#!/bin/bash

echo 'deb http://apt.llvm.org/trusty/ llvm-toolchain-trusty-6.0 main' | sudo tee -a /etc/apt/sources.list
echo 'deb http://ppa.launchpad.net/ubuntu-toolchain-r/test/ubuntu trusty main' | sudo tee -a /etc/apt/sources.list

sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys AF4F7421
sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys BA9EF27F

sudo apt update
sudo apt install software-properties-common -y
sudo add-apt-repository -y ppa:george-edison55/cmake-3.x
sudo apt update
