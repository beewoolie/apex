/* cmd-echo.c

   written by Marc Singer
   16 May 2009

   Copyright (C) 2009 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

//#include <linux/types.h>
//#include <linux/ctype.h>
#include <apex.h>
#include <command.h>
//#include <driver.h>
//#include <error.h>
//#include <console.h>

int cmd_echo (int argc, const char** argv)
{
  int i;
  for (i = 1; i < argc; ++i)
    printf ("%s%c", argv[i], (i + 1 < argc) ? ' ' : '\n');

  return 0;
}

static __command struct command_d c_echo = {
  .command = "echo",
  .func = cmd_echo,
  COMMAND_DESCRIPTION ("echo command arguments to console")
  COMMAND_HELP(
"echo [ARGS...]\n"
"  Echo command arguments, ARGS, to the console.\n"
  )
};
