/* cmd-reset.c

   written by Marc Singer
   11 Jul 2011

   Copyright (C) 2011 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <apex.h>
#include <command.h>
#include <service.h>

#include "hardware.h"

#define WDOG_WCR	__REG16 (PHYS_WDOG1 + 0x00)
#define WDOG_WSR	__REG16 (PHYS_WDOG1 + 0x02)
#define WDOG_WRSR	__REG16 (PHYS_WDOG1 + 0x04)
#define WDOG_WICR	__REG16 (PHYS_WDOG1 + 0x06)
#define WDOG_WMCR	__REG16 (PHYS_WDOG1 + 0x08)

#define WDOG_WCR_WDE	(1<<2)	/* WDE, watchdog enable */
#define WDOG_WCR_WT_SH	(8)

#define SRC_SCR		__REG (PHYS_SRC + 0x00)


static void cmd_reset (int argc, const char** argv)
{
  release_services ();

  MASK_AND_SET (SRC_SCR, 0xf<<7, 0x5<<7); 	   /* Enable watchdog reset */
  WDOG_WCR = WDOG_WCR_WDE | (0 << WDOG_WCR_WT_SH); /* Reset in 0.5 seconds */

  while (1)
    ;
}

static __command struct command_d c_reset = {
  .command = "reset",
  .func = (command_func_t) cmd_reset,
  COMMAND_DESCRIPTION ("reset target")
  COMMAND_HELP(
"reset\n"
"  Reset the system.\n"
"  This will perform a (nearly) immediate, hard reset of the CPU.\n"
  )
};
