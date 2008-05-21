/* cpuinfo.c

   written by Marc Singer
   4 Feb 2005

   Copyright (C) 2005 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <apex.h>
#include <service.h>
#include <arch-arm.h>
#include "hardware.h"
#include <asm/cp15.h>

#if !defined (CONFIG_SMALL)

static void cpuinfo_report (void)
{
  unsigned long id;
  unsigned long cache;
  unsigned long ctrl;
  unsigned long cpsr;
  unsigned long ttbl;
  unsigned long domain;
  unsigned long csc = (CSC_PWRSR>>CSC_PWRSR_CHIPID_SHIFT);
  unsigned long test;
  char* sz = NULL;

  switch (csc & 0xf0) {
  default  : sz = "lh?";     break;
  case 0x00: sz = "lh7a400"; break;
  case 0x20: sz = "lh7a404"; break;
  }

  LOAD_CP15_ID (id);
  LOAD_CP15_CACHE (cache);
  LOAD_CP15_CTRL (ctrl);
  LOAD_TTB (ttbl);
  LOAD_DOMAIN (domain);
  __asm volatile ("mrc p15, 0, %0, c15, c0, 0" : "=r" (test));
  __asm volatile ("mrs %0, cpsr"	   : "=r" (cpsr));
  printf ("  cpu:      id 0x%08lx    ctrl 0x%08lx (%s)   cpsr 0x%08lx\n"
	  "          ttbl 0x%08lx  domain 0x%08lx  cache 0x%08lx\n"
	  "          chipid 0x%x %s  csc_clkset 0x%08lx\n"
	  "          cp15test 0x%04lx\n",
	  id, ctrl, cp15_ctrl (ctrl), cpsr,
	  ttbl, domain, cache, (unsigned) csc, sz, CSC_CLKSET,
	  test & 0xffff);

#if defined (CPLD_REVISION)
  printf ("  cpld:   revision 0x%x\n", CPLD_REVISION);
#endif
}

static __service_7 struct service_d cpuinfo_service = {
  .report = cpuinfo_report,

};

#endif
