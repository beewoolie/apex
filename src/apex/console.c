/* console.c
     $Id$

   written by Marc Singer
   1 Nov 2004

   Copyright (C) 2004 The Buici Company

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

#include <stdarg.h>
#include <driver.h>
#include <linux/string.h>
#include <linux/ctype.h>

#include <apex.h>

struct driver_d* console_driver;

int puts (const char* fmt)
{
  return console_driver->write (0, fmt, strlen (fmt));
}

int putchar (int ch)
{
  return console_driver->write (0, &ch, 1);
}

int read_command (const char* szPrompt, int* pargc, const char*** pargv)
{
  static char rgb[1024];	/* Command line buffer */
  int cb;

  puts (szPrompt);

  for (cb = 0; cb < sizeof (rgb) - 1 && (cb == 0 || rgb[cb - 1]); ++cb) {
    console_driver->read (0, &rgb[cb], 1);
    switch (rgb[cb]) {
    case '\r':
      rgb[cb] = 0;		/* Mark end of input */
      console_driver->write (0, "\r\n", 2);
      break;
    case '\b':
      if (cb) {
	console_driver->write (0, "\b \b", 3);
	cb -= 2;
      }
      else
	--cb;			/* Stay at the start */
      break;
    case '\x15':		/* ^U */
      while (cb--)
	console_driver->write (0, "\b \b", 3);
      printf ("\r%s", szPrompt);
      break;
    default:
      if (isprint (rgb[cb]))
	console_driver->write (0, &rgb[cb], 1);
      else
	--cb;
      break;
    }
  }

  rgb[cb] = 0;			/* Redundant except for overflow */

  return parse_command (rgb, pargc, pargv);
}

