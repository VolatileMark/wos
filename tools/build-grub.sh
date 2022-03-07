#!/bin/bash

FTP_URL="https://ftp.gnu.org/gnu"

GRUB_VERSION="2.06"

WORKDIR="/tmp/osworkdir"
BUILD_DIR="build-grub"

TARGET="x86_64"
PREFIX_BASE=$(realpath "./$TARGET-grub2")
PREFIX_EFI="$PREFIX_BASE/efi"
PREFIX_PC="$PREFIX_BASE/pc"

echo "Creating working directory..."
mkdir -p "$WORKDIR" && cd "$WORKDIR"

echo "Creating install directories..."
mkdir -p "$PREFIX_EFI"
mkdir -p "$PREFIX_PC"

echo "Downloading grub-$GRUB_VERSION..."
curl -O "$FTP_URL/grub/grub-$GRUB_VERSION.tar.gz"
tar -xf "grub-$GRUB_VERSION.tar.gz"

echo "Building grub-$GRUB_VERSION-efi..."
mkdir -p "$BUILD_DIR-efi" && cd "$BUILD_DIR-efi"
sh -c "../grub-$GRUB_VERSION/configure --prefix=\"$PREFIX_EFI\" --target=$TARGET --with-platform=efi"
make -j$(nproc)
make install
cd "$PREFIX_EFI"
sh -c "bin/grub-mkfont -o share/grub/unicode.pf2 /usr/share/fonts/truetype/unifont/unifont.ttf"

cd "$WORKDIR"
echo "Building grub-$GRUB_VERSION-pc..."
mkdir -p "$BUILD_DIR-pc" && cd "$BUILD_DIR-pc"
sh -c "../grub-$GRUB_VERSION/configure --prefix=\"$PREFIX_PC\" --target=$TARGET --with-platform=pc"
make -j$(nproc)
make install
cd "$PREFIX_PC"
sh -c "bin/grub-mkfont -o share/grub/unicode.pf2 /usr/share/fonts/truetype/unifont/unifont.ttf"

rm -rf "$WORKDIR"

echo
echo "Script finished executing. Be sure to check the output for errors."