/* cmd_pause.c
     $Id$

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <linux/types.h>
#include <apex.h>
#include <command.h>


int cmd_pause (int argc, const char** argv)
{
  extern unsigned long timer_read (void);
  extern unsigned long timer_delta (unsigned long, unsigned long);
  unsigned long time = timer_read ();

  printf ("pausing for 2 seconds\r\n");

  while (timer_delta (time, timer_read ()) < 2*1000)
    ;

  printf ("done\r\n");

  return 0;
}

static __command struct command_d c_pause = {
  .command = "pause",
  .description = "pause for 2 seconds",
  .func = cmd_pause,
};
