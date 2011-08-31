/* cmd-info.c

   written by Marc Singer
   10 June 2005

   Copyright (C) 2005 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   The info command shows information about services.

*/

#include <config.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <apex.h>
#include <command.h>
#include <driver.h>
#include <service.h>
#include <error.h>
#include <spinner.h>


#if !defined (CONFIG_SMALL)
//# define USE_COPY_VERIFY	/* Define to include verify feature */
#endif

/** display information about a descriptor or a service.  First we
    check for a valid and parseable descriptor.  If there is one we
    pass the descriptor to the driver to describe.  For non-regions it
    searches for partial or complete matches against services and
    shows the report() for every one. */
int cmd_info (int argc, const char** argv)
{
  int result = 0;
  bool all = false;
  const char* name = argv[1];

  /* Show names of services */
  if (argc == 1) {
    {
      extern char APEX_SERVICE_START[];
      extern char APEX_SERVICE_END[];
      struct service_d* d;
      for (d = (struct service_d*) APEX_SERVICE_START;
           d < (struct service_d*) APEX_SERVICE_END;
           ++d) {
        if (!d->report)
          continue;
        if (!d->name) {
          printf (" srv %p\n", d);
          continue;
        }
        printf (" %-*.*s - %s\n", 16, 16, d->name,
                d->description ? d->description : "?");
      }
    }

    return 0;
  }


  if (argc != 2)
    return ERROR_PARAM;

  all = strcmp (name, ".") == 0;

  {
    extern char APEX_SERVICE_START[];
    extern char APEX_SERVICE_END[];
    struct service_d* service = NULL;
    struct service_d* service_match = NULL;
    int cb = strlen (name);

    result = 0;

    for (service = (struct service_d*) APEX_SERVICE_START;
         service < (struct service_d*) APEX_SERVICE_END;
         ++service) {
      if (service->name && (all || strnicmp (name, service->name, cb) == 0)) {
        if (service->report) {
          service_match = service;
          service->report ();
        }
      }
    }
    if (!service_match)
      result = ERROR_UNSUPPORTED;
  }


  if (result) {
    printf ("Unable to find service '%s'\n", argv[1]);
    goto fail;
  }

 fail:
  return result;
}

static __command struct command_d c_info = {
  .command = "info",
  .func = cmd_info,
  COMMAND_DESCRIPTION ("display information about services")
  COMMAND_HELP(
"info"
" [SRC]\n"
"  Display information about a service.  Without SRC the command\n"
"  lists services.  Use the SRC '.' to show information about all\n"
"  services.\n"
"  e.g.  info cpu\n"
"        info .\n"
  )
};
