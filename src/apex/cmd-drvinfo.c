/* cmd-drvinfo.c
     $Id$

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 The Buici Company

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
#include <driver.h>

static int cmd_drvinfo (int argc, const char** argv)
{
  extern char APEX_DRIVER_START;
  extern char APEX_DRIVER_END;
  struct driver_d* d;

  for (d = (struct driver_d*) &APEX_DRIVER_START;
       d < (struct driver_d*) &APEX_DRIVER_END;
       ++d) {
    if (!d->name)
      continue;
    printf (" %-*.*s - %s\r\n", 16, 16, d->name, 
	    d->description ? d->description : "?");
  }

  return 0;
}

static __command struct command_d c_drvinfo = {
  .command = "drvinfo",
  .description = "list available drivers",
  .func = cmd_drvinfo,
};
