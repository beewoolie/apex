/* console.c
     $Id$

   written by Marc Singer
   1 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <stdarg.h>
#include <driver.h>
#include <linux/string.h>

struct driver_d* console_driver;

extern int snprintf(char * buf, size_t size, const char * fmt, ...);

extern int __attribute__ ((format (printf, 1, 2)))
     printf (const char * fmt, ...);

extern int puts (const char * fmt);

int printf (const char* fmt, ...)
{
  static char rgb[1024];
  ssize_t cb;

  va_list ap;
  va_start (ap, fmt);

  cb = snprintf (rgb, sizeof (rgb), fmt, ap);
  va_end (ap);
  
  if (cb > 0)
    console_driver->write (0, rgb, cb);

  return cb;
}

int puts (const char* fmt)
{
  return console_driver->write (0, fmt, strlen (fmt));
}
