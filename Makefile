#
# Makefile for APEX loader
#

ARCH=arm
CROSS_COMPILE=/usr/arm-linux/gcc-3.3-glibc-2.3.2/bin/arm-linux-

# ===========================================================================

# Make variables (CC, etc...)

AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump
AWK		= awk
GENKSYMS	= scripts/genksyms/genksyms
DEPMOD		= /sbin/depmod
KALLSYMS	= scripts/kallsyms
PERL		= perl
CHECK		= sparse
CHECKFLAGS     :=
MODFLAGS	= -DMODULE
CFLAGS_MODULE   = $(MODFLAGS)
AFLAGS_MODULE   = $(MODFLAGS)
LDFLAGS_MODULE  = -r
CFLAGS_KERNEL	=
AFLAGS_KERNEL	=

NOSTDINC_FLAGS  = -nostdinc -iwithprefix include

srctree := $(CURDIR)

export srctree objtree VPATH TOPDIR
export BUILD_VERBOSE

export	VERSION PATCHLEVEL SUBLEVEL EXTRAVERSION LOCALVERSION KERNELRELEASE \
	ARCH CONFIG_SHELL HOSTCC HOSTCFLAGS CROSS_COMPILE AS LD CC \
	CPP AR NM STRIP OBJCOPY OBJDUMP MAKE AWK GENKSYMS PERL UTS_MACHINE \
	HOSTCXX HOSTCXXFLAGS LDFLAGS_MODULE CHECK CHECKFLAGS

export CPPFLAGS NOSTDINC_FLAGS LINUXINCLUDE OBJCOPYFLAGS LDFLAGS
export CFLAGS CFLAGS_KERNEL CFLAGS_MODULE 
export AFLAGS AFLAGS_KERNEL AFLAGS_MODULE

-include config

include $(srctree)/scripts/Makefile.lib

#include $(srctree)/src/arch-$(ARCH)/Makefile

.PHONY: all
all:
	$(Q)$(MAKE) $(build)=src/arch-arm
