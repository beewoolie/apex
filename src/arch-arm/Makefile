#
# Makefile for APEX loader
#

LDFLAGS_apex := -p --no-undefined -X
OBJCOPYFLAGS := -O binary -R .note -R .comment -R .bss
GZFLAGS      := -9

CFLAGS_entry.o += -Os

core-y += src/mach-lh7952x/ src/arch-arm/entry/ 
libs-y += src/arch-arm/lib/

rom := src/arch-arm/rom

apex.bin apex.srec : apex
	$(Q)$(MAKE) $(build)=$(rom) $(rom)/$@
