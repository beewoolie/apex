#
# Makefile for APEX loader
#

obj-y := entry.o

CPPFLAGS_apex.lds += -DAPEX_VMA=$(CONFIG_APEX_VMA)

CFLAGS_entry.o += -Os

extra-y=apex.lds

