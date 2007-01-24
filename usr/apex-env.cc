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

static error_t arg_parser (int key, char* arg, struct argp_state* state)
{
  struct arguments& args = *(struct arguments*) state->input;

  switch (key) {
  case ARGP_KEY_ARG:
    if (args.argc > 2)
      argp_usage (state);
    args.argv[args.argc++] = arg;
    break;

  case ARGP_KEY_END:
    // *** FIXME: check for the right number of arguments per command
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

int main (int argc, char** argv)
{
  struct arguments args;

  argp_parse (&argp, argc, argv, 0, 0, 0);

  printf ("%d %s\n", args.argc, args.argv[0]);

  Link link;

  if (!link.open ()) {
    printf ("unable to find APEX\n");
    return -1;
  }

  link.show_environment ();

  return 0;
}
