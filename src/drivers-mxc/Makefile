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

obj-$(CONFIG_DRIVER_MXC_SERIAL)	+= drv-mxc-serial.o

ifneq ($(CONFIG_THUMB),)
  CFLAGS += -mthumb
endif
