#
# Makefile for APEX loader
#

LDFLAGS_apex := -p --no-undefined -X -g 
OBJCOPYFLAGS := -O binary -R .note -R .comment -R .bss
GZFLAGS      := -9

CFLAGS_entry.o += -Os

init-y += src/arch-arm/entry/ 
core-y += src/mach-lh7952x/ src/drivers/ src/arch-arm/linux/
libs-y += src/arch-arm/lib/

rom := src/arch-arm/rom

apex.bin apex.srec : apex
	$(Q)$(MAKE) $(build)=$(rom) $(rom)/$@
