/* sdramboot.c

   written by Marc Singer
   30 Sep 2005

   Copyright (C) 2005 Marc Singer

   This program is licensed under the terms of the GNU General Public
   License as published by the Free Software Foundation.  Please refer
   to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   This is code to support a message telling the user that SDRAM has
   not been initialized.

*/

#include <config.h>
#include <apex.h>
#include <service.h>
#include <attributes.h>
#include <sdramboot.h>

int __xbss(init) fSDRAMBoot;

void sdramboot_report (void)
{
  if (fSDRAMBoot)
    printf ("          *** No SDRAM init when APEX executed from SDRAM.\n");
}

static __service_0 struct service_d sdramboot_service = {
  .report = sdramboot_report,
};
