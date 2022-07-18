#!/bin/bash

FTP_URL="https://download.qemu.org/"

CORES=${CORES:-$(nproc)}
QEMU_VERSION="7.0.0"

WORKDIR=$(realpath "./tmp")
BUILD_DIR="build-qemu"

TARGET="x86_64"
PREFIX=$(realpath "./$TARGET-qemu")

echo "Creating working directory..."
mkdir -p "$WORKDIR" && cd "$WORKDIR"

echo "Creating install directories..."
mkdir -p "$PREFIX"

echo "Downloading qemu-$QEMU_VERSION..."
curl -O "$FTP_URL/qemu-$QEMU_VERSION.tar.xz"
tar -xJf "qemu-$QEMU_VERSION.tar.xz"

echo "Building qemu-$QEMU_VERSION..."
mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"
sh -c "../qemu-$QEMU_VERSION/configure --target-list=\"$TARGET-softmmu\" --prefix=\"$PREFIX\""
make -j$CORES
make install

rm -rf "$WORKDIR"

echo
echo "Script finished executing. Be sure to check the output for errors."
