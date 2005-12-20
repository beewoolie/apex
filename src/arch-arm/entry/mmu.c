/* mmu.c
     $Id$

   written by Marc Singer
   19 Dec 2005

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

   Code to enable and disable the MMU.  Enabling the MMU means that we
   can use the cache.

*/

#include <config.h>
//#include <asm/bootstrap.h>
#include <debug_ll.h>
#include <mach/coprocessor.h>
#include <attributes.h>
#include <linux/string.h>
#include <service.h>

#if !defined (COPROCESSOR_WAIT)
# define COPROCESSOR_WAIT
#endif

unsigned long __xbss(ttbl) ttbl[1<<10];


/* mmu_init

*/

void mmu_init (void)
{
  int i;
  unsigned long domain;

//  memset (ttbl, 0, sizeof (ttbl)); /* All entries produce faults */

  /* Fill with 1:1 mapping sections */
  for (i = 0; i < (1<<12); ++i) {
    ttbl[i] = (i<<20) 
      | (3<<10)			/* AP(R/W) */
      | (0<<5)			/* domain(0) */
      | ((i >= 0xc00 && i <= 0xe00) 
	 ? (3<<2)		/* CB(cacheable,buffered) */
	 : 0)			/* CB(uncacheable,unbuffered) */
      | (2<<0);			/* type(section) */
  }

  domain = 0xffffffff;
  __asm volatile ("mcr p15, 0, %0, c2, c0" : : "r" (ttbl));
  __asm volatile ("mcr p15, 0, %0, c3, c0" : : "r" (domain));

	/* Enable MMU */
  { 
    unsigned long l;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t" 
		    "orr %0, %0, #((1<<3)|(1<<2)|(1<<0))\n\t" /* cache & buffer */
		    "orr %0, %0, #(1<<12)\n\t"	  /* i-cache */
		    "mcr p15, 0, %0, c1, c0, 0" : "=r" (l));
  }
}

void mmu_release (void)
{
  __asm volatile ("mcr p15, 0, %0, c7, c5, 0" : : "r" (0)); // flush I cache
  COPROCESSOR_WAIT;
  __asm volatile ("mcr p15, 0, %0, c7, c6, 0" : : "r" (0)); // flush D cache
  COPROCESSOR_WAIT;
  __asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0)); // Drain buffer
  COPROCESSOR_WAIT;
  __asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0)); // Flush TLBs
  COPROCESSOR_WAIT;

	/* Disable MMU */
  {
    unsigned long l;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t" 
		    "bic %0, %0, #((1<<3)|(1<<2)|(1<<0))\n\t" /* cache & buffer */
		    "bic %0, %0, #(1<<12)\n\t"	       /* i-cache */
		    "mcr p15, 0, %0, c1, c0, 0" : "=r" (l));
  }
}

static __service_1 struct service_d mmu_service = {
  .init    = mmu_init,
  .release = mmu_release,
};
