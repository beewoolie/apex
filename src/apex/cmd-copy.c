/* cmd-copy.c
     $Id$

   written by Marc Singer
   5 Nov 2004

   Copyright (C) 2004 The Buici Company

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
	 cbCopy += cb)
      dout.driver->write (&dout, rgb, cb);
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
