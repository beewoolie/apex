# Makefile

PACKAGE:=arm-kernel-shim
VERSION:=1.0

TOOLS:=/usr/arm-linux/gcc3/arm-linux-
CC=$(TOOLS)gcc
LD=$(TOOLS)ld
OBJCOPY=$(TOOLS)objcopy

CFLAGS:=-g -Os  -nostartfiles -nostdlib 
LDFLAGS:= -g -X -Map shim.map -Ttext 0

SRCS:= shim.lds main.c
OBJS:=$(SRCS:.c=.o)

$(PACKAGE).bin: $(PACKAGE)
	$(OBJCOPY) -O binary $< $@

$(PACKAGE): $(OBJS)
	$(LD) $(LDFLAGS) $(OBJS) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@


.PHONY: clean
clean:
	rm *.o $(PACKAGE) $(PACKAGE).bin

$(PACKAGE)-$(VERSION).tar.gz:
	[ -d $(PACKAGE)-$(VERSION) ] && rm -rf $(PACKAGE)-$(VERSION)
	mkdir $(PACKAGE)-$(VERSION)
	( cd $(PACKAGE)-$(VERSION) ;\
	  svn co file:///svn/tools/$(PACKAGE) . )
	tar zcvf $(PACKAGE)-$(VERSION).tar.gz $(PACKAGE)-$(VERSION)
	rm -rf $(package)-$(VERSION)
