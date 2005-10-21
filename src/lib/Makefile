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

lib-y := vsprintf.o ctype.o
lib-y += strlen.o strnlen.o strchr.o strlcpy.o strcat.o strcpy.o 
lib-y += strcmp.o strnicmp.o strcspn.o
lib-y += memcmp.o memset.o
lib-$(CONFIG_SMALL) += memcpy.o memmove.o
lib-y += crc32.o xmodem.o
lib-y += env.o spinner.o
lib-y += dump.o
lib-y += zlib.o zlib-heap.o
lib-y += png.o
lib-y += sort.o
lib-y += alias.o

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