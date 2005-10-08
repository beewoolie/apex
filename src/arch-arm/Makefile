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

LDFLAGS_apex := -p --no-undefined -X -g
OBJCOPYFLAGS := -O binary -R .note -R .comment -R .bss
GZFLAGS      := -9

# *** This lets the clean target work, but I'm not sure it's a good
# idea to include it.
-include .config
quote:="
# "
CONFIG_MACH:= $(subst $(quote),,$(CONFIG_MACH))

arch-$(CONFIG_CPU_ARMV4)   :=-D__ARM_ARCH__=4 -march=armv4t
arch-$(CONFIG_CPU_ARMV5)   :=-D__ARM_ARCH__=5 $(call cc-option,-march=armv5te,-march=armv4)

tune-$(CONFIG_CPU_ARM720T) :=-mtune=arm7tdmi
tune-$(CONFIG_CPU_ARM920T) :=-mtune=arm9tdmi
tune-$(CONFIG_CPU_ARM922T) :=-mtune=arm9tdmi
tune-$(CONFIG_CPU_ARM925T) :=-mtune=arm9tdmi
tune-$(CONFIG_CPU_ARM926T) :=-mtune=arm9tdmi
tune-$(CONFIG_CPU_XSCALE)  :=$(call cc-option,-mtune=xscale,-mtune=strongarm110) -Wa,-mcpu=xscale

CFLAGS+= $(arch-y) $(tune-y)

ifneq ($(CONFIG_THUMB),)
CFLAGS+= -mthumb-interwork
AFLAGS+= -mthumb-interwork
endif

init-y += src/arch-arm/entry/ 
core-y += src/mach-$(CONFIG_MACH)/ 
core-y += src/drivers/ src/net/ src/arch-arm/linux/
core-$(CONFIG_DRIVERS_LH) +=  src/drivers-lh/
libs-y += src/arch-arm/lib/

-include src/mach-$(CONFIG_MACH)/Makefile.libs

rom := src/arch-arm/rom

apex.bin apex.srec : include/config.h apex
	$(Q)$(MAKE) $(build)=$(rom) $(rom)/$@
