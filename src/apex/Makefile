#
# Makefile for APEX loader
#

obj-y :=
#obj-$(CONFIG_LPD7952X) += initialize.o
obj-y += init.o init-drivers.o console.o console-printf.o
obj-y += command.o cmd-version.o cmd-help.o cmd-boot.o

obj-$(CONFIG_CMD_COPY) += cmd-copy.o
obj-$(CONFIG_CMD_CHECKSUM) += cmd-checksum.o
obj-$(CONFIG_CMD_DRVINFO) += cmd-drvinfo.o
obj-$(CONFIG_CMD_DUMP) += cmd-dump.o
obj-$(CONFIG_CMD_ERASE) += cmd-erase.o
obj-$(CONFIG_CMD_PAUSE) += cmd-pause.o
obj-$(CONFIG_CMD_VERIFY) += cmd-verify.o
obj-$(CONFIG_CMD_XRECEIVE) += cmd-xreceive.o
obj-$(CONFIG_CMD_ENV) += cmd-env.o


#CFLAGS += -mthumb
#CFLAGS_xmodem.o += -mthumb