/* cmd-xreceive.c
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

#include <linux/types.h>
#include <linux/ctype.h>
#include <apex.h>
#include <command.h>
#include <driver.h>
#include <error.h>

extern int xmodem_receive (struct descriptor_d*);

int cmd_xreceive (int argc, const char** argv)
{
  struct descriptor_d d;
  int result;

  if (argc != 2)
    return ERROR_PARAM;

  if ((result = parse_descriptor (argv[1], &d))) {
    printf ("Unable to open target %s (%d)\n", argv[1], result);
    return ERROR_OPEN;
  }

  if (!d.length)
    d.length = DRIVER_LENGTH_MAX;

  if (d.driver->open (&d)) {
    d.driver->close (&d);
    return ERROR_OPEN;
  }
  
  {
    int cbRead = xmodem_receive (&d);
    printf ("%d bytes received\n", cbRead);
  }
  
  d.driver->close (&d);

  return 0;
}

static __command struct command_d c_xreceive = {
  .command = "xreceive",
  .description = "receive data over the serial line",
  .func = cmd_xreceive,
  COMMAND_HELP(
"xreceive REGION\n"
"  Receive binary data via the console using Xmodem protocol.\n"
"  Data is written directly to REGION.\n"
"  The length of REGION is ignored.\n\n"
"  e.g.  xreceive 0x2001000    # Receive and write to SDRAM\n"
"        xreceive nor:0        # Receive and write to NOR flash\n"
  )
};
