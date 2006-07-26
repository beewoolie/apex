# Makefile

CROSS_COMPILE ?= /usr/arm-linux/gcc3/arm-linux-
#CFLAGS ?= -I/a/trans/embedded/m3/tools-target/usr/include
#LDFLAGS ?= -L/a/trans/embedded/m3/tools-target/usr/lib

CFLAGS += -I../include

SRCS:=printenv.cc
OBJS:=$(SRCS:.cc=.o)

CC=$(CROSS_COMPILE)g++
CXX=$(CROSS_COMPILE)g++
LD=$(CROSS_COMPILE)ld
STRIP=$(CROSS_COMPILE)strip

CFLAGS+= -Os -g -c

TARGETS:=printenv

.PHONY: all
all: $(TARGETS)

printenv: printenv.o

printenv: printenv.o
	@echo linking $@
	@$(CC) $(CFLAGS_LINK) -o $@ $^ $(LIBRARIES) $($@_LIB)

%.o: %.cc
	@echo compile $@ $(CCFLAGS)
	@$(CXX) $(CFLAGS) -o $@ $<

.PHONY: clean
clean:
	-rm $(TARGETS) *.o