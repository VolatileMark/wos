#!/bin/bash

#DEPS="bear build-essential binutils mtools gdb rpm2cpio bison flex libgmp3-dev libmpc-dev libmpfr-dev texinfo gcc-multilib nasm xorriso unifont libfreetype-dev libglib2.0-dev libfdt-dev libpixman-1-dev zlib1g-dev libsdl1.2-dev libgtk-3-dev libvte-dev libnfs-dev libiscsi-dev"
DEPS="
    bear gcc g++ binutils gdb mtools
    bison flex texinfo nasm xorriso
    unifont-fonts make automake bzip2
    autoconf git ninja-build python3
    glib2-devel freetype-devel 
    libfdt-devel pixman-devel zlib-devel 
    sdl12-compat-devel gtk3-devel
    vte-devel libnfs-devel libiscsi-devel
    libaio-devel libcap-ng-devel 
    libiscsi-devel capstone-devel
"

if [[ $EUID -ne 0 ]]; then
   echo "This script must be run as root" 
   exit 1
fi

echo "Updating repositories..."
dnf upgrade -y > /dev/null
dnf install $1 $DEPS
