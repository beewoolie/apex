/* init.c
     $Id$

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
#include <apex.h>
#include <command.h>
#include <service.h>

extern int cmd_version (int, const char**);

static void init_services (void)
{
  extern char APEX_SERVICE_START;
  extern char APEX_SERVICE_END;
  struct service_d* service;

  for (service = (struct service_d*) &APEX_SERVICE_START;
       service < (struct service_d*) &APEX_SERVICE_END;
       ++service)
    if (service->init)
      service->init ();
}

void release_services (void)
{
  extern char APEX_SERVICE_START;
  extern char APEX_SERVICE_END;
  struct service_d* service;

  for (service = (struct service_d*) &APEX_SERVICE_END;
       service-- > (struct service_d*) &APEX_SERVICE_START; )
    if (service->release)
      service->release ();
}

void init (void)
{
  init_services ();
  cmd_version (0, NULL);	/* Signon */
  exec_monitor ();
}
