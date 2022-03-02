#!/bin/bash

FTP_URL="https://ftp.gnu.org/gnu"

GRUB_VERSION="2.06"

WORKDIR="/tmp/osworkdir"
BUILD_DIR="build"

TARGET="x86_64"
PREFIX=$(realpath "./$TARGET-grub2-efi")

echo "Creating working directory..."
mkdir -p "$WORKDIR" && cd "$WORKDIR"

echo "Creating install directory..."
mkdir -p "$PREFIX"

echo "Downloading grub-$GRUB_VERSION..."
curl -O "$FTP_URL/grub/grub-$GRUB_VERSION.tar.gz"
tar -xf "grub-$GRUB_VERSION.tar.gz"
echo "Building grub-$GRUB_VERSION..."
mkdir -p "$BUILD_DIR-grub" && cd "$BUILD_DIR-grub"
sh -c "../grub-$GRUB_VERSION/configure --prefix=\"$PREFIX\" --target=$TARGET --with-platform=efi"
make -j$(nproc)
make install

rm -rf "$WORKDIR"

echo
echo "Script finished executing. Be sure to check the output for errors."