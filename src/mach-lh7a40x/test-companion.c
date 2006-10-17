/* test-companion.c

   written by Marc Singer
   17 Oct 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <apex.h>
#include "hardware.h"
#include <command.h>

#include <debug_ll.h>

int cmd_click (int argc, const char** argv)
{
  int i;
  printf ("%s\n", __FUNCTION__);

  for (i = 0; i < 1000; ++i) {
    GPIO_PGD |= (1<<7);
    mdelay (250);
    GPIO_PGD &= ~(1<<7);
    mdelay (750);
  }


  return 0;
}

static __command struct command_d c_click = {
  .command = "click",
  .func = cmd_click,
  COMMAND_DESCRIPTION ("buzzer click")
  COMMAND_HELP (
"click\n"
"\n")
};

