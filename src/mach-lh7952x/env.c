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
  .default_value = "console=ttyAM0 root=/dev/hda1",
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
