#
# Makefile for APEX loader
#
#   Copyright (C) 2004 Marc Singer
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License as
#   published by the Free Software Foundation; either version 2 of the
#   License, or (at your option) any later version.
#
#   This program is distributed in the hope that it will be useful, but
#   WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#   General Public License for more details.
#
#   You should have received a copy of the GNU General Public License
#   along with this program; if not, write to the Free Software
#   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
#   USA.
#

HERE=src/mach-ixp42x/_intel

export CROSS_COMPILE=/usr/armv5b-softfloat-linux/gcc-3.4.2-glibc-2.3.3/bin/armv5b-softfloat-linux-

#CROSS_COMPILE ?= /usr/armv5b-softfloat-linux/gcc-3.4.2-glibc-2.3.3/bin/armv5b-softfloat-linux-

all:
	make -C $HERE/ixp400_xscale_sw \
	  LINUX_CROSS_COMPILE=$(CROSS_COMPILE)\
	  IX_TARGET=linuxbe\
	  IX_XSCALE_SW=`pwd`/$HERE/ixp400_xscale_sw\
	  linuxbe_KERNEL_DIR=`pwd`/$HERE/linux

#	  linuxbe_KERNEL_DIR=$(HOME)/z/embedded/linux

#lib-$(CONFIG_NPE) += IxNpeMicrocode.o

unpack:
	-rm -rf $HERE/ixp400_xscale_sw $HERE/ixp_osal
	(cd $HERE; unzip ixp400AccessLibrary-1_5.zip)
	(cd $HERE; unzip ixp400NpeLibrary-1_5.zip)
	(cd $HERE; patch -p1 ../ixp400_xscale_1.5_linux26.patch)

prep:
	ln -s src/mach-ixp42x/_intel/ixp400_xscale_sw/lib/linuxbe \
	      src/mach-ixp42x/_intel/lib
