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
#include <error.h>
#include <linux/ctype.h>
#include <environment.h>

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
	printf ("Ambiguous command.  Try 'help'.\r\n");
	return ERROR_NOCOMMAND;
      }
      command_match = command;
      if (command->command[cb] == 0) /* Exact match */
	break;
    }
  }
  if (command_match) {
    int result = command_match->func (argc, argv);
    if (result < 0)
      printf ("Error %d\r\n", result);
    return result;
  }
  return ERROR_NOCOMMAND;
}


void exec_monitor (void)
{
  const char* szStartup = env_fetch ("startup");
  if (szStartup) {
    char sz[strlen (szStartup) + 1];
    char* pch = sz;
    int argc;
    const char** argv;
    strcpy (sz, szStartup);
    //    printf ("startup '%s'\r\n", sz);
    
    while (*pch) {
      char* pchEnd = strchr (pch, ';');
      *pchEnd = 0;
      printf ("\r# %s\r\n", pch);
      parse_command (pch, &argc, &argv);
      if (call_command (argc, argv))
	break;
      pch = pchEnd + 1;
    }
  }

  do {
    const char** argv;
    int argc;

    read_command ("\rapex> ", &argc, &argv);
    call_command (argc, argv);

  } while (1);
}
