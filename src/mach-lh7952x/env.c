/* env.c
     $Id$

   written by Marc Singer
   7 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

   Environment for the LH79524.

*/

#include <environment.h>

static __env struct env_d e_cmdline = {
  .key = "cmdline",
  .default_value = "console=ttyAM0",
  .description = "Linux kernel boot command",
};

static __env struct env_d e_bootaddr = {
  .key = "bootaddr",
  .default_value = "0x20008000",
  .description = "Linux kernel start address",
};
