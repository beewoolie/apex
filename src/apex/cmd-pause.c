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
  printf ("pausing for 2 seconds\r\n");

#if 0
  {
    unsigned long time = timer_read ();
    while (timer_delta (time, timer_read ()) < 2*1000)
      ;
  }
#endif

#if 1
  {
    int i = 2*1000*1000;
    i /= 32*1000;
    while (i--)
      usleep (32*1000);
  }
#endif

  printf ("done\r\n");

  return 0;
}

static __command struct command_d c_pause = {
  .command = "pause",
  .description = "pause for 2 seconds",
  .func = cmd_pause,
};
