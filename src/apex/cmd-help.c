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
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include <error.h>

int cmd_help (int argc, const char** argv)
{
  extern char APEX_COMMAND_START;
  extern char APEX_COMMAND_END;
  struct command_d* command;
#if defined (CONFIG_ALLHELP)
  const char* sz = NULL;
  int cb = 0;
  int fAll = 0;
#endif

#if defined (CONFIG_ALLHELP)
  if (argc > 2)
    return ERROR_PARAM;
  if (argc == 2) {
    sz = argv[1];
    cb = strlen (sz);
    if (cb == 1 && *sz == '.')
      fAll = 1, cb = 0;
  }
#else
  if (argc > 2)
    return ERROR_PARAM;
#endif

  for (command = (struct command_d*) &APEX_COMMAND_START;
       command < (struct command_d*) &APEX_COMMAND_END;
       ++command) {
    if (!command->command || (!command->func
#if defined (CONFIG_ALLHELP)
			      && !fAll && cb == 0
#endif
			      ))
      continue;
#if defined (CONFIG_ALLHELP)
    if (cb) {
      if (!command->help)
	continue;
      if (strnicmp (sz, command->command, cb) == 0)
	printf (command->help);
      continue;
    }
#endif
    printf (" %-*.*s - %s\n", 16, 16, command->command, 
	    command->description ? command->description : "?");
  }

  return 0;
}

static __command struct command_d c_help = {
  .command = "help",
  .description = "list available commands",
  .func = cmd_help,
  COMMAND_HELP(
"help [. | COMMAND]\n"
"  Display help.\n"
"  The optional command parameter will show the detailed help\n"
"  for commands that would match that prefix.\n"
"  e.g.  help dump             # Show help for dump command\n"
"        help d                # Show help for all 'd' commands\n"
"        help .                # Show all help topics\n"
  )
};

#if defined (CONFIG_ALLHELP)

static __command struct command_d c_help_region = {
  .command = "region",
  .description = "help on regions",
  COMMAND_HELP(
"region [DRIVER:][[@]START][+LENGTH]\n"
"  Regions define a source or destination for data.  Commands\n"
"  that use regions are abstracted from the underlying IO mechanism.\n"
"  This isn't as important for memory mapped devices as it is for\n"
"  others such as SPI eeproms, NAND flash, and network devices.\n\n"
"  The DRIVER name is matched case insensitively.  Any unambiguous\n"
"  prefix will match such that 'nor:' will map to the 'nor-cfi:'\n"
"  driver.  When the DRIVER field is absent, the default memory: driver\n"
"  is used.  The drvinfo command will display a list of available\n"
"  drivers.  The START value may be in hexadecimal, decimal, or decimal\n"
"  with a multipler, e.g. 10k for kilobytes, 1m for megabytes.  The\n"
"  LENGTH takes the same form as START.\n\n"
"  The driver is responsible for parsing the region string.  It\n"
"  will add defaults when necessary or fail if the region is\n"
"  invalid.\n\n"
"  e.g.  mem:0+1k              # First kilobyte of physical memory \n"
"        mem:+1k               # Same as above\n"
"        +1k                   # Same as above\n"
"        +1024                 # Same as above\n"
"        nor:0                 # Start of nor flash\n"
"        nor:128k+64k          # 64 KiB of flash at the 128KiB boundary\n"
"        0x20200000+2m         # Two megabytes in memory\n"
  )
};

static __command struct command_d c_help_commandline = {
  .command = "command-line",
  .description = "help on the APEX command line",
  COMMAND_HELP(
"command-line\n"
"  The command line interpreter is simple.  Command words are separated\n"
"  separated into words and passed to the command interpreting function.\n"
"  Quoting is not yet supported.  Command words are separated by\n"
"  whitespace.  Command lines may be edited with ^H and DEL to erase the\n"
"  previous character and ^U to clobber the whole line.  There is no \n"
"  command history.\n\n"
//"  Multiple commands may be submitted in a single line when separated by\n"
//"  a semicolon.\n\n"
"  The interpreter is forgiving.  When matching commands it is case\n"
"  insensitive.  It will accept a command from on an unambiguous prefix.\n"
"  Thus 'h' is sufficient to invoke help.\n"
  )
};

#endif
