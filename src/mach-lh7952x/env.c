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

#include <config.h>
#include <environment.h>
#include <driver.h>
#include <service.h>

#if defined (CONFIG_MACH_LPD79520) && !defined (CONFIG_ENV_DEFAULT_CMDLINE)

__env struct env_d e_cmdline = {
  .key = "cmdline",
  .default_value = "console=ttyAM1"
		   " root=/dev/hda1"
  //		   " root=/dev/mtdblock1 rootfstype=jffs2"
		   " mtdparts="
		   "lpd79520_norflash:2m(boot)ro,-(root)"
  ,
  .description = "Linux kernel command line",
};

#endif

#if defined (CONFIG_MACH_LPD79524) && !defined (CONFIG_ENV_DEFAULT_CMDLINE)

__env struct env_d e_cmdline = {
  .key = "cmdline",
  .default_value = "console=ttyAM0"
#if 1
		   " root=/dev/hda2 3" /* Boot to CF card */
#else
  //		   " root=/dev/hda1"
  //		   " root=/dev/mtdblock1 rootfstype=jffs2"
		   " root=/dev/mtdblock3 rootfstype=jffs2"
		   " mtdparts="
		   "lpd79524_norflash:2m(boot)ro,-(root)"
		   ";" 
		   "lpd79524_nandflash:2m(boot)ro,-(root)"
#endif
  ,
  .description = "Linux kernel command line",
};

#endif

#if defined (CONFIG_MACH_KEV79524) && !defined (CONFIG_ENV_DEFAULT_CMDLINE)

__env struct env_d e_cmdline = {
  .key = "cmdline",
  .default_value = "console=ttyAM0"
  //		   " root=/dev/hda1"
  //		   " root=/dev/mtdblock1 rootfstype=jffs2"
		   " root=/dev/mtdblock3 rootfstype=jffs2"
		   " mtdparts="
		   "lpd79524_norflash:2m(boot)ro,-(root)"
  ,
  .description = "Linux kernel command line",
};

#endif

