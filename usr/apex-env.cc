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

   o Need to decide how to update environment
     o Writing can only be done as fh
     o Or, perhaps mmap requires mtdblock though I doubt it
     o May need to keep track of the last open entry per key

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


const char* argp_program_version = "apex-env 1.0";


const char* g_szArgsDoc;

struct arguments {
  arguments () {
    bzero (this, sizeof (*this)); }

  int argc;
  const char* argv[3];		// command, key, value
};

struct command {
  const char* sz;
  void (*func) (Link&, int, const char**);
};

static error_t arg_parser (int key, char* arg, struct argp_state* state)
{
  struct arguments& args = *(struct arguments*) state->input;

  switch (key) {
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

static struct argp argp = {
  0, arg_parser,
  "COMMAND [ARG ...]",
  "apex-env -- user-mode access to APEX boot loader environment"
};

void cmd_printenv (Link& link, int argc, const char** argv)
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

void cmd_setenv (Link& link, int argc, const char** argv)
{
  if (argc == 3)
    link.setenv (argv[1], argv[2]);
  else
    return throw "incorrect number of command arguments";
}

void cmd_unsetenv (Link& link, int argc, const char** argv)
{
  if (argc == 2)
    link.unsetenv (argv[1]);
  else
    throw "incorrect number of command arguments";
}

void cmd_eraseenv (Link& link, int argc, const char** argv)
{
  if (argc == 1)
    link.eraseenv ();
  throw "incorrect number of command arguments";
}

static struct command commands[] = {
  { "printenv",		cmd_printenv },
  { "setenv",		cmd_setenv },
  { "unsetenv",		cmd_unsetenv },
  { "eraseenv",		cmd_eraseenv },
};

int main (int argc, char** argv)
{
  struct arguments args;
  argp_parse (&argp, argc, argv, 0, 0, &args);

  Link link;

  if (!link.open ()) {
    printf ("unable to find APEX\n");
    return -1;
  }

  if (args.argc == 0) {
    ++args.argc;
    args.argv[0] = "printenv";
  }

  //  printf ("argc %d  '%s'\n", args.argc, args.argv[0]);

  for (int i = 0; i < sizeof (commands)/sizeof (*commands); ++i) {
    if (strcasecmp (args.argv[0], commands[i].sz) == 0) {
      try {
	commands[i].func (link, args.argc, args.argv);
      } catch (char const* sz) {
	printf ("error: %s\n", sz);
      }
    }
  }

  return 0;
}
