/** @file env.c

   written by Marc Singer
   30 Jun 2011

   Copyright (C) 2011 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   Environment for the MX51 targets.

*/

#include <config.h>
#include <environment.h>
#include <driver.h>
#include <service.h>

#if ! defined (CONFIG_ENV_DEFAULT_CMDLINE)

__env struct env_d e_cmdline = {
  .key = "cmdline",
  .default_value = "console=ttymxc0,tty1 root=/dev/sda2 rootwait rw quiet splash",
  .description = "Linux kernel command line",
};

#endif
