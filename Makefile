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
CHECK		= echo sparse
CHECKFLAGS     :=
MODFLAGS	= -DMODULE
CFLAGS_MODULE   = $(MODFLAGS)
AFLAGS_MODULE   = $(MODFLAGS)
LDFLAGS_MODULE  = -r
CFLAGS_KERNEL	=
AFLAGS_KERNEL	=

KBUILD_BUILTIN := 1

ifeq ($(KBUILD_VERBOSE),1)
  quiet =
  Q =
else
  quiet=quiet_
  Q = @
endif

export quiet Q KBUILD_VERBOSE

NOSTDINC_FLAGS  = -nostdinc -iwithprefix include

export KBUILD_MODULES KBUILD_BUILTIN KBUILD_VERBOSE
export KBUILD_CHECKSRC KBUILD_SRC KBUILD_EXTMOD

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

comma := ,
depfile = $(subst $(comma),_,$(@D)/.$(@F).d)

config-targets := 0
mixed-targets  := 0
dot-config     := 1

#ifneq ($(filter $(no-dot-config-targets), $(MAKECMDGOALS)),)
#        ifeq ($(filter-out $(no-dot-config-targets), $(MAKECMDGOALS)),)
#                dot-config := 0
#        endif
#endif
#
#ifeq ($(KBUILD_EXTMOD),)
#        ifneq ($(filter config %config,$(MAKECMDGOALS)),)
#                config-targets := 1
#                ifneq ($(filter-out config %config,$(MAKECMDGOALS)),)
#                        mixed-targets := 1
#                endif
#        endif
#endif
#
#ifeq ($(mixed-targets),1)
## ===========================================================================
## We're called with mixed targets (*config and build targets).
## Handle them one by one.
#
#%:: FORCE
#        $(Q)$(MAKE) -C $(srctree) KBUILD_SRC= $@
#
#else
#ifeq ($(config-targets),1)
## ===========================================================================
## *config targets only - make sure prerequisites are updated, and descend
## in scripts/kconfig to make the *config target
#
#config: scripts_basic FORCE
#        $(Q)$(MAKE) $(build)=scripts/kconfig $@
#%config: scripts_basic FORCE
#        $(Q)$(MAKE) $(build)=scripts/kconfig $@
#
#else
## ===========================================================================
## Build targets only - this includes vmlinux, arch specific targets, clean
## targets and others. In general all targets except *config targets.
#
#ifeq ($(KBUILD_EXTMOD),)
## Additional helpers built in scripts/
## Carefully list dependencies so we do not try to build scripts twice
## in parrallel
#.PHONY: scripts
#scripts: scripts_basic include/config/MARKER
#        $(Q)$(MAKE) $(build)=$(@)
#
#scripts_basic: include/linux/autoconf.h
#
## Objects we will link into vmlinux / subdirs we need to visit
#init-y          := init/
#drivers-y       := drivers/ sound/
#net-y           := net/
#libs-y          := lib/
#core-y          := usr/
#endif # KBUILD_EXTMOD
#
#ifeq ($(dot-config),1)
## In this section, we need .config
#
## Read in dependencies to all Kconfig* files, make sure to run
## oldconfig if changes are detected.
#-include .config.cmd
#
#include .config
#
## If .config needs to be updated, it will be done via the dependency
## that autoconf has on .config.
## To avoid any implicit rule to kick in, define an empty command
#.config: ;
#
## If .config is newer than include/linux/autoconf.h, someone tinkered
## with it and forgot to run make oldconfig
#include/linux/autoconf.h: .config
#	$(Q)$(MAKE) -f $(srctree)/Makefile silentoldconfig
#else
## Dummy target needed, because used as prerequisite
#include/linux/autoconf.h: ;
#endif

include $(srctree)/scripts/Makefile.lib
include $(srctree)/src/arch-$(ARCH)/Makefile

# We don't yet have any other directories
#libs-y += src/lib/
core-y += src/apex/

apex-dirs := $(patsubst %/,%,$(filter %/, \
                                $(init-y) $(core-y) $(libs-y)))

