/* env.c
     $Id$

   written by Marc Singer
   7 Nov 2004

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

   Environment for the LH79524.

*/

#include <environment.h>
#include <driver.h>
#include <service.h>

static __env struct env_d e_cmdline = {
  .key = "cmdline",
  .default_value = "console=ttyAM0 root=/dev/hda1 "
		   "mtdparts=lpd79524_flash:2m(boot),-(root)",
  .description = "Linux kernel command line",
};

static __env struct env_d e_bootaddr = {
  .key = "bootaddr",
  .default_value = "0x20008000",
  .description = "Linux start address",
};

static __env struct env_d e_startup = {
  .key = "startup",
  .default_value =
    "copy nor:256k#1536k mem:0x20008000;"
    "wait 20 Autoboot in 2 seconds.;"
    "boot",
  .description = "Startup commands",
};

extern struct descriptor_d env_d;

static void lh79524_env_init (void)
{
  if (parse_descriptor ("nor:128K#16k", &env_d))
    return;
  if (env_d.driver->open (&env_d))
    env_d.driver->close (&env_d);
}

static __service_3 struct service_d lh79524_env_service = { 
  .init = lh79524_env_init,
};
