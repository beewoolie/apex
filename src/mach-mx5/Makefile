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

obj-y += initialize.o timer.o # env.o cpuinfo.o cmd-reset.o cmd-sleep.o
obj-y += header.o
#obj-$(CONFIG_EXCEPTIONS)        += exception_vectors.o
#obj-$(CONFIG_DRIVER_ONENAND)	+= cmd-onenand.o
#obj-$(CONFIG_IPU_TEST)		+= ipu.o
obj-$(CONFIG_MX51_UART1)	+= serial-mx51.o
