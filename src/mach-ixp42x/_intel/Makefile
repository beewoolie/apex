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

#CROSS_COMPILE ?= /usr/armv5b-softfloat-linux/gcc-3.4.2-glibc-2.3.3/bin/armv5b-softfloat-linux-

all:
	make -C src/mach-ixp42x/intel/ixp400_xscale_sw \
	  LINUX_CROSS_COMPILE=$(CROSS_COMPILE)\
	  IX_TARGET=linuxbe\
	  IX_XSCALE_SW=`pwd`/src/mach-ixp42x/intel/ixp400_xscale_sw\
	  linuxbe_KERNEL_DIR=/home/elf/z/embedded/linux

#lib-$(CONFIG_NPE) += IxNpeMicrocode.o
