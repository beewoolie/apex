/* env.c

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

#if defined (CONFIG_KERNEL_LMA)
static __env struct env_d e_bootaddr = {
  .key = "bootaddr",
  .default_value = _t(CONFIG_KERNEL_LMA),
  .description = "Linux start address",
};
#endif

#if (defined (CONFIG_ENV_REGION_KERNEL) && defined (CONFIG_KERNEL_LMA))\
    || defined (CONFIG_AUTOBOOT)
static __env struct env_d e_startup = {
  .key = "startup",
  .default_value =
#if defined (CONFIG_ENV_REGION_KERNEL) && defined (CONFIG_KERNEL_LMA)
    "copy " CONFIG_ENV_REGION_KERNEL " " _t(CONFIG_KERNEL_LMA) ";"
#endif
#if defined (CONFIG_ENV_REGION_RAMDISK) && defined (CONFIG_RAMDISK_LMA)
    "copy " CONFIG_ENV_REGION_RAMDISK " " _t(CONFIG_RAMDISK_LMA) ";"
#endif
#if defined (CONFIG_ENV_STARTUP)
    CONFIG_ENV_STARTUP
#endif
#if defined (CONFIG_AUTOBOOT)
# if CONFIG_AUTOBOOT_DELAY != 0
    "wait " _t(CONFIG_AUTOBOOT_DELAY) " Type ^C key to cancel autoboot.;"
# endif
    "boot"
#endif
  ,
  .description = "Startup commands",
};
#endif

#if defined (CONFIG_ENV_REGION)
extern struct descriptor_d env_d;

# if defined (CONFIG_ENV_LINK)

extern char APEX_ENV_START;
extern char APEX_ENV_END;
extern char APEX_VMA_COPY_START;
extern char APEX_VMA_COPY_END;

static __section (.envlink) struct env_link env_link = {
  ENV_LINK_MAGIC,
  &APEX_VMA_COPY_START,		/* Immutable portion of APEX */
  &APEX_VMA_COPY_END,
  &APEX_ENV_START,
  &APEX_ENV_END,
  sizeof (struct env_d),
  CONFIG_ENV_REGION };

#  define ENV_REGION_STRING env_link.region
# else
#  define ENV_REGION_STRING CONFIG_ENV_REGION
# endif

static void env_init (void)
{
  if (parse_descriptor (ENV_REGION_STRING, &env_d))
    return;
  if (env_d.driver->open (&env_d))
    env_d.driver->close (&env_d);
}

static __service_7 struct service_d env_service = {
  .init = env_init,
};
#endif
