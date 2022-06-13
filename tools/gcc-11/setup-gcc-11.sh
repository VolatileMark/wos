#!/bin/bash

for file in ./*.rpm; do
    rpm2cpio $file | cpio -idmv
done