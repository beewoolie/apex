#!/bin/sh
export CROSS_COMPILE=/usr/armv5b-softfloat-linux/gcc-3.4.2-glibc-2.3.3/bin/armv5b-softfloat-linux-

export LINUX_CROSS_COMPILE=$CROSS_COMPILE
export IX_TARGET=linuxbe
export IX_XSCALE_SW=`pwd`/ixp400_xscale_sw
export linuxbe_KERNEL_DIR=`pwd`/linux-2.6.11
