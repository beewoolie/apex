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

extern int snprintf(char * buf, size_t size, const char * fmt, ...);

void init (void)
{
  char sz[20];
  
  snprintf (sz, sizeof (sz), "Hello\n");

  while (1)
    ;
}
