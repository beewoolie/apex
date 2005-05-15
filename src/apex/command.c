/* command.c
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
#include <linux/ctype.h>
#include <environment.h>
#include <spinner.h>

const char* error_description;

int parse_command (char* rgb, int* pargc, const char*** pargv)
{
  static const char* argv[64];	/* Command words */
  int cb = strlen (rgb);

	/* Construct argv.  We allow simple quoting within double
	   quotation marks.  */
  {
    char* pb = rgb;

    while (isspace (*pb))
      ++pb;

    *pargc = 0;
    for (; *pargc < sizeof (argv)/sizeof (char*) && pb - rgb < cb; ++pb) {
      while (*pb && isspace (*pb))
	*pb++ = 0;
      if (*pb == '\"') {
	argv[(*pargc)++] = ++pb;
	while (*pb && *pb != '\"')
	  ++pb;
	*pb = 0;
	continue;
      } 
      if (!*pb)
	continue;
      argv[(*pargc)++] = pb++;
      while (*pb && !isspace (*pb))
	++pb;
      --pb;
    }      
  }

  *pargv = argv;

  return 1;
}


int call_command (int argc, const char** argv) 
{
  extern char APEX_COMMAND_START;
  extern char APEX_COMMAND_END;
  int cb;
  struct command_d* command_match = NULL;
  struct command_d* command;

  if (!argc)
    return 0;

  cb = strlen (argv[0]);

  for (command = (struct command_d*) &APEX_COMMAND_START;
       command < (struct command_d*) &APEX_COMMAND_END;
       ++command) {
    if (strnicmp (argv[0], command->command, cb) == 0) {
      if (command_match) {
	printf ("Ambiguous command.  Try 'help'.\n");
	return ERROR_NOCOMMAND;
      }
      command_match = command;
      if (command->command[cb] == 0) /* Exact match */
	break;
    }
  }
  if (command_match) {
    int result;
    error_description = NULL;
    result = command_match->func (argc, argv);
    if (result < ERROR_IMPORTANT) {
      printf ("Error %d", result);
#if !defined (CONFIG_SMALL)
      if (error_description == NULL) {
	switch (result) {
	case ERROR_PARAM:
	  error_description = "parameter error"; break;
	case ERROR_OPEN:
	  error_description = "error on open"; break;
	case ERROR_AMBIGUOUS:
	  error_description = "ambiguous"; break;
	case ERROR_NODRIVER:
	  error_description = "no driver"; break;
	case ERROR_UNSUPPORTED:
	  error_description = "unsupported"; break;
	case ERROR_BADPARTITION:
	  error_description = "bad partition"; break;
	case ERROR_FILENOTFOUND:
	  error_description = "file not found"; break;
	case ERROR_IOFAILURE:
	  error_description = "i/o failure"; break;
	}
      }
#endif
      printf (" (%s)", error_description);
      printf ("\n");
    }
    return result;
  }
  return ERROR_NOCOMMAND;
}


void exec_monitor (void)
{
  const char* szStartup;

//  printf ("exec_monitor\n");
  szStartup = env_fetch ("startup");
//  printf (" startup %s\n", szStartup);

  if (szStartup) {
    char sz[strlen (szStartup) + 1];
    char* pch = sz;
    int argc;
    const char** argv;
    strcpy (sz, szStartup);
    
    while (*pch) {
      char* pchEnd = strchr (pch, ';');
      *pchEnd = 0;
      printf ("\r# %s\n", pch);
      parse_command (pch, &argc, &argv);
      if (call_command (argc, argv))
	break;
      pch = pchEnd + 1;
    }
  }

  do {
    const char** argv;
    int argc;

    SPINNER_CLEAR;
    read_command ("\rapex> ", &argc, &argv);
    call_command (argc, argv);

  } while (1);
}
