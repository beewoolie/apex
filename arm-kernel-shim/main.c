/* main.c

   written by Marc Singer
   23 Jun 2006

   Copyright (C) 2006 Marc Singer

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

   ARM kernel bootstrap shim.

   This code may be prepended to a 2.6 Linux kernel in order to setup
   the ATAGS and the machine type in the EXTREME circumstance that the
   platform bootloader cannot be replaced.

*/

#define __KERNEL__

#include "config.h"
#include "types.h"		/* include/asm-arm/types.h */
#include "setup.h"		/* include/asm-arm/setup.h */

#define H_SIZE(pv)	(((struct tag_header*)pv)->size)
#define H_TAG(pv)	(((struct tag_header*)pv)->tag)

#define P_CORE(pv)	((struct tag_core*)(pv + sizeof (struct tag_header)))
#define P_MEM32(pv)	((struct tag_mem32*)(pv + sizeof (struct tag_header)))
#define P_CMDLINE(pv)  ((struct tag_cmdline*)(pv + sizeof (struct tag_header)))

#define NAKED		__attribute__((naked))

#if defined (COMMANDLINE)
const char __attribute__((section(".rodata"))) cmdline[] = COMMANDLINE;
#endif

void NAKED __attribute__((section(".boot"))) boot (u32 r0, u32 r1, u32 r2)
{
  __asm volatile (" nop");
}

int NAKED start (void)
{
  void* pv;
  extern char SHIM_VMA_END;

#if defined (FORCE_BIGENDIAN)

  {
    unsigned long v;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    "orr %0, %0, #(1<<7)\n\t" /* Switch to bigendian */
		    "mcr p15, 0, %0, c1, c0, 0" : "=&r" (v));
  }
#endif

#if defined (FORCE_LITTLEENDIAN)

  {
    unsigned long v;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    "bic %0, %0, #(1<<7)\n\t" /* Switch to littleendian */
		    "mcr p15, 0, %0, c1, c0, 0" : "=&r" (v));
  }
#endif

  pv = (void*) PHYS_PARAMS;

	/* Always start with the CORE tag */
  H_SIZE(pv)		= tag_size (tag_core);
  H_TAG(pv)		= ATAG_CORE;
  P_CORE(pv)->flags = 0;
  P_CORE(pv)->pagesize = 0;
  P_CORE(pv)->rootdev = 0;
  pv += H_SIZE(pv)*4;

	/* Memory tags are always second */
  H_SIZE(pv)		= tag_size (tag_mem32);
  H_TAG(pv)		= ATAG_MEM;
  P_MEM32(pv)->size	= RAM_BANK0_LENGTH;
  P_MEM32(pv)->start	= RAM_BANK0_START;
  pv += H_SIZE(pv)*4;

#if defined (RAM_BANK1_START)
  H_SIZE(pv)		= tag_size (tag_mem32);
  H_TAG(pv)		= ATAG_MEM;
  P_MEM32(pv)->size	= RAM_BANK1_LENGTH;
  P_MEM32(pv)->start	= RAM_BANK1_START;
  pv += H_SIZE(pv)*4;
#endif

	/* Command line */
#if defined (COMMANDLINE)
  H_SIZE(pv)		= tag_size(tag_cmdline) + (sizeof (cmdline)+1+3)/4;
  H_TAG(pv)		= ATAG_CMDLINE;
  {
    int i;
    for (i = 0; i < sizeof (cmdline); ++i)
      P_CMDLINE(pv)->cmdline[i] = cmdline[i];
  }
  pv += H_SIZE(pv)*4;
#endif

	/* End */
  H_SIZE(pv)		= 0;
  H_TAG(pv)		= ATAG_NONE;

	/* Pass control to the kernel */
  boot (0, MACH_TYPE, PHYS_PARAMS);
}
