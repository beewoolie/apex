/* cmd-version.c
     $Id$

   written by Marc Singer
   3 Nov 2004

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

*/

#include <config.h>
#include <linux/types.h>
#include <apex.h>
#include <command.h>
#include <service.h>

extern char APEX_VMA_COPY_START;
extern char APEX_VMA_COPY_END;

#define _s(v) #v 
#define _t(v) _s(v)

int cmd_version (int argc, const char** argv)
{
  printf (
"\n\nAPEX Boot Loader " APEXRELEASE
" -- Copyright (c) 2004 Marc Singer\n\n"
"APEX comes with ABSOLUTELY NO WARRANTY."
#if !defined (CONFIG_SMALL)
"  It is free software and you\n"
"are welcome to redistribute it under certain circumstances.\n"
"For details, refer to the file COPYING in the program source."
#endif
"\n\n"
"  apex => mem:0x%p+0x%lx\n"
	  ,
	  (void*) &APEX_VMA_COPY_START,
	  (unsigned long )(&APEX_VMA_COPY_END - &APEX_VMA_COPY_START));
#if defined (CONFIG_ENV_REGION)
  printf ("  env  => %s\n", _t(CONFIG_ENV_REGION));
#endif

#if !defined (CONFIG_SMALL)
  {
    extern char APEX_SERVICE_START;
    extern char APEX_SERVICE_END;
    struct service_d* service;

    for (service = (struct service_d*) &APEX_SERVICE_START;
	 service < (struct service_d*) &APEX_SERVICE_END;
	 ++service)
      if (service->report)
	service->report ();
  }
#endif
  putchar ('\n');

#if defined (CONFIG_ALLHELP)
  printf ("Use the 'help .' command to get a list of help topics.\n\n");
#endif

  return 0;
}

static __command struct command_d c_version = {
  .command = "version",
  .description = "show version and copyright",
  .func = cmd_version,
};
