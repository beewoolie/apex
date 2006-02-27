/* cmd_checksum.c
     $Id$

   written by Marc Singer
   6 Nov 2004

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
#include <apex.h>
#include <command.h>
#include <driver.h>
#include <error.h>
#include <spinner.h>

extern unsigned long compute_crc32 (unsigned long crc, const void *pv, int cb);

int cmd_checksum (int argc, const char** argv)
{
  struct descriptor_d d;
  int result = 0;

  if (argc < 2)
    return ERROR_PARAM;

  if (   (result = parse_descriptor (argv[1], &d))
      || (result = open_descriptor (&d))) {
    printf ("Unable to open target (%s)\n", argv[1]);
    goto fail;
  }

  {
    int index = 0;
    unsigned long crc = 0;
    while (index < d.length) {
      char rgb[1024];
      int cb = d.driver->read (&d, rgb, sizeof (rgb));
      SPINNER_STEP;
      crc = compute_crc32 (crc, rgb, cb);
      index += cb;
    }

    printf ("\rcrc32 0x%lx (%lu) over %d (0x%x) bytes\n",
	    crc, crc, index, index);
  }

 fail:
  close_descriptor (&d);

  return result;
}

static __command struct command_d c_checksum = {
  .command = "checksum",
  .description = "compute crc32 checksum",
  .func = cmd_checksum,
  COMMAND_HELP(
"checksum REGION\n"
"  Calculate a CRC32 checksum over REGION.\n"
"  The result is the standard CCITT CRC32 polynomial.\n"
  )

};
