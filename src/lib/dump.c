/* dump.c

   written by Marc Singer
   16 Nov 2004

   Copyright (C) 2004 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#include <linux/types.h>
#include <linux/ctype.h>
#include <apex.h>
#include <command.h>
#include <driver.h>
#include <error.h>

void dumpw (const void* pv, size_t cb, unsigned long index, int width)
{
  const char* rgb = (const char*) pv;

  int i;

  if (   (width == 2 && ((unsigned long) rgb & 1))
      || (width == 4 && ((unsigned long) rgb & 3))) {
    printf ("%s: unable to display unaligned data\n", __FUNCTION__);
    return;
  }

  while (cb > 0) {
    size_t available = 16;
    if (available > cb)
      available = cb;
    printf ("%08lx: ", index);
    for (i = 0; i < 16; ++i) {
      if (i < available) {
	switch (width) {
	default:
	case 1:
	  printf ("%02x ", rgb[i]);
	  break;
	case 2:
	  printf ("%04x ", *((u16*)&rgb[i]));
	  ++i;
	  break;
	case 4:
	  printf ("%08x ", *((u32*)&rgb[i]));
	  i += 3;
	  break;
	}
      }
      else
	printf ("   ");
      if (i%8 == 7)
	putchar (' ');
    }
    for (i = 0; i < 16; ++i) {
      if (i == 8)
	putchar (' ');
      putchar ( (i < available) ? (isascii (rgb[i]) && isprint (rgb[i])
                                   ? rgb[i] : '.') : ' ');
    }
    printf ("\n");

    cb    -= available;
    index += available;
    rgb   += available;
  }
}
