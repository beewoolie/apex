/* cmd_help.c
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
    printf (" %-*.*s - %s\r\n", 16, 16, command->command, 
	    command->description ? command->description : "?");
  }

  return 0;
}

static __command struct command_d c_help = {
  .command = "help",
  .description = "list available commands",
  .func = cmd_help,
};
