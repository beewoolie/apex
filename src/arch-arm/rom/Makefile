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

targets += apex.bin apex.srec

OBJCOPYFLAGS_apex.bin := -Rethernet.bss

$(obj)/apex.bin: apex FORCE
	$(call if_changed,objcopy)

# *** FIXME: there may need to be an adjustment to the start address 
OBJCOPYFLAGS_apex.srec := -O srec
$(obj)/apex.srec: apex FORCE
	$(call if_changed,objcopy)

