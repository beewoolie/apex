/* cpuinfo.c

   written by Marc Singer
   3 Feb 2005

   Copyright (C) 2005 Marc Singer

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
#include <apex.h>
#include <linux/string.h>
#include "hardware.h"
#include <service.h>
#include <arch-arm.h>

#if !defined (CONFIG_SMALL)

static void cpuinfo_report (void)
{
  unsigned long id;
  unsigned long cache;
  unsigned long ctrl;
  unsigned long ttbl;
  unsigned long domain;
  unsigned long cpsr;

  __asm volatile ("mrc p15, 0, %0, c0, c0": "=r" (id));
  __asm volatile ("mrc p15, 0, %0, c0, c0, 1" : "=r" (cache));
  __asm volatile ("mrc p15, 0, %0, c1, c0": "=r" (ctrl));
  __asm volatile ("mrc p15, 0, %0, c2, c0, 0" : "=r" (ttbl));
  __asm volatile ("mrc p15, 0, %0, c3, c0, 0" : "=r" (domain));
  __asm volatile ("mrs %0, cpsr": "=r" (cpsr));
  printf ("  cpu:      id 0x%08lx  ctrl   0x%lx (%s)  cpsr 0x%lx\n"
	  "          ttbl 0x%08lx  domain 0x%08lx  cache 0x%08lx\n",
	  id, ctrl, cp15_ctrl (ctrl), cpsr, ttbl, domain, cache);
}

static __service_7 struct service_d cpuinfo_service = {
  .report = cpuinfo_report,
};

#endif
