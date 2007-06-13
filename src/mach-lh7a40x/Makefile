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

obj-y := arch.o initialize.o serial.o timer.o env.o cpuinfo.o
obj-y += cmd-spi.o
obj-$(CONFIG_CLCDC)		+= clcdc.o
obj-$(CONFIG_CODEC)		+= codec.o
obj-$(CONFIG_ADC)		+= adc.o
obj-y				+= cmd-reset.o
obj-y				+= cmd-standby.o
obj-$(CONFIG_CMD_ARM920_TEST)	+= cmd-cp15test.o
obj-$(CONFIG_RELOCATE_COMPANION_EVT1) += relocate-companion.o
obj-$(CONFIG_MACH_COMPANION)    += clcdc-companion.o
obj-$(CONFIG_RELOCATE_COMPANION_EVT2) += preinitialization-companion.o

obj-$(CONFIG_MACH_COMPANION)	+= test-companion.o
obj-x$(CONFIG_MACH_COMPANION)$(CONFIG_VARIATIONS) += variation-companion.o

obj-y += $(obj-xyy)
