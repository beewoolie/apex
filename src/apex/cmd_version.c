/* cmd_version.c
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

int cmd_version (int argc, const char** argv)
{
  printf ("\r\nAPEX Boot Loader\r\n"
	  "Copyright (c) 2004, Marc Singer\r\n\r\n");
  return 0;
}

static __command struct command_d c_version = {
  .command = "version",
  .func = cmd_version,

};
