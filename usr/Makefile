# Makefile

CROSS_COMPILE ?= /usr/arm-linux/gcc3/arm-linux-
#CFLAGS ?= -I/a/trans/embedded/m3/tools-target/usr/include
#LDFLAGS ?= -L/a/trans/embedded/m3/tools-target/usr/lib

CFLAGS += -I../include

SRCS:=apex-env.cc mtdpartition.cc link.cc
OBJS:=$(SRCS:.cc=.o)

DEPS:=$(SRCS:%.cc=%.d)

CC=$(CROSS_COMPILE)g++
CXX=$(CROSS_COMPILE)g++
LD=$(CROSS_COMPILE)ld
STRIP=$(CROSS_COMPILE)strip

CFLAGS+= -Os -g -c
CFLAGS_LINK+= -g

TARGETS:=apex-env

.PHONY: all
all: $(TARGETS) groom

apex-env: $(OBJS)
	@echo linking $@
	@$(CC) $(CFLAGS_LINK) -o $@ $(OBJS) $(LIBRARIES) $($@_LIB)

%.o: %.cc
	@echo compile $<
	@$(CXX) $(CFLAGS) -o $@ $<

%.d: %.cc
	@set -e; rm -f $@; \
          $(CXX) -MM $(CFLAGS) $< > $@.$$$$; \
          sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
          rm -f $@.$$$$

.PHONY: clean
clean:
	-rm $(wildcard $(TARGETS) *.o *.d)

CRUFT:=$(wildcard  *.d.*)

.PHONY: groom
groom:
ifneq ($(CRUFT),)
	@rm $(CRUFT)
endif

-include $(DEPS)
