/* init.c
     $Id$

   written by Marc Singer
   31 Oct 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

unsigned long status;
unsigned long value = 1023;

#include <linux/types.h>

extern int __attribute__ ((format (printf, 1, 2)))
     printf (const char * fmt, ...);

extern void init_drivers (void);

void signon (void)
{
  printf ("\r\nAPEX Boot Loader\r\n"
	  "Copyright (c) 2004, Marc Singer\r\n\r\n");
}

void init (void)
{
  init_drivers ();
  signon ();

  printf ("apex> ");
  while (1)
    ;
}
