/* console-printf.c
     $Id$

   written by Marc Singer
   2 Nov 2004

   Copyright (C) 2004 The Buici Company

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
