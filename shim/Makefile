# Makefile

TOOLS:=/usr/arm-linux/gcc3/arm-linux-
CC=$(TOOLS)gcc
LD=$(TOOLS)ld
OBJCOPY=$(TOOLS)objcopy

CFLAGS:=-g -Os  -nostartfiles -nostdlib 
LDFLAGS:= -g -X -Map shim.map -Ttext 0

SRCS:= shim.lds main.c
OBJS:=$(SRCS:.c=.o)

shim.bin: shim
	$(OBJCOPY) -O binary $< $@

shim: $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@


clean:
	rm *.o shim shim.bin

