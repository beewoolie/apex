/* spinner.c
     $Id$

   written by Marc Singer
   8 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

   Implements a user feedback mechanism while performing long running
   commands.

*/

#include <config.h>
#include <apex.h>

#include <spinner.h>

void spinner_step (void)
{
  static int step;
  int v = (timer_delta (0, timer_read ())/128)%8;
  static unsigned char rgch[]
    = { '|', '/', '-', '\\', '|', '/', '-', '\\' } ;

  if (v == step)
    return;
  step = v;

  putchar ('\r');
  putchar (rgch[step]);
}
