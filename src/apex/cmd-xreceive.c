/* cmd-xreceive.c
     $Id$

   written by Marc Singer
   6 Nov 2004

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

extern int xmodem_receive (struct descriptor_d*);

int cmd_xreceive (int argc, const char** argv)
{
  struct descriptor_d d;
  int result;

  if (argc != 2)
    return ERROR_PARAM;

  if ((result = parse_descriptor (argv[1], &d))) {
    printf ("Unable to open target %s (%d)\r\n", argv[1], result);
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
    printf ("%d bytes received\r\n", cbRead);
  }
  
  d.driver->close (&d);

  return 0;
}

static __command struct command_d c_xreceive = {
  .command = "xreceive",
  .description = "receive data over the serial line",
  .func = cmd_xreceive,
};
