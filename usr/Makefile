# Makefile

CROSS_COMPILE ?= /usr/arm-linux/gcc3/arm-linux-
#CFLAGS ?= -I/a/trans/embedded/m3/tools-target/usr/include
#LDFLAGS ?= -L/a/trans/embedded/m3/tools-target/usr/lib

CFLAGS += -I../include

SRCS:=printenv.cc ospartition.cc mtdpartition.cc
OBJS:=$(SRCS:.cc=.o)

CC=$(CROSS_COMPILE)g++
CXX=$(CROSS_COMPILE)g++
LD=$(CROSS_COMPILE)ld
STRIP=$(CROSS_COMPILE)strip

CFLAGS+= -O0 -g -c
CFLAGS_LINK+= -g

TARGETS:=printenv

.PHONY: all
all: $(TARGETS)

printenv: $(OBJS)
	@echo linking $@
	@$(CC) $(CFLAGS_LINK) -o $@ $(OBJS) $(LIBRARIES) $($@_LIB)

%.o: %.cc
	@echo compile $@ $(CCFLAGS)
	@$(CXX) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	-rm $(TARGETS) *.o