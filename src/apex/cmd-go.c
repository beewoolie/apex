/* cmd-go.c
     $Id$

   written by Marc Singer
   3 Feb 2005

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

#include <linux/types.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include <error.h>
#include <linux/kernel.h>
#include <config.h>
#include <environment.h>
#include <service.h>

int cmd_go (int argc, const char** argv)
{
  unsigned long address;

  if (argc != 2)
    return ERROR_PARAM;


  address = simple_strtoul (argv[1], NULL, 0);

  printf ("Jumping to 0x%p...\n", (void*) address);

  //serial_flush_output();

  release_services ();

  __asm volatile ("mov pc, %0" :: "r" (address));

  return 0;
}

static __command struct command_d c_go = {
  .command = "go",
  .description = "execute program at address",
  .func = cmd_go,
};
