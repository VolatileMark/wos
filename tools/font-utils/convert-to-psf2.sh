#!/bin/bash

./readpsf $1 bmp
BMP=$(ls | grep .bmp)
./writepsf $BMP font.psf
rm -rf *.bmp
