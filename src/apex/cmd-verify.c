/* cmd_verify.c
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

#include <config.h>
#include <linux/types.h>
#include <apex.h>
#include <command.h>
#include <spinner.h>

int cmd_verify (int argc, const char** argv)
{
  unsigned long time = timer_read ();

  do {
    SPINNER_STEP;
  } while (timer_delta (time, timer_read ()) < 5*1000);
  return 0;
}

static __command struct command_d c_verify = {
  .command = "verify",
  .description = "compare sequences of bytes",
  .func = cmd_verify,

};
