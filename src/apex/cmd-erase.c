/* cmd-erase.c
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


int cmd_erase (int argc, const char** argv)
{
  struct open_d d;
  unsigned long fh;
  int result;

  if (argc != 2)
    return ERROR_PARAM;

  if ((result = parse_descriptor (argv[1], &d))) {
    printf ("Unable to open target %s (%d)\r\n", argv[1], result);
    return ERROR_OPEN;
  }

  if (!d.driver->erase)
    return ERROR_UNSUPPORTED;

  if (!d.length)
    d.length = 1;

  fh = open_descriptor (&d);
  if (fh == -1)
    return ERROR_OPEN;

  d.driver->erase (fh, d.length);

  close_descriptor (&d);

  return 0;
}

static __command struct command_d c_erase = {
  .command = "erase",
  .description = "erase device region",
  .func = cmd_erase,
};
