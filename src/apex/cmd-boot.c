/* cmd-boot.c
     $Id$

   written by Marc Singer
   6 Nov 2004

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

   *** NOTE: this function should be part of the arch-arm code since
   *** it uses the arm specific calling convention to the kernel.

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

#if defined (CONFIG_ATAG) 
# include <atag.h>
static int commandline_argc;
static const char** commandline_argv;

extern void build_atags (void);
#endif

#if defined (CONFIG_ARCH_NUMBER_FUNCTION)
#endif

int cmd_boot (int argc, const char** argv)
{
  unsigned long address = env_fetch_int ("bootaddr", 0xffffffff);
  int arch_number = -1;

  if (argc > 2 && strcmp (argv[1], "-g") == 0) {
    address = simple_strtoul (argv[2], NULL, 0);
    argc -= 2;
    argv += 2;
  }

  if (address == 0xffffffff) {
    printf ("Kernel address required.\n");
    return 0;
  }

#if defined (CONFIG_ARCH_NUMBER_FUNCTION)
  {
    extern int CONFIG_ARCH_NUMBER_FUNCTION (void);
    arch_number = CONFIG_ARCH_NUMBER_FUNCTION ();
  }
#endif

#if defined (CONFIG_ARCH_NUMBER)
  arch_number = CONFIG_ARCH_NUMBER;
#endif

#if defined (CONFIG_ATAG)
  commandline_argc = argc - 1;
  commandline_argv = argv + 1;

  build_atags ();
#endif

  printf ("Booting kernel at 0x%p...\n", (void*) address);

  //serial_flush_output();

  release_services ();

  ((void (*)(int, int, int)) address) 
    (0, arch_number, CONFIG_ATAG_PHYS);

  printf ("Uh, oh.  Linux returned.\n");

  return 0;
}

static __command struct command_d c_boot = {
  .command = "boot",
  .description = "boot Linux",
  .func = cmd_boot,
};

#if defined (CONFIG_ATAG)
struct tag* atag_commandline (struct tag* p)
{
  const char* szEnv = env_fetch ("cmdline");

  int cb = 0;
  p->u.cmdline.cmdline[0] = '\0';

  if (commandline_argc > 0) {
    char* pb = p->u.cmdline.cmdline;
    int i;

    for (i = 0; i < commandline_argc; ++i) {
      strlcpy (pb, commandline_argv[i], COMMAND_LINE_SIZE);
      pb += strlen (pb);
      *pb++ = ' ';
    }
	
    pb--;
    *pb++ = 0;

    cb = pb - p->u.cmdline.cmdline;
  }
  else if (szEnv) {
    strlcpy (p->u.cmdline.cmdline, szEnv, COMMAND_LINE_SIZE);
    cb = strlen (szEnv) + 1;
  }

  if (cb) {
//    printf ("cmdline '%s'\n", p->u.cmdline.cmdline);
    p->hdr.tag = ATAG_CMDLINE;
    p->hdr.size
      = (sizeof (struct tag_header) + cb + 4) >> 2;
    p = tag_next (p);
  }    

  return p;
}

static __atag_1 struct atag_d _atag_commandline = { atag_commandline };

#endif
