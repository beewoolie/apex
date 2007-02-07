# Makefile

PACKAGE:=arm-kernel-shim
VERSION:=1.0

TOOLS:=/usr/arm-linux/gcc3/arm-linux-
CC=$(TOOLS)gcc
LD=$(TOOLS)ld
OBJCOPY=$(TOOLS)objcopy

CFLAGS:=-g -Os -nostartfiles -nostdlib
LDFLAGS:= -g -X -Map shim.map -Ttext 0

SRCS:= shim.lds main.c
OBJS:=$(SRCS:.c=.o)

$(PACKAGE).bin: $(PACKAGE)
	$(OBJCOPY) -O binary $< $@

$(PACKAGE): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

%.s: %.c
	$(CC) -S $(CFLAGS) $< -o $@


.PHONY: clean
clean:
	-rm *.o $(PACKAGE) $(PACKAGE).bin *~ *.map

.PHONY: package
package: $(PACKAGE)-$(VERSION).tar.gz

$(PACKAGE)-$(VERSION).tar.gz:
	[ ! -d $(PACKAGE)-$(VERSION) ] || rm -rf $(PACKAGE)-$(VERSION)
	svn export -q file:///svn/tools/trunk/$(PACKAGE) $(PACKAGE)-$(VERSION)
	tar zcvf $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)-$(VERSION)
	rm -rf $(PACKAGE)-$(VERSION)
