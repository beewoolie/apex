/* command.c
     $Id$

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <linux/types.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>

void exec_monitor (void)
{
  do {
    const char** argv;
    int argc;

    printf ("apex> ");

    read_command (&argc, &argv);

    extern char APEX_COMMAND_START;
    extern char APEX_COMMAND_END;

    if (argc) {
      int cb = strlen (argv[0]);
      struct command_d* command_match = NULL;
      struct command_d* command;
      for (command = (struct command_d*) &APEX_COMMAND_START;
	   command < (struct command_d*) &APEX_COMMAND_END;
	   ++command) {
	if (strnicmp (argv[0], command->command, cb) == 0) {
	  if (command_match) {
	    printf ("ambiguous command\r\n");
	    command_match = NULL;
	    break;
	  }
	  command_match = command;
	}
	if (command->command[cb] == 0)
	  break;
      }
      if (command_match)
	command_match->func (argc, argv);
    }

  } while (1);
}

