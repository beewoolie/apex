/* console-printf.c
     $Id$

   written by Marc Singer
   2 Nov 2004

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
#include <apex.h>

extern struct driver_d* console_driver;

int printf (const char* fmt, ...)
{
  static char rgb[1024];
  ssize_t cb;

  va_list ap;
  va_start (ap, fmt);

  cb = vsnprintf (rgb, sizeof (rgb), fmt, ap);
  va_end (ap);
  
  if (cb > 0)
    console_driver->write (0, rgb, cb);

  return cb;
}
