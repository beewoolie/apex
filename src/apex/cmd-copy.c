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
  struct open_d din;
  struct open_d dout;
  unsigned long fhIn;
  unsigned long fhOut;
  int result;
  ssize_t cbCopy = 0;

  if (argc != 3)
    return ERROR_PARAM;

  if ((result = open_descriptor (argv[1], &din))) {
    printf ("Unable to open target %s (%d)\r\n", argv[1], result);
    return ERROR_OPEN;
  }

  if ((result = open_descriptor (argv[2], &dout))) {
    printf ("Unable to open target %s (%d)\r\n", argv[2], result);
    return ERROR_OPEN;
  }

  if (!din.driver->read || !dout.driver->write)
    return ERROR_UNSUPPORTED;

  if (!dout.length)
    dout.length = DRIVER_LENGTH_MAX;

  fhIn = din.driver->open (&din);
  if (fhIn == -1)
    return ERROR_OPEN;
  fhOut = dout.driver->open (&dout);
  if (fhOut == -1) {
    din.driver->close (fhIn);
    return ERROR_OPEN;
  }

  {
    char rgb[512];
    ssize_t cb;

    for (; (cb = din.driver->read (fhIn, rgb, sizeof (rgb))) > 0;
	 cbCopy += cb)
      dout.driver->write (fhOut, rgb, cb);
  }

  din.driver->close (fhIn);
  dout.driver->close (fhOut);

  printf ("%d bytes transferred\r\n", cbCopy);

  return 0;
}

static __command struct command_d c_copy = {
  .command = "copy",
  .description = "copy data between devices",
  .func = cmd_copy,
};
