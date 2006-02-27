/* console.c
     $Id$

   written by Marc Singer
   1 Nov 2004

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

#include <stdarg.h>
#include <driver.h>
#include <linux/string.h>
#include <linux/ctype.h>

#include <apex.h>
#include <debug_ll.h>

struct driver_d* console_driver;

/* Ring buffer for polled bytes */
static char __xbss(console) rgbBuffer[128];
static size_t cbBuffer;

ssize_t console_read (struct descriptor_d* d, void* pv, size_t cb)
{
  int available;
  ssize_t cbRead = 0;

  if (console_driver == 0)
    return 0;

  available = cbBuffer;
  if (available) {
    if (available > cb)
      available = cb;
    memcpy (pv, rgbBuffer, available);
    cbRead = available;
    if (cbBuffer > available)
      memmove (rgbBuffer, rgbBuffer + available, cbBuffer - available);
    cbBuffer -= available;
    pv += available;
    cb -= available;
  }
  /* *** FIXME: do we need to be concerned about the read function
     *** returning a negative number? */
  if (cb)
    return cbRead + console_driver->read (d, pv, cb);
  return cbRead;
}

ssize_t console_write (struct descriptor_d* d, const void* pv, size_t cb)
{
//  PUTC_LL ('#');

  if (console_driver == 0)
    return 0;
  return console_driver->write (d, pv, cb);
}

ssize_t console_poll (struct descriptor_d* d, size_t cb)
{
  if (console_driver == 0)
    return 0;

  if (cb && cbBuffer) {
    if (cb > cbBuffer)
      cb = cbBuffer;
    return cb;
  }

	/* Special break character polling */
  if (cb == 0) {
    while (cbBuffer < sizeof rgbBuffer
	   && console_driver->poll (d, 1)) {
      char ch;
      console_driver->read (d, &ch, 1);
      if (ch == 0x3)		/* *** FIXME: ^C check */
	return 1;
      rgbBuffer[cbBuffer++] = ch;
    }
    return 0;
  }

  return console_driver->poll (d, cb);
}

static __driver_0 struct driver_d _console = {
  .name = "console",
  .description = "console driver",
  .flags = DRIVER_SERIAL | DRIVER_CONSOLE,
  .read = console_read,
  .write = console_write,
  .poll = console_poll,
};

/* Note that this datum will appear in the data segment.  Because it
   is const, we can be assured that it won't be changed within the
   code.  So, it's OK that we have something statically initialized in
   the program.  We cannot allow there to be initialized static symbols
   that may change during th execution of the program. */
const struct driver_d* console = &_console;

int puts (const char* fmt)
{
  return console->write (0, fmt, strlen (fmt));
}

int putchar (int ch)
{
  char rgb[1];
  rgb[0] = ch;
  return console->write (0, rgb, 1);
}

int read_command (const char* szPrompt, int* pargc, const char*** pargv)
{
  static char __xbss(console) rgb[1024];	/* Command line buffer */
  int cb;

  puts (szPrompt);

  for (cb = 0; cb < sizeof (rgb) - 1 && (cb == 0 || rgb[cb - 1]); ++cb) {
    console->read (0, &rgb[cb], 1);
    switch (rgb[cb]) {
    case '\r':
      rgb[cb] = 0;		/* Mark end of input */
      console->write (0, "\r\n", 2);
      break;
    case '\b':
      if (cb) {
	console->write (0, "\b \b", 3);
	cb -= 2;
      }
      else
	--cb;			/* Stay at the start */
      break;
    case '\x15':		/* ^U */
      while (cb--)
	console->write (0, "\b \b", 3);
      printf ("\r%s", szPrompt);
      break;
    default:
      if (isprint (rgb[cb]))
	console->write (0, &rgb[cb], 1);
      else
	--cb;
      break;
    }
  }

  rgb[cb] = 0;			/* Redundant except for overflow */

  return parse_command (rgb, pargc, pargv);
}
