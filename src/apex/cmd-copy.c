/* cmd-copy.c
     $Id$

   written by Marc Singer
   5 Nov 2004

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
#include <linux/ctype.h>
#include <apex.h>
#include <command.h>
#include <driver.h>
#include <error.h>
#include <spinner.h>


int cmd_copy (int argc, const char** argv)
{
  struct descriptor_d din;
  struct descriptor_d dout;
  int result;
  ssize_t cbCopy = 0;

  if (argc != 3)
    return ERROR_PARAM;

  if ((result = parse_descriptor (argv[1], &din))) {
    printf ("Unable to open target %s (%d)\r\n", argv[1], result);
    return ERROR_OPEN;
  }

  if ((result = parse_descriptor (argv[2], &dout))) {
    printf ("Unable to open target %s (%d)\r\n", argv[2], result);
    return ERROR_OPEN;
  }

  if (!din.driver->read || !dout.driver->write)
    return ERROR_UNSUPPORTED;

  if (!dout.length)
    dout.length = DRIVER_LENGTH_MAX;

  if (din.driver->open (&din) || dout.driver->open (&dout)) {
    din.driver->close (&din);
    dout.driver->close (&dout);
    return ERROR_OPEN;
  }

  {
    char rgb[512];
    ssize_t cb;

    for (; (cb = din.driver->read (&din, rgb, sizeof (rgb))) > 0;
	 cbCopy += cb) {
      SPINNER_STEP;
      dout.driver->write (&dout, rgb, cb);
    }
  }

  din.driver->close (&din);
  dout.driver->close (&dout);

  printf ("\r%d bytes transferred\r\n", cbCopy);

  return 0;
}

static __command struct command_d c_copy = {
  .command = "copy",
  .description = "copy data between devices",
  .func = cmd_copy,
};
