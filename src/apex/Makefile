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

obj-y := init.o services.o console.o console-printf.o
obj-y += command.o cmd-version.o cmd-help.o cmd-boot.o

obj-$(CONFIG_COMMAND_HISTORY)   += command-history.o

obj-$(CONFIG_ENV)		+= env.o

obj-$(CONFIG_CMD_CHECKSUM)	+= cmd-checksum.o
obj-$(CONFIG_CMD_COMPARE)	+= cmd-compare.o
obj-$(CONFIG_CMD_COPY)		+= cmd-copy.o
obj-$(CONFIG_CMD_DRVINFO)	+= cmd-drvinfo.o
obj-$(CONFIG_CMD_DUMP)		+= cmd-dump.o
obj-$(CONFIG_CMD_ENV)		+= cmd-env.o
obj-$(CONFIG_CMD_ERASE)		+= cmd-erase.o
obj-$(CONFIG_CMD_FILL)		+= cmd-fill.o
obj-$(CONFIG_CMD_GO)		+= cmd-go.o
obj-$(CONFIG_CMD_INFO)		+= cmd-info.o
obj-$(CONFIG_CMD_PAUSE)		+= cmd-pause.o
obj-$(CONFIG_CMD_WAIT)		+= cmd-wait.o
obj-$(CONFIG_CMD_XRECEIVE)	+= cmd-xreceive.o
obj-$(CONFIG_CMD_ALIAS)		+= cmd-alias.o

ifneq ($(CONFIG_THUMB),)
# CFLAGS += -mthumb
endif

CFLAGS_cmd-version.o += -DAPEXRELEASE=\"$(APEXRELEASE)\"
CFLAGS_env.o	     += -DAPEXRELEASE=\"$(APEXRELEASE)\"
