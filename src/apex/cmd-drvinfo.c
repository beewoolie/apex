/* cmd-drvinfo.c
     $Id$

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <linux/types.h>
#include <apex.h>
#include <command.h>
#include <driver.h>

static int cmd_drvinfo (int argc, const char** argv)
{
  extern char APEX_DRIVER_START;
  extern char APEX_DRIVER_END;
  struct driver_d* d;

  for (d = (struct driver_d*) &APEX_DRIVER_START;
       d < (struct driver_d*) &APEX_DRIVER_END;
       ++d) {
    if (!d->name)
      continue;
    printf (" %-*.*s - %s\r\n", 16, 16, d->name, 
	    d->description ? d->description : "?");
  }

  return 0;
}

static __command struct command_d c_drvinfo = {
  .command = "drvinfo",
  .description = "list available drivers",
  .func = cmd_drvinfo,
};
