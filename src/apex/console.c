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

int puts (const char* fmt)
{
  return console_driver->write (0, fmt, strlen (fmt));
}
