/* cmd_checksum.c
     $Id$

   written by Marc Singer
   6 Nov 2004

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
#include <error.h>

extern unsigned long compute_crc32 (unsigned long crc, const void *pv, int cb);

int cmd_checksum (int argc, const char** argv)
{
  struct descriptor_d d;
  int result;

  if (argc < 2)
    return ERROR_PARAM;

  if ((result = parse_descriptor (argv[1], &d))) {
    printf ("Unable to open target (%d)\r\n", result);
    return ERROR_OPEN;
  }

  if (d.driver->open (&d)) {
    d.driver->close (&d);
    return ERROR_OPEN;
  }


  {
    int index = 0;
    unsigned long crc = 0;
    while (index < d.length) {
      char rgb[512];
      int cb = d.driver->read (&d, rgb, sizeof (rgb));
      crc = compute_crc32 (crc, rgb, cb);
      index += cb;
    }

    printf ("\rcrc32 %d bytes 0x%lx (%lu)\r\n", index, crc, crc);
  }

  return 0;
}

static __command struct command_d c_checksum = {
  .command = "checksum",
  .description = "compute crc checksum",
  .func = cmd_checksum,

};
