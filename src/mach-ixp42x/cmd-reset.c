/* cmd-reset.c
     $Id$

   written by Marc Singer
   12 Nov 2004

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

#include <config.h>
#include <apex.h>
#include <command.h>
#include "hardware.h"

#define KEY 0x482e
#define OTS_WDOG_ENABLE_RESET	(1<<0) /* Allow watchdog to reset CPU */
#define OTS_WDOG_ENABLE_CNT_EN	(1<<2) /* Enable watchdog countdown */

static int  __attribute__((noreturn)) cmd_reset (int argc, const char** argv)
{
  OTS_WDOG_KEY = KEY;		/* Unlock watchdog registers */
  OTS_WDOG = 1;			/* Short count */
  OTS_WDOG_ENABLE = 0
    | OTS_WDOG_ENABLE_RESET
    | OTS_WDOG_ENABLE_CNT_EN;
}

static __command struct command_d c_reset = {
  .command = "reset",
  .description = "reset target",
  .func = cmd_reset,
};
