/* cmd-boot.c
     $Id$

   written by Marc Singer
   6 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <linux/types.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include <error.h>
#include <linux/kernel.h>
#include <config.h>
#include <environment.h>

#if defined (CONFIG_ATAG) 
# include <atag.h>
static int commandline_argc;
static const char** commandline_argv;

extern void build_atags (void);
#endif

int cmd_boot (int argc, const char** argv)
{
  unsigned long address = env_fetch_int ("bootaddr", 0xffffffff);

  if (argc > 2 && strcmp (argv[1], "-g") == 0) {
    address = simple_strtoul (argv[2], NULL, 0);
    argc -= 2;
    argv += 2;
  }

  if (address == 0xffffffff) {
    printf ("No kernel start address.\r\n");
    return 0;
  }

#if defined (CONFIG_ATAG)
  commandline_argc = argc - 1;
  commandline_argv = argv + 1;

  build_atags ();
#endif

  printf ("Booting kernel at 0x%p...\r\n", (void*) address);

  //serial_flush_output();
  //exit_subsystems();

  ((void (*)(int, int, int)) address) 
    (0, CONFIG_ARCH_NUMBER, CONFIG_ATAG_PHYS);

  printf ("Uh, oh.  Linux returned.\r\n");

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

  if (szEnv) {
    strlcpy (p->u.cmdline.cmdline, szEnv, COMMAND_LINE_SIZE);
    cb = strlen (szEnv) + 1;
  }
  else {
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
  }

  if (cb) {
//    printf ("cmdline '%s'\r\n", p->u.cmdline.cmdline);
    p->hdr.tag = ATAG_CMDLINE;
    p->hdr.size
      = (sizeof (struct tag_header) + cb + 4) >> 2;
    p = tag_next (p);
  }    

  return p;
}

static __atag_1 struct atag_d _atag_commandline = { atag_commandline };

#endif
