/* init-drivers.c
     $Id$

   written by Marc Singer
   1 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <driver.h>

extern char APEX_DRIVER_START;
extern char APEX_DRIVER_END;

extern struct driver_d* console_driver;

void init_drivers (void)
{
  struct driver_d* driver;
  for (driver = (struct driver_d*) &APEX_DRIVER_START;
       driver < (struct driver_d*) &APEX_DRIVER_END;
       ++driver) {
    if (driver->probe && driver->probe () != 0)
      continue;
    if (driver->flags & DRIVER_CONSOLE)
      console_driver = driver;
  }
}
