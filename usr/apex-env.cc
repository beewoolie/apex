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

#include "link.h"

//#define TALK

#if defined (TALK)
# define PRINTF(f ...)	printf(f)
#else
# define PRINTF(f ...)
#endif


int main (int argc, char** argv)
{
  Link link;

  if (!link.open ()) {
    printf ("unable to find APEX\n");
    return -1;
  }

  link.show_environment ();

  return 0;
}
