/* cpuinfo.c
     $Id$

   written by Marc Singer
   4 Feb 2005

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
#include <service.h>
#include "hardware.h"

#if !defined (CONFIG_SMALL)

static void cpuinfo_report (void)
{
  unsigned long id;
  unsigned long ctrl;
  unsigned long cpsr;
  unsigned long csc = (CSC_PWRSR>>CSC_PWRSR_CHIPID_SHIFT);
  char* sz = NULL;

  switch (csc & 0xf0) {
  default  : sz = "lh?";     break;
  case 0x00: sz = "lh7a400"; break;
  case 0x20: sz = "lh7a404"; break;
  }

  __asm volatile ("mrc p15, 0, %0, c0, c0" : "=r" (id));
  __asm volatile ("mrc p15, 0, %0, c1, c0" : "=r" (ctrl));
  __asm volatile ("mrs %0, cpsr"	   : "=r" (cpsr));
  printf ("  cpu:    id 0x%lx  ctrl 0x%lx  cpsr 0x%lx\n"
	  "          chipid 0x%x %s\n",
	  id, ctrl, cpsr, (unsigned) csc, sz);

#if defined (CPLD_REVISION)
  printf ("  cpld:   revision 0x%x\n", CPLD_REVISION);
#endif
}

static __service_7 struct service_d cpuinfo_service = {
  .report = cpuinfo_report,

};
#endif
