/* cmd_verify.c
     $Id$

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <linux/types.h>
#include <apex.h>
#include <command.h>
#include <spinner.h>

int cmd_verify (int argc, const char** argv)
{
  unsigned long time = timer_read ();

  do {
    SPINNER_STEP;
  } while (timer_delta (time, timer_read ()) < 5*1000);
  return 0;
}

static __command struct command_d c_verify = {
  .command = "verify",
  .description = "compare sequences of bytes",
  .func = cmd_verify,

};
