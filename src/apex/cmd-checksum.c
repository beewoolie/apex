/* cmd-checksum.c

   written by Marc Singer
   6 Nov 2004

   Copyright (C) 2004 Marc Singer

   This program is licensed under the terms of the GNU General Public
   License as published by the Free Software Foundation.  Please refer
   to the file debian/copyright for further details.

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
    printf ("Unable to open '%s'\n", argv[1]);
    goto fail;
  }

  {
    int index = 0;
    unsigned long crc = 0;
    while (index < d.length) {
      char __aligned rgb[512];
      int cb = d.driver->read (&d, rgb, sizeof (rgb));
      if (cb < 0) {
	result = cb;
	goto fail;
      }
      SPINNER_STEP;
      crc = compute_crc32 (crc, rgb, cb);
      index += cb;
    }

	/* Add the length to the computation */
    {
      unsigned char b;
      unsigned long v;
      for (v = index; v; v >>= 8) {
	b = v & 0xff;
	crc = compute_crc32 (crc, &b, 1);
      }
    }

    printf ("\rcrc32 0x%lx (%lu) over %d (0x%x) bytes\n",
	    ~crc, ~crc, index, index);
  }

 fail:
  close_descriptor (&d);

  return result;
}

static __command struct command_d c_checksum = {
  .command = "checksum",
  .func = cmd_checksum,
  COMMAND_DESCRIPTION ("compute crc32 checksum")
  COMMAND_HELP(
"checksum REGION\n"
"  Calculate a CRC32 checksum over REGION.\n"
"  The result conforms to the POSIX standard for the cksum command.\n"
  )

};
