/* dump.c
     $Id$

   written by Marc Singer
   16 Nov 2004

   Copyright (C) 2004 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.

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

void dump (const unsigned char* rgb, int cb, unsigned long index)
{
  int i;

  while (cb > 0) {
    printf ("%08lx: ", index);
    for (i = 0; i < 16; ++i) {
      if (i < cb)
	printf ("%02x ", rgb[i]);
      else
	printf ("   ");
      if (i%8 == 7)
	putchar (' ');
    }
    for (i = 0; i < 16; ++i) {
      if (i == 8)
	putchar (' ');
      putchar ( (i < cb) ? (isprint (rgb[i]) ? rgb[i] : '.') : ' ');
    }
    printf ("\r\n");

    cb -= 16;
    index += 16;
  }
}
