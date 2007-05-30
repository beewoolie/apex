/* cmd-reset.c

   written by Marc Singer
   25 May 2007

   Copyright (C) 2007 Marc Singer

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

#include <config.h>
#include <apex.h>
#include <command.h>
#include <service.h>

#include "hardware.h"

#define WDOG_WCR	__REG16 (PHYS_WDOG + 0x00)
#define WDOG_SR		__REG16 (PHYS_WDOG + 0x02)
#define WDOG_WRSR	__REG16 (PHYS_WDOG + 0x04)

#define WDOG_WCR_WDE	(1<<2)
#define WDOG_WT_SH	(8)


static void cmd_reset (int argc, const char** argv)
{
  release_services ();		/* Primarily to prep NOR flash */

  WDOG_WCR = WDOG_WCR_WDE;

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
