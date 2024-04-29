FROM ubuntu:22.04

ARG DEBIAN_FRONTEND=noninteractive
ENV LLVM_DIR /usr/lib/llvm-17/

# Install needed base packages
RUN apt-get update && apt-get install -y    \
    git cmake ninja-build build-essential   \
    python3-minimal python3-pip             \
    wget software-properties-common

# Setup LLVM repos for ubuntu
RUN echo deb http://apt.llvm.org/jammy/ llvm-toolchain-jammy-17 main     \
    | tee -a /etc/apt/sources.list
RUN echo deb-src http://apt.llvm.org/focal/ llvm-toolchain-focal-17 main \
    | tee -a /etc/apt/sources.list
RUN wget -O llvm-snapshot.gpg.key https://apt.llvm.org/llvm-snapshot.gpg.key
RUN apt-key add llvm-snapshot.gpg.key

# Install LLVM
RUN apt-get update -y && apt-get upgrade -y && apt-get install -y              \
    libllvm17 llvm-17 llvm-17-dev llvm-17-doc llvm-17-examples llvm-17-runtime \
    libedit-dev libzstd-dev libcurl4-gnutls-dev libstdc++-12-dev clang-17

WORKDIR /usr/src/app
CMD ["/bin/bash"]