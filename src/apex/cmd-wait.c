/* cmd-wait.c
     $Id$

   written by Marc Singer
   8 Nov 2004

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

*/

#include <config.h>
#include <linux/types.h>
#include <apex.h>
#include <command.h>
#include <linux/kernel.h>
#include <error.h>
#include <spinner.h>
#include <driver.h>
#include <console.h>


/* cmd_wait

   prompts the user and waits a specified number of 10ths of a second.
   If the user presses a key, the function returns 1.  Otherwise, it
   returns 0.

*/

int cmd_wait (int argc, const char** argv)
{
  int timeout;

  if (argc < 2)
    return ERROR_PARAM;

  timeout = simple_strtoul (argv[1], NULL, 0)*100;

  argc -= 2;
  argv += 2;
  while (argc--) {
    puts (*argv++);
    putchar (' ');
  }
  puts ("\r\n");

  /* *** FIXME: we may want to flush input */

  {
    unsigned long time = timer_read ();
    while (timer_delta (time, timer_read ()) < timeout) {
      SPINNER_STEP;
      if (console->poll (0, 0)) /* ^C will terminate */
	return 1;
    }
  }

  return 0;
}

static __command struct command_d c_wait = {
  .command = "wait",
  .func = cmd_wait,
  COMMAND_DESCRIPTION ("wait before continuing")
  COMMAND_HELP(
"wait TIMEOUT MESSAGE\n"
"  Pauses to let the user interrupt an automated process.\n"
"  The TIMEOUT value is in 10ths of a second.  Pressing a\n"
"  key on the console will interrupt wait and cancel commands\n"
"  being executed.\n"
"  e.g.  wait 50 Press a key to cancel the automatic boot process.\n"
  )
};
