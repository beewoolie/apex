/* cmd_version.c
     $Id$

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <linux/types.h>
#include <apex.h>
#include <command.h>

extern char APEX_VMA_COPY_START;
extern char APEX_VMA_COPY_END;

int cmd_version (int argc, const char** argv)
{
  printf ("\r\n\nAPEX Boot Loader " APEXRELEASE "  mem:@0x%p#0x%lx\r\n"
	  "Copyright (c) 2004, Marc Singer\r\n\n",
	  (void*) &APEX_VMA_COPY_START,
	  (unsigned long )(&APEX_VMA_COPY_END - &APEX_VMA_COPY_START));
  return 0;
}

static __command struct command_d c_version = {
  .command = "version",
  .description = "show version and copyright",
  .func = cmd_version,
};
