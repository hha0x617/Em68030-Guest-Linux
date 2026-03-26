FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y \
    gcc-m68k-linux-gnu \
    binutils-m68k-linux-gnu \
    make \
    bc \
    flex \
    bison \
    libelf-dev \
    libssl-dev \
    curl \
    xz-utils \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
