/* cpuinfo.c
     $Id$

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

static __service_7 struct service_d cpuinfo_service = {
  .report = cpuinfo_report,
};

void target_report (void)
{
  extern int fSDRAMBoot;

  if (fSDRAMBoot)
    printf ("  ixp42x: *** No SDRAM init when APEX loaded into SDRAM.\n");
}

#endif
