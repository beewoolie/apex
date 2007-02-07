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
  struct tag* p;
//  void* pv;
  //  extern char SHIM_VMA_END;

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

  p = (struct tag*) PHYS_PARAMS;

	/* Always start with the CORE tag */
  p->hdr.tag		= ATAG_CORE;
  p->hdr.size		= tag_size (tag_core);
  p->u.core.flags	= 0;
  p->u.core.pagesize	= 0;
  p->u.core.rootdev	= 0;
  p = tag_next (p);

	/* Memory tags are always second */
  p->hdr.tag		= ATAG_MEM;
  p->hdr.size		= tag_size (tag_mem32);
  p->u.mem.size		= RAM_BANK0_LENGTH;
  p->u.mem.start	= RAM_BANK0_START;
  p = tag_next (p);

#if defined (RAM_BANK1_START)
  p->hdr.tag		= ATAG_MEM;
  p->hdr.size		= tag_size (tag_mem32);
  p->u.mem.size		= RAM_BANK1_LENGTH;
  p->u.mem.start	= RAM_BANK1_START;
  p = tag_next (p);
#endif

#if defined (INITRD_START)
  p->hdr.tag		= ATAG_INITRD2;
  p->hdr.size		= tag_size (tag_initrd);
  p->u.initrd.start	= INITRD_START;
  p->u.initrd.size	= INITRD_LENGTH;
  p = tag_next (p);
#endif

	/* Command line */
#if defined (COMMANDLINE)
  p->hdr.tag		= ATAG_CMDLINE;
  p->hdr.size		= tag_size (tag_cmdline) + (sizeof (cmdline)+1+3)/4;
  {
    int i;
    for (i = 0; i < sizeof (cmdline); ++i)
      p->u.cmdline.cmdline[i] = cmdline[i];
  }
  p = tag_next (p);
#endif

	/* End */
  p->hdr.tag		= ATAG_NONE;
  p->hdr.size		= 0;

	/* Pass control to the kernel */
  boot (0, MACH_TYPE, PHYS_PARAMS);
}
