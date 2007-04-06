/* env.c

   written by Marc Singer
   16 Dec 2006

   Copyright (C) 2006 Marc Singer

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

   Environment for the MX31ADS.

*/

#include <config.h>
#include <environment.h>
#include <driver.h>
#include <service.h>

#if ! defined (CONFIG_ENV_DEFAULT_CMDLINE)

__env struct env_d e_cmdline = {
  .key = "cmdline",
  .default_value = "console=ttymxc0 root=/dev/hda1 "
  //		   "mtdparts=flash.0:2m(boot),-(root)"
  ,
  .description = "Linux kernel command line",
};

#endif
