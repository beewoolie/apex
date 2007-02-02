/* command.c

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
#include <lookup.h>

const char* error_description;

#if defined (CONFIG_TIME_COMMANDS)
#define TIMECMD_INIT unsigned long time
#define TIMECMD_START ({ time = timer_read (); })
#define TIMECMD_REPORT \
	({ printf ("%ld ms\n", timer_delta (time, timer_read ())); })
#else
#define TIMECMD_INIT
#define TIMECMD_START
#define TIMECMD_REPORT
#endif

#if defined (CONFIG_ALIAS) || defined (CONFIG_ENV)

static char* expand_variables (const char* rgbSrc)
{
  const char* pchSrc;
  static char __xbss(command) rgb[CB_COMMAND_MAX];
  char* pch = rgb;
  char* pchKey = NULL;
  int state = 0;
  const char* value;
  int fChanged = 0;

  for (pchSrc = rgbSrc; pch < rgb + sizeof (rgb) - 1; ++pchSrc) {
    switch (state) {
    case 0:			/* Beginning of word */
      if (*pchSrc == '"')
	state = 11;
      if (*pchSrc == '$') {
	state = 2;
	pchKey = pch;		/* Mark start of key */
      }
      if (*pchSrc == '\\') {
	state = 21;
      }
      break;

    case 1:			/* Mid-word */
      if (isspace (*pchSrc))
	state = 0;
      break;

    case 2:			/* Scanning variable key */
      if (isalpha (*pchSrc) || *pchSrc == '_' || *pchSrc == '-')
	break;
      *pch = 0;
      value = lookup_alias_or_env (pchKey + 1, 0);
      if (value) {
	if (pch + strlen (value) + 1 >= rgb + sizeof (rgb) - 1)
	  return 0;		/* Simple failure on overflow */
	strcpy (pchKey, value);
	pch = pchKey + strlen (value);
	fChanged = 1;
	state = 0;
      }
      break;

    case 11:			/* Quoting */
      if (*pchSrc == '"')
	state = 1;
      break;

    case 21:			/* Escaping */
      state = 0;
      --pch;			/* Drop the \ */
      fChanged = 1;
      break;
    }
    *pch++ = *pchSrc;
    if (!*pchSrc)
      break;
  }
  return fChanged && state == 0 ? rgb : NULL;
}
#endif


/* parse_command

   accepts a string in a buffer and returns a parsed command in the
   form of a count and a pointer to a list of words.

   The return value is zero for a normal parse, less than zero if the
   command doesn't parse, and one if the result of the command is to
   be ignored.  This last occurs when the command begins with '-'.

*/

int parse_command (char* rgb, int* pargc, const char*** pargv)
{
  static const char* __xbss(command) argv[C_ARG_MAX];	/* Command words */
  char* pb;
  int cb;
  int result = 0;

#if defined (CONFIG_ALIASES) || defined (CONFIG_ENV)
  pb = expand_variables (rgb);
  if (pb) {
    rgb = pb;
    printf ("# %s\n", pb);
  }
  else
    pb = rgb;
#else
  pb = rgb;
#endif
  cb = strlen (pb);

	/* Construct argv.  We allow simple quoting within double
	   quotation marks.  */
  {
    while (*pb && isspace (*pb))
      ++pb;

    if (*pb == '-')		/* Ignore result of the command */
      result = 1, ++pb;

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

  return result;
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
	case ERROR_BREAK:
	  error_description = "break"; break;
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
  const char* szStartup = lookup_alias_or_env ("startup", NULL);

//  printf ("exec_monitor\n");
//  printf (" startup %s\n", szStartup);

  if (szStartup) {
    char sz[strlen (szStartup) + 1];
    char* pch = sz;
    int argc;
    const char** argv;
    strcpy (sz, szStartup);

    while (pch && *pch) {
      int result;
      char* pchEnd = strchr (pch, ';');
      if (pchEnd)
	*pchEnd = 0;
      while (*pch == ' ')
	++pch;
      printf ("\r# %s\n", pch);
      result = parse_command (pch, &argc, &argv);
      if (result >= 0 && (call_command (argc, argv) && result != 1))
	break;
      pch = (pchEnd ? pchEnd + 1 : 0);
    }
  }

  do {
    const char** argv;
    int argc;
    TIMECMD_INIT;

    SPINNER_CLEAR;
    read_command ("\r" "apex> ", &argc, &argv);
    TIMECMD_START;
    call_command (argc, argv);
    TIMECMD_REPORT;

  } while (1);
}
