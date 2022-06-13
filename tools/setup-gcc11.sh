#!/bin/bash

GIT_URL="https://github.com/VolatileMark"
REPO="gcc11-rpms"
WORKDIR=$(realpath ./tmp)

mkdir -p $WORKDIR && cd $WORKDIR
rm -rf $REPO
git clone "$GIT_URL/$REPO.git"
cd $REPO

for file in ./*.rpm; do
    rpm2cpio $file | cpio -idmv
done

mv -f ./usr ../../gcc11

cd ..
rm -rf $WORKDIR
