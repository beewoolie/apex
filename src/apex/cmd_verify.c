/* cmd_verify.c
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

int cmd_verify (int argc, const char** argv)
{
  printf ("unimplemented\r\n");
  return 0;
}

static __command struct command_d c_version = {
  .command = "verify",
  .func = cmd_verify,

};
