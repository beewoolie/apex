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

int read_command (int* pargc, const char*** pargv)
{
  static char rgb[1024];	/* Command line buffer */
  static const char* argv[64];	/* Command words */
  int cb;

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
      break;
    default:
      console_driver->write (0, &rgb[cb], 1);
      break;
    }
  }

  rgb[cb] = 0;			/* Redundant except for overflow */

	/* Construct argv.  We allow simple quoting within double
	   quotation marks.  */
  {
    char* pb = rgb;

    while (isspace (*pb))
      ++pb;

    *pargc = 0;
    for (; *pargc < sizeof (argv)/sizeof (char*) && pb - rgb < cb; ++pb) {
      while (*pb && isspace (*pb))
	*pb++ = 0;
      if (*pb == '\"') {
	argv[(*pargc)++] = ++pb;
	while (*pb && *pb != '\"')
	  ++pb;
	*pb = 0;
	continue;
      } 
      if (!*pb)
	continue;
      argv[(*pargc)++] = pb++;
      while (*pb && !isspace (*pb))
	++pb;
      --pb;
    }      
  }

  *pargv = argv;

  return 1;
}

