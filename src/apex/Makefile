#
# Makefile for APEX loader
#

obj-y :=
#obj-$(CONFIG_LPD7952X) += initialize.o
obj-y += init.o init-drivers.o console.o console-printf.o
obj-y += command.o cmd-version.o cmd-verify.o cmd-help.o cmd-pause.o 
obj-y += cmd-drvinfo.o cmd-dump.o cmd-copy.o cmd-erase.o
obj-y += xmodem.o cmd-xreceive.o

#CFLAGS += -mthumb
#CFLAGS_xmodem.o += -mthumb