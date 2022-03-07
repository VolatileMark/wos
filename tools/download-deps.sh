#!/bin/bash

DEPS="bear build-essential binutils mtools gdb rpm2cpio bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo gcc-multilib nasm xorriso unifont libfreetype-dev libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev libsdl1.2-dev libgtk-3-dev libvte-dev libnfs-dev libiscsi-dev"

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

if ! test "$1" = "-y";
then
    echo "This script will install the following packages:"
    echo "$DEPS"
    read -p "Do you want to proceed? [Y/N] > " -n 1 -r
    echo
    if [[ ! $REPLY =~ ^[Yy]$ ]]
    then
        exit 1
    fi
fi

echo "Updating repositories..."
apt-get update > /dev/null
apt-get install -y $1 $DEPS