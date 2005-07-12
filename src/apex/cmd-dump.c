/* cmd-dump.c
     $Id$

   written by Marc Singer
   4 Nov 2004

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


#define MORE_PAGE	(16*16)
#define MORE_LINE	(16)

int cmd_dump (int argc, const char** argv)
{
  struct descriptor_d d;
  int result = 0;
  unsigned long index;
  unsigned long more;
  static int width;

  /* Parse arguments */
  while (argc > 1 && *argv[1] == '-') {
    switch (argv[1][1]) {
    case '1':
    case '2':
    case '4':
      width = argv[1][1] - '0';
      break;
    default:
      return ERROR_PARAM;
      break;
    }
    --argc;
    ++argv;
  }

  if (argc < 2)
    return ERROR_PARAM;

  if ((result = parse_descriptor (argv[1], &d))) {
    printf ("Unable to open target (%s)\n", argv[1]);
    goto fail;
  }

  if (!d.length)
    d.length = 16*8;		/* ** FIXME: get from environment */

  if ((result = open_descriptor (&d)))
    goto fail;

  index = d.start;
  more = index + MORE_PAGE;

  /* *** FIXME: it would be a very good idea to let this function read
     *** more than 16 bytes from the input stream.  It might be
     *** expensive to fetch the incoming data (e.g. NAND) so we'd like
     *** to be friendly.  That said, it might not matter much since
     *** we're just showing it to the user.  */

  while (index < d.start + d.length) {
    char rgb[16];
    int cb = d.driver->read (&d, rgb, sizeof (rgb));

    if (cb == 0) {
      result = ERROR_RESULT (ERROR_FAILURE, "premature end of input");
      goto fail;
    }

    dumpw (rgb, cb, index, width);
    index += cb;

    if (index >= more && index < d.start + d.length) {
      extern struct driver_d* console_driver;
      char ch;

      printf (" --More-- ");
      console_driver->read (0, &ch, 1);
      switch (ch) {
      default:
      case 'q':
      case '.':
	index = d.start + d.length;
	break;

      case '\r':
      case '\n':
	more = index + MORE_LINE;
	break;

      case ' ':
	more = index + MORE_PAGE;
	break;
      }
      printf ("\r");
    }
  }

  printf ("         \r");

 fail:
  close_descriptor (&d);

  return result;
}

static __command struct command_d c_dump = {
  .command = "dump",
  .description = "dump data to the console",
  .func = cmd_dump,
  COMMAND_HELP(
"dump [-1|-2|-4] SRC\n"
"  Display SRC region data on the console.\n"
"  The default SRC region length is 64 bytes.  The -# options\n"
"  change the width of the displayed values.  The width option\n"
"  is sticky.  Once set it will remain in effect until changed\n"
"  again.\n\n"
"  e.g.  dump -4 nor:0		# Display 32 bit words\n"
"        dump 0x20200000\n"
  )
};
