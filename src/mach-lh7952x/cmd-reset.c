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

#if defined (CONFIG_MACH_LH79524)
#include "lh79524.h"
#endif

#if defined (CONFIG_MACH_LH79520)
# include "lh79520.h"
#endif

static int  __attribute__((noreturn)) cmd_reset (int argc, const char** argv)
{
  __REG (RCPC_PHYS + RCPC_CTRL) |= (1<<9); /* Unlock */
  __REG (RCPC_PHYS + RCPC_SOFTRESET) = 0xdead;
}

static __command struct command_d c_reset = {
  .command = "reset",
  .description = "reset target",
  .func = cmd_reset,
};
