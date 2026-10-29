FROM ubuntu:24.04

RUN apt-get update \
    && apt-get install -y --no-install-recommends \
       build-essential gdb qemu-user-static \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace