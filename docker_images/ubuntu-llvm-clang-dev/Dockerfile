FROM ubuntu:18.04

RUN apt-get -y update && apt-get -y install build-essential cmake llvm-6.0-dev libclang-6.0-dev zlib1g-dev
RUN useradd -m -p dev dev

USER dev
