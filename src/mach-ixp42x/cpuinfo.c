/* cpuinfo.c
     $Id$

   written by Marc Singer
   3 Feb 2005

   Copyright (C) 2005 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/


#include <apex.h>
#include <config.h>
#include "hardware.h"
#include <service.h>

#if !defined (CONFIG_SMALL)

static void cpuinfo_report (void)
{
  unsigned long id;
  unsigned long ctrl;
  unsigned long cpsr;

  __asm volatile ("mrc p15,0,%0,c0,c0": "=r" (id));
  __asm volatile ("mrc p15,0,%0,c1,c0": "=r" (ctrl));
  __asm volatile ("mrs %0, cpsr": "=r" (cpsr));
  printf ("  cpu: id 0x%lx  ctrl 0x%lx  cpsr 0x%lx\n", id, ctrl, cpsr);
}

#endif


static __service_7 struct service_d cpuinfo_service = {
#if !defined (CONFIG_SMALL)
  .report = cpuinfo_report,
#endif
};
