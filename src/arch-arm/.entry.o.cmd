cmd_src/arch-arm/entry.o := /usr/arm-linux/gcc-3.3-glibc-2.3.2/bin/arm-linux-gcc -Wp,-MD,src/arch-arm/.entry.o.d -nostdinc -iwithprefix include  -I/m/elf/home/z/embedded/apex/src/arch-arm -Isrc/arch-arm     -c -o src/arch-arm/entry.o src/arch-arm/entry.c

deps_src/arch-arm/entry.o := \
  src/arch-arm/entry.c \
    $(wildcard include/config/h.h) \

src/arch-arm/entry.o: $(deps_src/arch-arm/entry.o)

$(deps_src/arch-arm/entry.o):
