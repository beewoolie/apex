#
# Makefile for APEX loader
#

obj-y :=
#obj-$(CONFIG_LPD7952X) += initialize.o
obj-y += initialize.o serial.o timer.o
obj-$(CONFIG_DRIVER_NAND) += drv-nand.o
obj-$(CONFIG_DRIVER_NOR) += drv-nor.o
obj-y += env.o

CFLAGS_initialize.o += -Os
