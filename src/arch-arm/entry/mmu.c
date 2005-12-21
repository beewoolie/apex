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

   ARM922
   ------

   o I-cache may be enabled without the MMU by setting the Icr bit in
     CP15.  Without the MMU, all instruction fetches are cacheable,
     addresses are linearly mapped, and no protection checks are
     made. 
   o D-cache can only be used with the MMU enabled as the
     cacheable/bufferable bits are defined in the MMU tables.
   o CP15/1 doesn't have a write-buffer enable bit.  Instead, the
     buffer is enabled by the MMU Btt bits for each page or section.
   o The Linux kernel doesn't use the write-through code for the 922.
     Not sure why.

*/

#include <config.h>
#include <debug_ll.h>
#include <mach/coprocessor.h>
#include <attributes.h>
#include <linux/string.h>
#include <service.h>
#include <apex.h>

#if !defined (COPROCESSOR_WAIT)
# define COPROCESSOR_WAIT
#endif

//#define CONFIG_HAVE_BCR	/* Have a Bcr bit in the control register */
//#define CONFIG_WT		/* Write-through */
#define CONFIG_WB		/* Write-back */

#define C_PTE			(1<<12)

unsigned long __xbss(ttbl) ttbl[C_PTE];

#define Icr   (1<<12)
#define Dcr   (1<<2)
#define MMUEN (1<<0)

/* mmu_init

*/

void mmu_init (void)
{
  int i;
  unsigned long domain;

//  memset (ttbl, 0, sizeof (ttbl)); /* All entries produce faults */

  /* Fill with 1:1 mapping sections */
  for (i = 0; i < C_PTE; ++i) {
    ttbl[i] = (i<<20) 
      | (3<<10)			/* AP(R/W) */
      | (0<<5)			/* domain(0) */
      | ((i >= 0xc00 && i <= 0xe00) 
#if defined (CONFIG_WB)
	 ? (3<<2)		/* CB(cacheable,buffered) */
#endif
#if defined (CONFIG_WT)
	 ? (2<<2)		/* CB(cacheable,unbuffered) */
#endif
	 : 0)			/* CB(uncacheable,unbuffered) */
      | (2<<0);			/* type(section) */
  }

  domain = 0xffffffff;
  __asm volatile ("mcr p15, 0, %0, c2, c0" : : "r" (ttbl));
  __asm volatile ("mcr p15, 0, %0, c3, c0" : : "r" (domain));

  __asm volatile ("mcr p15, 0, %0, c7, c5, 0" : : "r" (0));  // Flush I cache
  __asm volatile ("mcr p15, 0, %0, c7, c6, 0" : : "r" (0));  // Flush D cache
  __asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));  // Flush TLBs

	/* Enable MMU */
  { 
    unsigned long l;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t" 
		    /* D-cache, Write buffer, MMU enable */
		    "orr %0, %0, #("
#if defined (CONFIG_HAVE_BCR) && defined (CONFIG_WB)
		    "(1<<3)|"
#endif
		    "(1<<2)|(1<<0))\n\t"
		    "orr %0, %0, #(1<<12)\n\t"	  /* I-cache */
		    "mcr p15, 0, %0, c1, c0, 0" : "=r" (l));
  }
  COPROCESSOR_WAIT;
}

void mmu_protsegment (void* pv, int cacheable, int bufferable)
{
  int i = (unsigned long) (pv) >> 20;
  ttbl[i] = (i<<20) 
    | (3<<10)			/* AP(R/W) */
    | (0<<5)			/* domain(0) */
    | (bufferable ? (1<<2) : 0)
    | (cacheable  ? (1<<3) : 0)
    | (2<<0);			/* type(section) */
}

void mmu_release (void)
{

  __asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0)); // Drain buffer
  {
#define DLEN(l)		((l>>(12+0))&3)
#define DSIZE(l)	((l>>(12+6))&7)
#define DASSOC(l)	((l>>(12+3))&7)
#define DM(l)		((l>>(12+2))&1)
    unsigned long cache;
    int i, j;
    unsigned long l;
    int linelen;
    int assoc;
    __asm volatile ("mrc p15, 0, %0, c0, c0, 1" : "=r" (cache));
    linelen = DLEN(cache)+3;
    assoc = 32 - DASSOC(cache);
    /* *** FIXME: this docs are unclear about what to do with M==1 and
       cleaning. */
//    if (DM(l))
//      assoc = 3*assoc/2;
    for (i = 1<<(DSIZE(cache) + 6 - DASSOC(cache) - DLEN(cache)); i--; )
      for (j = 1<<DASSOC(cache); j--;) {
	l = (i<<linelen)|(j<<assoc);
	__asm volatile ("mcr p15, 0, %0, c7, c10, 2" : : "r" (l));  // Clean
      }
    
  }

	/* Disable MMU */
  {
    unsigned long l;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t" 
		    /* D-cache, Write buffer, MMU disable */
		    "bic %0, %0, #("
#if defined (CONFIG_HAVE_BCR) && defined (CONFIG_WB)
		    "(1<<3)|"
#endif
		    "(1<<2)|(1<<0))\n\t"
		    "bic %0, %0, #1<<12\n\t"	       /* I-cache */
		    "mcr p15, 0, %0, c1, c0, 0" : "=r" (l));
  }
  COPROCESSOR_WAIT;

  __asm volatile ("mcr p15, 0, %0, c7, c5, 0" : : "r" (0));  // Flush I cache
  __asm volatile ("mcr p15, 0, %0, c7, c6, 0" : : "r" (0));  // Flush D cache
  __asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));  // Flush TLBs
}

static __service_1 struct service_d mmu_service = {
  .init    = mmu_init,
  .release = mmu_release,
};
