/* mmu.c

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

   Any target that wants to use this feature should define a macro
   in mach-X/memory.h,

     PROTECTION_FOR(p)

   The return value is -1 to not map the segment (1MiB region of
   memory), Otherwise, return Ctt and or Btt. For example, SDRAM
   should be (3<<2), NOR flash should be 0.


   ARMV4 (ARM922 & ARM720)
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


   XSCALE
   ------

   o Uses a different method for cleaning cache than ARMV4.


*/

#include <config.h>
#include <mach/coprocessor.h>
#include <attributes.h>
#include <linux/string.h>
#include <service.h>
#include <apex.h>
#include <mach/memory.h>	/* protection_for() function/macro */
#include <debug_ll.h>
#include <asm/mmu.h>

#if !defined (COPROCESSOR_WAIT)
# define COPROCESSOR_WAIT
#endif

#if !defined (PROTECTION_FOR)
#warning "There's not much point in enabling the MMU without declaring a PROTECTION_FOR macro."
#define PROTECTION_FOR(p) (0)
#endif

#if 0
# define CONFIG_HAVE_BCR	/* Have a Bcr bit in the control register */
#endif

#define C_PTE			(1<<12)

unsigned long __xbss(ttbl) ttbl[C_PTE];

#define Icr   (1<<12)
#define Bcr   (1<<3)
#define Dcr   (1<<2)
#define Btt   (1<<2)
#define Ctt   (1<<3)
#define MMUEN (1<<0)


void mmu_cache_clean (void)
{
  CACHE_CLEAN;
}

void mmu_tlb_purge (void)
{
  TLB_PURGE;
}

/* mmu_init

   performs MMU initialization.  We don't do anything very complex
   here.  There is no protection.  The only thing we care about is
   enabling caches and the write buffer.

*/

void mmu_init (void)
{
  int i;
  unsigned long domain;

  /* Fill with 1:1 mapping sections */
  for (i = 0; i < C_PTE; ++i) {
    int protection = PROTECTION_FOR (i<<20);
    if (protection == -1) {
      ttbl[i] = 0;
      continue;
    }
    protection &= (Ctt | Btt);
#if defined (CONFIG_FORCE_WRITETHROUGH_DCACHE)
    if (protection == (Ctt | Btt))
      protection &= ~Btt;	/* Disable buffering of cached segments */
#endif
    ttbl[i] = (i<<20)
      | (3<<10)			/* AP(R/W) */
      | (0<<5)			/* domain(0) */
      | protection
      | (2<<0);			/* type(section) */
  }

  domain = 0xffffffff;
  __asm volatile ("mcr p15, 0, %0, c2, c0" : : "r" (ttbl));
  __asm volatile ("mcr p15, 0, %0, c3, c0" : : "r" (domain));

  __asm volatile ("mcr p15, 0, %0, c7, c5, 0" : : "r" (0));  // Inv. I cache
  __asm volatile ("mcr p15, 0, %0, c7, c6, 0" : : "r" (0));  // Inv. D cache
  __asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));  // Inv. TLBs
  COPROCESSOR_WAIT;

#if defined (CONFIG_CPU_XSCALE)
		/* The XScale core guide says to do this.  It isn't
		   clear why it should be necessary, but we oblige. */
  __asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0)); // Drain buffer
#endif

	/* Enable MMU */
  {
    unsigned long l;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    /* D-cache, Write buffer, MMU enable */
		    "orr %0, %0, #("
#if defined (CONFIG_HAVE_BCR) && !defined (CONFIG_FORCE_WRITETHROUGH_DCACHE)
		    "(1<<3)|"
#endif
		    "(1<<2)|(1<<0))\n\t"		/* D-cache, MMU-EN */
		    "orr %0, %0, #(1<<12)\n\t"		/* I-cache */
/* RR doesn't appear to make the boot faster. */
//		    "orr %0, %0, #(1<<14)\n\t"		/* RR, predictable */
		    "mcr p15, 0, %0, c1, c0, 0" : "=&r" (l));
  }
  COPROCESSOR_WAIT;
}


/* mmu_protsegment

   changes the protection for a region of memory given an address in
   the region.  With segments defined in the MMU tables, this means
   each 1MiB defines a section.

*/

void mmu_protsegment (void* pv, int cacheable, int bufferable)
{
	/* Convert address to table index/segment # */
  int i = (unsigned long) (pv) >> 20;

  ttbl[i] = (i<<20)
    | (3<<10)			/* AP(R/W) */
    | (0<<5)			/* domain(0) */
    | (bufferable ? Btt : 0)
    | (cacheable  ? Ctt : 0)
    | (2<<0);			/* type(section) */
}


/* mmu_release

   performs the work to switch the MMU off before transferring to
   another piece of code, Linux kernel, alternative boot loader.

*/

void mmu_release (void)
{

  __asm volatile ("mcr p15, 0, %0, c7, c10, 4" : : "r" (0)); // Drain buffer

  CACHE_CLEAN;

	/* Disable MMU */
  {
    unsigned long l;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    /* D-cache, Write buffer, MMU disable */
		    "bic %0, %0, #("
#if defined (CONFIG_HAVE_BCR) && !defined (CONFIG_FORCE_WRITHROUGH_DCACHE)
		    "(1<<3)|"
#endif
		    "(1<<2)|(1<<0))\n\t"		/* D-cache, MMU-EN */
		    "bic %0, %0, #1<<12\n\t"		/* I-cache */
		    "mcr p15, 0, %0, c1, c0, 0" : "=&r" (l));
  }
  COPROCESSOR_WAIT;

  __asm volatile ("mcr p15, 0, %0, c7, c5, 0" : : "r" (0));  // Inv. I cache
  __asm volatile ("mcr p15, 0, %0, c7, c6, 0" : : "r" (0));  // Inv. D cache
  __asm volatile ("mcr p15, 0, %0, c8, c7, 0" : : "r" (0));  // Inv. TLBs
  COPROCESSOR_WAIT;

  __asm volatile ("mcr p15, 0, %0, c2, c0" : : "r" (0)); /* Clear ttbl */
}

static __service_1 struct service_d mmu_service = {
  .init    = mmu_init,
  .release = mmu_release,
};
