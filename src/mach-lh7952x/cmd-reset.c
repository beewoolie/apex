/* cmd-reset.c
     $Id$

   written by Marc Singer
   12 Nov 2004

   Copyright (C) 2004 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#include <apex.h>
#include <command.h>
#include "lh79524.h"

int  __attribute__((noreturn)) cmd_reset (int argc, const char** argv)
{
  __REG (RCPC_PHYS | RCPC_CTRL) |= (1<<9); /* Unlock */
  __REG (RCPC_PHYS | RCPC_SOFTRESET) = 0xdead;
}

static __command struct command_d c_reset = {
  .command = "reset",
  .description = "reset target",
  .func = cmd_reset,
};
