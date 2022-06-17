#!/bin/bash

WORKDIR=$(realpath ./tmp)
GCC_VERSION=${GCC_VERSION:-11.1.0}
KERNELORG_REPO="https://mirrors.edge.kernel.org/pub/tools/crosstool/files/bin/x86_64/$GCC_VERSION/"

mkdir -p $WORKDIR && cd $WORKDIR

curl -O "$KERNELORG_REPO/x86_64-gcc-$GCC_VERSION-nolibc-x86_64-linux.tar.gz"
tar -xf "x86_64-gcc-$GCC_VERSION-nolibc-x86_64-linux.tar.gz"
cd "gcc-$GCC_VERSION-nolibc/x86_64-linux/bin"
for file in x86_64-linux-*; do
    ln -s "$file" "${file#x86_64-linux-}"
done
cd ../../
mv "x86_64-linux" "../../grub-gcc"

cd ..
rm -rf $WORKDIR
