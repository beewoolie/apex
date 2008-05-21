#
# Makefile for APEX loader
#
#   Copyright (C) 2004 Marc Singer
#
#   This program is licensed under the terms of the GNU General Public
#   License as published by the Free Software Foundation.  Please refer
#   to the file debian/copyright for further details.
#

lib-y := vsprintf.o ctype.o
lib-y += strlen.o strnlen.o strchr.o strlcpy.o strcat.o strcpy.o
lib-y += strcmp.o strnicmp.o strcspn.o
lib-y += memcmp.o memset.o
lib-$(CONFIG_SMALL) += memcpy.o memmove.o
lib-y += crc32.o xmodem.o
lib-y += spinner.o
lib-$(CONFIG_ENV) += env.o
lib-y += dump.o
lib-y += zlib.o zlib-heap.o
#lib-y += png.o
lib-y += pngr.o
lib-y += sort.o
lib-$(CONFIG_ALIASES) += alias.o
lib-y += lookup.o
lib-y += memtest.o

ifneq ($(CONFIG_THUMB),)
CFLAGS_vsprintf.o	+= -mthumb
CFLAGS_ctype.o		+= -mthumb
#CFLAGS_xmodem.o		+= -mthumb -mcaller-super-interworking
CFLAGS_strlen.o		+= -mthumb
CFLAGS_strnlen.o	+= -mthumb
CFLAGS_strrchr.o	+= -mthumb
CFLAGS_strlcpy.o	+= -mthumb
CFLAGS_strnicmp.o	+= -mthumb
CFLAGS_strcpy.o		+= -mthumb
CFLAGS_memcmp.o		+= -mthumb
CFLAGS_memset.o		+= -mthumb
CFLAGS_memcpy.o		+= -mthumb
CFLAGS_memmove.o	+= -mthumb
CFLAGS_png.o		+= -mthumb
endif

#lib-y += div64.o
#lib-y += udiv.o
#CFLAGS_vsprintf.o += -mthumb
#CFLAGS_crc32.o = -mthumb -mthumb-interwork