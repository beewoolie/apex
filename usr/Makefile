# Makefile

CROSS_COMPILE ?= /usr/arm-linux/gcc3/arm-linux-
#CFLAGS ?= -I/a/trans/embedded/m3/tools-target/usr/include
#LDFLAGS ?= -L/a/trans/embedded/m3/tools-target/usr/lib

CFLAGS += -I../include

SRCS:=apex-env.cc ospartition.cc mtdpartition.cc link.cc
OBJS:=$(SRCS:.cc=.o)

DEPS:=$(SRCS:%.cc=%.d)

CC=$(CROSS_COMPILE)g++
CXX=$(CROSS_COMPILE)g++
LD=$(CROSS_COMPILE)ld
STRIP=$(CROSS_COMPILE)strip

CFLAGS+= -O0 -g -c
CFLAGS_LINK+= -g

TARGETS:=apex-env

.PHONY: all
all: $(TARGETS)

printenv: $(OBJS)
	@echo linking $@
	@$(CC) $(CFLAGS_LINK) -o $@ $(OBJS) $(LIBRARIES) $($@_LIB)

apex-env: $(OBJS)
	@echo linking $@
	@$(CC) $(CFLAGS_LINK) -o $@ $(OBJS) $(LIBRARIES) $($@_LIB)

%.o: %.cc
	@echo compile $@ $(CCFLAGS)
	@$(CXX) $(CFLAGS) -o $@ $<

%.d: %.cc
	@set -e; rm -f $@; \
          $(CXX) -MM $(CFLAGS) $< > $@.$$$$; \
          sed 's,\($*\)\.o[ :]*,\1.o $@ : ,g' < $@.$$$$ > $@; \
          rm -f $@.$$$$

.PHONY: clean
clean:
	-rm $(TARGETS) *.o

-include $(DEPS)
