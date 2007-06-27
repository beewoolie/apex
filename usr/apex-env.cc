/* apex-env.cc

   written by Marc Singer
   17 Jan 2007

   Copyright (C) 2007 Marc Singer

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

   This program grants access to the APEX environment from user-mode.
   Most of the interest part of the code are in the Link class that
   handles the IO with flash and understands the various protocols and
   formats.  This is merely a driver that calls the Link methods.

   TODO
   ----

   o It may be a good idea to add a confirmation step to the erase
     command.  Doing so also requires a 'y' option to override the
     question.

*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <argp.h>

#include "link.h"

//#define TALK

#if defined (TALK)
# define PRINTF(f ...)	printf(f)
#else
# define PRINTF(f ...)
#endif


const char* argp_program_version = "apex-env 1.1";

struct arguments {
  arguments () {
    bzero (this, sizeof (*this)); }

  int argc;
  const char* argv[3];		// command, key, value
  bool force;			// Force for the sake of eraseenv
};

struct command {
  const char* sz;
  void (*func) (Link&, int, const char**, struct arguments*);
};

static error_t arg_parser (int key, char* arg, struct argp_state* state)
{
  struct arguments& args = *(struct arguments*) state->input;

  switch (key) {
  case 'F':
    args.force = true;
    break;
  case ARGP_KEY_ARG:
    if (args.argc >= sizeof (args.argv)/sizeof (*args.argv))
      argp_usage (state);
    args.argv[args.argc++] = arg;
    break;

  case ARGP_KEY_END:
    // *** FIXME: check for the right number of arguments per command?
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

static struct argp_option options[] = {
  { "force", 'F', 0, 0, "Force a command that may cause data loss" },
  { 0 }
};

static struct argp argp = {
  options, arg_parser,
  "COMMAND [ARG ...]",
  "apex-env provides for user-mode access to APEX boot loader environment\n"
  "\n"
  "Commands:\n"
  "  describe [KEY]\t- describe KEY or all variables\n"
  "  dump\t\t\t- hexadecimal/ascii dump of environment region\n"
  "  eraseenv\t\t- erase environment region\n"
  "  printenv [KEY]\t- print KEY or all variables\n"
  "  region\t\t- report APEX environment region\n"
  "  release\t\t- report installed APEX release version\n"
  "  setenv KEY VALUE\t- set variable KEY to VALUE\n"
};

void cmd_printenv (Link& link, int argc, const char** argv,
		   struct arguments* args)
{
  switch (argc) {
  case 1:
    link.show_environment ();
    break;
  case 2:
    link.printenv (argv[1]);
    break;
  default:
    throw "incorect number of command arguments";
  }
}

void cmd_describe (Link& link, int argc, const char** argv,
		   struct arguments* args)
{
  if (argc > 2)
    throw "incorrect number of command arguments";

  link.describe ((argc > 1) ? argv[1] : NULL);
}

void cmd_dump (Link& link, int argc, const char** argv,
	       struct arguments* args)
{
  if (argc == 1)
    link.dump ();
  else
    return throw "incorrect number of command arguments";
}

void cmd_setenv (Link& link, int argc, const char** argv,
		 struct arguments* args)
{
  if (argc == 3)
    link.setenv (argv[1], argv[2]);
  else
    return throw "incorrect number of command arguments";
}

void cmd_unsetenv (Link& link, int argc, const char** argv,
		   struct arguments* args)
{
  if (argc == 2)
    link.unsetenv (argv[1]);
  else
    throw "incorrect number of command arguments";
}

void cmd_eraseenv (Link& link, int argc, const char** argv,
		   struct arguments* args)
{
  if (argc == 1)
    link.eraseenv (args->force);
  else
    throw "incorrect number of command arguments";
}

void cmd_release (Link& link, int argc, const char** argv,
		  struct arguments* args)
{
  if (argc == 1)
    printf ("%s\n", link.apexrelease ());
  else
    throw "incorrect number of command arguments";
}

void cmd_region (Link& link, int argc, const char** argv,
		 struct arguments* args)
{
  if (argc == 1)
    printf ("%s\n", link.apexregion ());
  else
    throw "incorrect number of command arguments";
}

static struct command commands[] = {
  { "describe",		cmd_describe },
  { "dump",		cmd_dump },
  { "printenv",		cmd_printenv },
  { "setenv",		cmd_setenv },
  { "unsetenv",		cmd_unsetenv },
  { "eraseenv",		cmd_eraseenv },
  { "release",		cmd_release },
  { "region",		cmd_region },
};

int main (int argc, char** argv)
{
  struct arguments args;
  argp_parse (&argp, argc, argv, 0, 0, &args);

  try {
    Link link;

    link.open ();

    if (args.argc == 0) {
      ++args.argc;
      args.argv[0] = "printenv"; // Default command
    }

    struct command* command_match = NULL;
    for (int i = 0; i < sizeof (commands)/sizeof (*commands); ++i)
      if (strncasecmp (args.argv[0], commands[i].sz,
		       strlen (args.argv[0])) == 0) {
	if (command_match)
	  throw "ambiguous command";
	command_match = &commands[i];
      }
    if (!command_match)
      throw "unknown command";
    command_match->func (link, args.argc, args.argv, &args);
    return 0;
  }
  catch (char const* sz) {
    printf ("error: %s\n", sz);
    argp_help (&argp, stderr, ARGP_HELP_SEE, "apex-env");
  }

  return 0;
}
