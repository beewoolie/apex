/* init.c
     $Id$

   written by Marc Singer
   31 Oct 2004

   Copyright (C) 2004 The Buici Company

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

void init (void)
{
  init_services ();
  init_drivers ();
  cmd_version (0, NULL);	/* Signon */
  exec_monitor ();
}
