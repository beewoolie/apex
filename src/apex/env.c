/* env.c
     $Id$

   written by Marc Singer
   10 Nov 2004

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

   Default environment variables.

*/

#include <config.h>
#include <environment.h>
#include <driver.h>
#include <service.h>

#define _s(v) #v 
#define _t(v) _s(v)

#if (defined (CONFIG_ENV_REGION_KERNEL) && defined (CONFIG_KERNEL_LMA))\
    || defined (CONFIG_ENV_AUTOBOOT)
static __env struct env_d e_startup = {
  .key = "startup",
  .default_value =
#if defined (CONFIG_ENV_REGION_KERNEL) && defined (CONFIG_KERNEL_LMA)
    "copy " _t(CONFIG_ENV_REGION_KERNEL) " " _t(CONFIG_KERNEL_LMA) ";"
#endif
#if defined (CONFIG_ENV_AUTOBOOT)
# if CONFIG_ENV_AUTOBOOT != 0
    "wait " _t(CONFIG_ENV_AUTOBOOT) " Press a key to cancel autoboot.;"
# endif
    "boot"
#endif
  ,
  .description = "Startup commands",
};
#endif

#if defined (CONFIG_ENV_REGION)
extern struct descriptor_d env_d;

static void env_init (void)
{
  if (parse_descriptor (_t(CONFIG_ENV_REGION), &env_d))
    return;
  if (env_d.driver->open (&env_d))
    env_d.driver->close (&env_d);
}

static __service_3 struct service_d env_service = { 
  .init = env_init,
};
#endif