init-y    := $(patsubst %/, %/built-in.o, $(init-y))
core-y    := $(patsubst %/, %/built-in.o, $(core-y))
drivers-y := $(patsubst %/, %/built-in.o, $(drivers-y))
net-y     := $(patsubst %/, %/built-in.o, $(net-y))
libs-y1   := $(patsubst %/, %/lib.a, $(libs-y))
libs-y2   := $(patsubst %/, %/built-in.o, $(libs-y))
libs-y    := $(libs-y1) $(libs-y2)

apex-init := $(head-y) $(init-y)
apex-main := $(core-y) $(libs-y) $(drivers-y) $(net-y)
apex-all  := $(apex-init) $(apex-main)
apex-lds  :=src/arch-$(ARCH)/apex.lds

# Rule to link vmlinux - also used during CONFIG_KALLSYMS
# May be overridden by arch/$(ARCH)/Makefile
quiet_cmd_apex__ ?= LD      $@
      cmd_apex__ ?= $(LD) $(LDFLAGS) $(LDFLAGS_vmlinux) -o $@ \
      -T $(apex-lds) $(apex-init) \
      --start-group $(apex-main) --end-group \
      $(filter-out $(apex-lds) $(apex-init) $(apex-main) FORCE ,$^)

# Link of apex
# If CONFIG_KALLSYMS is set .version is already updated
# Generate System.map and verify that the content is consistent

define rule_apex__
#        $(if $(CONFIG_KALLSYMS),,+$(call cmd,vmlinux_version))

        $(call cmd,apex__)
        $(Q)echo 'cmd_$@ := $(cmd_apex__)' > $(@D)/.$(@F).cmd

#        $(Q)$(if $($(quiet)cmd_sysmap),                 \
#          echo '  $($(quiet)cmd_sysmap) System.map' &&) \
#        $(cmd_sysmap) $@ System.map;                    \
#        if [ $$? -ne 0 ]; then                          \
#                rm -f $@;                               \
#                /bin/false;                             \
#        fi;
#        $(verify_kallsyms)
endef

# apex image
apex: $(apex-lds) $(apex-init) $(apex-main) FORCE
	$(call if_changed_rule,apex__)

# The actual objects are generated when descending, 
# make sure no implicit rule kicks in
$(sort $(apex-init) $(apex-main)) $(apex-lds): $(apex-dirs) ;

# Handle descending into subdirectories listed in $(apex-dirs)
# Preset locale variables to speed up the build process. Limit locale
# tweaks to this spot to avoid wrong language settings when running
# make menuconfig etc.
# Error messages still appears in the original language

.PHONY: $(apex-dirs)
$(apex-dirs): # prepare-all scripts
	echo apex-dirs $@
	echo $(Q)$(MAKE) $(build)=$@
	$(Q)$(MAKE) $(build)=$@
#       Leave this as default for preprocessing vmlinux.lds.S, which is now
#       done in arch/$(ARCH)/kernel/Makefile

export CPPFLAGS_apex.lds += -P -C -U$(ARCH)

# Single targets
# ---------------------------------------------------------------------------

%.s: %.c scripts FORCE
        $(Q)$(MAKE) $(build)=$(@D) $@
%.i: %.c scripts FORCE
        $(Q)$(MAKE) $(build)=$(@D) $@
%.o: %.c scripts FORCE
        $(Q)$(MAKE) $(build)=$(@D) $@
%/:      scripts prepare FORCE
        $(Q)$(MAKE) KBUILD_MODULES=$(if $(CONFIG_MODULES),1) $(build)=$(@D)
%.lst: %.c scripts FORCE
        $(Q)$(MAKE) $(build)=$(@D) $@
%.s: %.S scripts FORCE
        $(Q)$(MAKE) $(build)=$(@D) $@
%.o: %.S scripts FORCE
        $(Q)$(MAKE) $(build)=$(@D) $@

FORCE:.

#.PHONY: all
#all:
#	$(Q)$(MAKE) $(build)=src/arch-arm

