/* init.c
     $Id$

   written by Marc Singer
   31 Oct 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <linux/types.h>
#include <apex.h>

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
