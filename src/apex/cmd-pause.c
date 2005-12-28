/* cmd_pause.c
     $Id$

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.

   -----------
   DESCRIPTION
   -----------

   This function should never be included in production builds.  It's
   available for testing timers.  A general purpose pause command
   would be welcome, one that accepts a timeout parameter.


   NOTES
   -----

   o Should modify this to allow for a parameter to control which
     timer and the interval.

*/

#include <linux/types.h>
#include <apex.h>
#include <command.h>
#include <error.h>

#define SEC_PAUSE 5

int cmd_pause (int argc, const char** argv)
{
  int mode = 0;
  printf ("pausing for %d seconds\n", SEC_PAUSE);

  if (argc > 1) {
    if (argc > 2)
      return ERROR_PARAM;
    switch (*argv[1]) {
    case 't':
      mode = 0;
      break;
    case 'u':
      mode = 1;
      break;
    default:
      return ERROR_PARAM;
    }
  }

  if (mode == 0) {
    unsigned long time = timer_read ();
    while (timer_delta (time, timer_read ()) < SEC_PAUSE*1000)
      ;
  }

  if (mode == 1) {
    int i = SEC_PAUSE*1000*1000;
    i /= 32*1000;
    while (i--)
      usleep (32*1000);
  }


#if 0
  usleep (SEC_PAUSE*1000*1000);
#endif

  printf ("done\n");

  return 0;
}

static __command struct command_d c_pause = {
  .command = "pause",
  .description = "pause for 2 seconds",
  .func = cmd_pause,
  COMMAND_HELP(
"pause [MODE]\n"
"  Pause for 5 seconds using one of several timing functions.\n"
"  This command is used to test the timing functions within APEX.\n"
"  The optional MODE selects which timer function to use.  Both methods\n"
"  should be reasonably accurate.\n"
"    t  - use the timer functions (default)\n"
"    u  - use the usleep timer\n"
)
};
