/* init.c

   written by Marc Singer
   31 Oct 2004

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

#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <apex.h>
#include <command.h>
#include <service.h>
#include <debug_ll.h>

#if defined (CONFIG_CMD_ALIAS)
#include <alias.h>
#endif
extern int cmd_version (int, const char**);

void init_services (void)
{
  extern char APEX_SERVICE_START;
  extern char APEX_SERVICE_END;
  struct service_d* service;
  int i = 0;

  PUTC_LL ('S');

  for (service = (struct service_d*) &APEX_SERVICE_START;
       service < (struct service_d*) &APEX_SERVICE_END;
       ++service, ++i) {
    PUTC_LL (i%8 + '0');
    PUTC_LL (',');
    PUTHEX_LL (&service->init);
    PUTC_LL (',');
    PUTHEX_LL (service->init);
    PUTC_LL ('\r');
    PUTC_LL ('\n');
    if (service->init)
      service->init ();
    PUTC_LL ('\r');
    PUTC_LL ('\n');
  }
  PUTC_LL ('s');
//  printf ("init complete\n");
}

void release_services (void)
{
  extern char APEX_SERVICE_START;
  extern char APEX_SERVICE_END;
  struct service_d* service;
  int i = 0;

  PUTC_LL ('S');

  for (service = (struct service_d*) &APEX_SERVICE_END;
       service-- > (struct service_d*) &APEX_SERVICE_START; ++i) {
    PUTC_LL (i%8 + '0');
    PUTC_LL (',');
    PUTHEX_LL (&service->release);
    PUTC_LL (',');
    PUTHEX_LL (service->release);
    PUTC_LL ('\r');
    PUTC_LL ('\n');
    if (service->release)
      service->release ();
  }
  PUTC_LL ('s');
}

void init (void)
{
  PUTC_LL ('I');
  init_services ();


#if defined (CONFIG_ALIASES)
  {
    extern char APEX_VMA_COPY_START;
    extern char APEX_VMA_COPY_END;
    char sz[80];
    sprintf (sz, "mem:0x%p+0x%lx",
	     (void*) &APEX_VMA_COPY_START,
	     (unsigned long )(&APEX_VMA_COPY_END - &APEX_VMA_COPY_START));
    alias_set ("apex", sz);
# if defined (CONFIG_ENV_REGION)
    alias_set ("env", CONFIG_ENV_REGION);
# endif
  }
#endif

  cmd_version (-1, NULL);	/* Signon */
  PUTC_LL ('i');
  exec_monitor ();
}
