#
# Makefile for APEX loader
#
#   Copyright (C) 2004 Marc Singer
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License
#   version 2 as published by the Free Software Foundation.
#   Please refer to the file debian/copyright for further details.
#

-include $(wildcard config)

H=`pwd`/src/mach-ixp42x/_intel

.PHONY: all build unpack patch quilt prep
all: unpack patch build

build:
	make -C $H/ixp_osal \
	  LINUX_CROSS_COMPILE=$(CROSS_COMPILE)\
	  IX_TARGET=linuxbe\
	  IX_XSCALE_SW=$H/ixp400_xscale_sw\
	  linuxbe_KERNEL_DIR=$H/linux\
	  libosal
	make -C $H/ixp400_xscale_sw \
	  LINUX_CROSS_COMPILE=$(CROSS_COMPILE)\
	  IX_TARGET=linuxbe\
	  IX_XSCALE_SW=$H/ixp400_xscale_sw\
	  linuxbe_KERNEL_DIR=$H/linux\
	  lib-all

#	  linuxbe_KERNEL_DIR=$(HOME)/z/embedded/linux

#lib-$(CONFIG_NPE) += IxNpeMicrocode.o

unpack:
	-rm -rf $H/ixp400_xscale_sw $H/ixp_osal $H/patches $H/.pc
	(cd $H; unzip ixp400AccessLibrary-1_5.zip)
	(cd $H; unzip ixp400NpeLibrary-1_5.zip)

patch:
	(cd $H; patch -p1 < ixp400_xscale_1.5_linux26.patch)

quilt:
	(cd $H; quilt import ixp400_xscale_1.5_linux26.patch)
	(cd $H; quilt push   ixp400_xscale_1.5_linux26.patch)

prep:
	[ -L $H/lib ] || ln -s $H/ixp400_xscale_sw/lib/linuxbe $H/lib
