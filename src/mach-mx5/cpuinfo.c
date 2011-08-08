/* cpuinfo.c

   written by Marc Singer
   25 Nov 2006

   Copyright (C) 2006 Marc Singer

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

#if !defined (CONFIG_SMALL)

static void cpuinfo_report (void)
{
  unsigned long id;
  unsigned long ctr;
  unsigned long ctrl;
  unsigned long auxctrl;
  unsigned long cpsr;
  unsigned long ttbl;
  unsigned long domain;
  unsigned long ccsidr;
  unsigned long clidr;
//  unsigned long csc = (CSC_PWRSR>>CSC_PWRSR_CHIPID_SHIFT);
  unsigned long l2cacheaux;
  unsigned long test;
  char* sz = "iMX51";

//  switch (csc & 0xf0) {
//  default  : sz = "lh?";     break;
//  case 0x00: sz = "lh7a400"; break;
//  case 0x20: sz = "lh7a404"; break;
//  }

  __asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r" (id));
  __asm volatile ("mrc p15, 0, %0, c0, c0, 1" : "=r" (ctr));
  __asm volatile ("mrc p15, 1, %0, c0, c0, 0" : "=r" (ccsidr));
  __asm volatile ("mrc p15, 1, %0, c0, c0, 1" : "=r" (clidr));
  __asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (ctrl));
  __asm volatile ("mrc p15, 0, %0, c1, c0, 1" : "=r" (auxctrl));
  __asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r" (ttbl));
  __asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r" (domain));
  __asm volatile ("mrc p15, 1, %0, c9, c0, 2" : "=r" (l2cacheaux));
  __asm volatile ("mrc p15, 0, %0, c15, c0, 0" : "=r" (test));
  __asm volatile ("mrs %0, cpsr"	   : "=r" (cpsr));
  printf ("  cpu:      id 0x%08lx   cache 0x%08lx    cpsr 0x%08lx\n"
	  "          ttbl 0x%08lx  domain 0x%08lx  chipid %s\n"
          "        ccsidr 0x%08lx   clidr 0x%08lx\n"
          "          ctrl 0x%08lx (%s)\n"
          "       auxctrl 0x%08lx  l2caux 0x%08lx\n"
	  "          cp15test 0x%04lx\n",
	  id, ctr, cpsr,
	  ttbl, domain, sz,
          ccsidr, clidr,
          ctrl, cp15_ctrl (ctrl),
          auxctrl, l2cacheaux,
          test & 0xffff);

#if 0
  {
    u16 wcr = __REG16 (PHYS_WDOG1 + 0x00);
    u16 wcr2 = __REG16 (PHYS_WDOG2 + 0x00);
    printf ("  wdog:    wcr 0x%04x  wcr 0x%04x\n", wcr, wcr2);
  }
#endif

#if 1
  {
    u32 dr[4]   = { GPIOx_DR(1),  GPIOx_DR(2),  GPIOx_DR(3),  GPIOx_DR(4) };
    u32 gdir[4] = { GPIOx_GDIR(1),GPIOx_GDIR(2),GPIOx_GDIR(3),GPIOx_GDIR(4) };
    printf ("  gpio:   1: dat 0x%08x  dir 0x%08x"
            "  3: dat 0x%08x  dir 0x%08x\n"
            "          2: dat 0x%08x  dir 0x%08x"
            "  4: dat 0x%08x  dir 0x%08x\n",
            dr[0], gdir[0],
            dr[2], gdir[2],
            dr[1], gdir[1],
            dr[3], gdir[3]);
  }
#endif
}

static __service_7 struct service_d cpuinfo_service = {
  .report = cpuinfo_report,

};

#endif
