/* console-printf.c
     $Id$

   written by Marc Singer
   2 Nov 2004

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

   The \r insertion code is, on the whole, relatively expensive
   code-size wise.  It could be more elegant, but that would be an
   even larger piece of code.  Still, we do it so that the program
   code is more sensible.

*/

#include <stdarg.h>
#include <driver.h>
#include <linux/string.h>
#include <apex.h>

#include <debug_ll.h>

extern struct driver_d* console;

int printf (const char* fmt, ...)
{
  static char __xbss(console) rgb[2*1024];
  ssize_t cb;
  va_list ap;
  extern ssize_t console_write (struct descriptor_d* d, 
				const void* pv, size_t cb);


//  PUTC_LL ('P');

  if (console == NULL)
    return 0;

  //  PUTC_LL ('P');

  va_start (ap, fmt);

  //  PUTC_LL ('P');
  cb = vsnprintf (rgb, sizeof (rgb) - 1, fmt, ap);
  va_end (ap);
  rgb[cb] = 0;
  
//  PUTC_LL ('P');

  {
    char* pb = rgb;
    for (; *pb; ++pb) {
      if (*pb == '\n')
	console->write (0, "\r", 1);
      console->write (0, pb, 1);
    }
  }

//  PUTC_LL ('_');

  return cb;
}
