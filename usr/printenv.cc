/* printenv.cc

   written by Marc Singer
   23 Mar 2006

   Copyright (C) 2006 Marc Singer

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

   Program to read the APEX environment from user-land.  This is a
   very crude implementation.  It doesn't check that the environment's
   magic number matches APEX's expected magic number.  If APEX ever
   changes to a general purpose environment, where variables can be
   added by the user, the ENV_LINK_MAGIC number will change and this
   code will have to adapt.

   This implementation isn't terribly robust.  It doesn't parse the
   environment region very carefully.  It really only works when the
   region is of the form "nor:start+length" where start and length of
   decimal integers optinally followed by 'k' for a 1024 multiplier.
   Generally, this will work, but it ought to allow for hexadecimal
   values as well.

   This implementation is smart enough to detect the new environment
   format where it is possible to read the environment keys without
   scanning through the APEX binary.  It still isn't possible to know
   the default environment values without reading APEX, so this code
   will still probe into the APEX binary.

   There ought to be a switch that allows the user to read the
   environment region without reading APEX, thus only seeing the
   values that are modified and stored in the environment.


   TODO
   ----

   o Organize a structure to hold
     o APEX binary
     o struct env_d[] with string fixups
     o environment read from flash
     o descriptor parsed from env_link
     o r/w pointer for environment
   o Need to decide how to update environment
     o Writing can be done as fh or mmap'd
     o Could, perhaps, use entry pointers into this mmap'd deal
     o Need to make sure that writing mmap'd doesn't clobber everything
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
