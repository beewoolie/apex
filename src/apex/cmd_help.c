/* cmd_help.c
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

int cmd_help (int argc, const char** argv)
{
  extern char APEX_COMMAND_START;
  extern char APEX_COMMAND_END;

  struct command_d* command;
  for (command = (struct command_d*) &APEX_COMMAND_START;
       command < (struct command_d*) &APEX_COMMAND_END;
       ++command) {
    if (!command->command)
      continue;
    printf (" %-*.*s - %s\r\n", 12, 12, command->command, 
	    command->description ? command->description : "?");
  }

  return 0;
}

static __command struct command_d c_help = {
  .command = "help",
  .description = "list available commands",
  .func = cmd_help,
};
