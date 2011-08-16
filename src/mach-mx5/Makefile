#
# Makefile for APEX loader
#
#   Copyright (C) 2011 Marc Singer
#
#   This program is free software; you can redistribute it and/or
#   modify it under the terms of the GNU General Public License
#   version 2 as published by the Free Software Foundation.
#   Please refer to the file debian/copyright for further details.
#

obj-y += initialize.o timer.o cpuinfo.o cmd-reset.o env.o
obj-y += header.o
obj-y += drv-mx5-spi-pmic.o
obj-y += drv-mx5-spi-flash.o
obj-y += drv-mx5-ecspi.o
obj-y += drv-mx5-esdhc.o
obj-y += clocks.o
obj-y += atag-revision.o
obj-y += atag-serialnr.o

obj-$(CONFIG_MX5_UART1)		+= serial-mx5.o
obj-$(CONFIG_EXCEPTIONS)        += exception_vectors.o
#obj-$(CONFIG_DRIVER_ONENAND)	+= cmd-onenand.o
#obj-$(CONFIG_IPU_TEST)		+= ipu.o
