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

   Environment for the IXP42x.

*/

#include <config.h>
#include <environment.h>
#include <driver.h>
#include <service.h>

#define _s(v) #v 
#define _t(v) _s(v)

#if defined (CONFIG_MACH_NSLU2)

static __env struct env_d e_cmdline = {
  .key = "cmdline",
  .default_value = "console=ttyAM0,115200"
		   " root=/dev/mtdblock1 rootfstype=jffs2"
		   " mtdparts="
		   "physmappedflash:2m(boot)ro,-(root)"
  ,
  .description = "Linux kernel command line",
};

#endif
