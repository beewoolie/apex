/* cmd-fill.c
     $Id$

   written by Marc Singer
   5 Nov 2004

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

   For the time being, this function can only fill with bytes.

*/

#include <config.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include <driver.h>
#include <error.h>
#include <spinner.h>


int cmd_fill (int argc, const char** argv)
{
  struct descriptor_d dout;
  char __aligned rgb[4];
  int result = 0;

  if (argc != 3)
    return ERROR_PARAM;

  rgb[0] = simple_strtoul (argv[1], NULL, 0);

  if (   (result = parse_descriptor (argv[2], &dout))
      || (result = open_descriptor (&dout))) {
    printf ("Unable to open target %s\n", argv[2]);
    goto fail_early;
  }


  if (!dout.driver->write) {
    result = ERROR_UNSUPPORTED;
    goto fail;
  }

  if (!dout.length)
    result = ERROR_PARAM;

  {
    //    char rgb[512];
    ssize_t cb = dout.length;
    int step = (dout.driver->flags >> DRIVER_WRITEPROGRESS_SHIFT)
      &DRIVER_WRITEPROGRESS_MASK;
    if (step)
      step += 10;

    printf ("%s: writing\n", __FUNCTION__);

    while (cb--) {
      size_t cbWrote;
      SPINNER_STEP;
      cbWrote = dout.driver->write (&dout, rgb, 1);
      if (!cbWrote) {
	result = ERROR_FAILURE;
	goto fail;
      }
    }
  }

 fail:
  close_descriptor (&dout);
 fail_early:

  return result;
}

static __command struct command_d c_fill = {
  .command = "fill",
  .func = cmd_fill,
  COMMAND_DESCRIPTION ("fill a region with a byte")
  COMMAND_HELP(
"fill VALUE DST\n"
"  Fills the DST region with the byte value VALUE.  This command\n"
"  cannot be used to erase flash.\n"
"  e.g.  fill 0xe5 0x100+256\n"
  )
};
