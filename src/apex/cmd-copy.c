/* cmd-copy.c
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
  int result = 0;
  ssize_t cbCopy = 0;

  if (argc != 3)
    return ERROR_PARAM;

  if (   (result = parse_descriptor (argv[1], &din))
      || (result = open_descriptor (&din))) {
    printf ("Unable to open target %s\n", argv[1]);
    goto fail_early;
  }

  if (   (result = parse_descriptor (argv[2], &dout))
      || (result = open_descriptor (&din))) {
    printf ("Unable to open target %s\n", argv[2]);
    goto fail;
  }

  if (!din.driver->read || !dout.driver->write) {
    result = ERROR_UNSUPPORTED;
    goto fail;
  }

  if (!dout.length)
    dout.length = DRIVER_LENGTH_MAX;

  {
    char rgb[1024];
    ssize_t cb;
    int report_last = -1;
    int step = (dout.driver->flags >> DRIVER_WRITEPROGRESS_SHIFT)
      &DRIVER_WRITEPROGRESS_MASK;
    if (step)
      step += 10;

    for (; (cb = din.driver->read (&din, rgb, sizeof (rgb))) > 0;
	 cbCopy += cb) {
      int report;
      size_t cbWrote;
      SPINNER_STEP;
      cbWrote = dout.driver->write (&dout, rgb, cb);
      if (cbWrote != cb) {
	result = ERROR_FAILURE;
	goto fail;
      }
      report = cbCopy>>step;
      if (step && report != report_last) {
	printf ("\r   %d KiB\r", cbCopy/1024);
	report_last = report;
      }
    }
  }

  printf ("\r%d bytes transferred\n", cbCopy);

 fail:
  close_descriptor (&din);
  close_descriptor (&dout);
 fail_early:

  return result;
}

static __command struct command_d c_copy = {
  .command = "copy",
  .description = "copy data between devices",
  .func = cmd_copy,
  COMMAND_HELP(
"copy SRC DST\n"
"  Copied data from SRC region to DST region.\n"
"  The length of the DST region is ignored.\n"
"  e.g.  copy mem:0x20200000+0x4500 nor:0\n"
  )
};
