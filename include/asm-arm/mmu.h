/* mmu.h

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

*/

#if !defined (__MMU_H__)
#    define   __MMU_H__

/* ----- Includes */

#include <linux/types.h>

#if defined (CONFIG_CPU_ARMV4)
# include <asm/mmu-armv4.h>
#endif

#if defined (CONFIG_CPU_XSCALE)
# include <asm/mmu-xscale.h>
#endif

#include <mach/coprocessor.h>

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#define TLB_PURGE\
  __asm volatile ("mcr p15, 0, %0, c8, c7, 0\n\t" :: "r" (0))
#define TLB_I_INVALIDATE(a)\
    __asm volatile ("mcr p15, 0, %0, c8, c5, 1\n\t" :: "r" (a))
#define TLB_D_INVALIDATE(a)\
    __asm volatile ("mcr p15, 0, %0, c8, c6, 1\n\t" :: "r" (a))

#if !defined (CACHE_DRAIN_D)
# define CACHE_DRAIN_D
#endif

#if !defined (CACHE_FLUSH)
# define CACHE_FLUSH
#endif

#if !defined (CACHE_UNLOCK)
# define CACHE_UNLOCK
#endif

void* alloc_uncached (size_t cb, size_t alignment);
void* alloc_uncached_top_retain (size_t cb, size_t alignment);
//void mmu_cache_clean (void);
//void mmu_tlb_purge (void);
void mmu_protsegment (void* pv, int cacheable, int bufferable);

#endif  /* __MMU_H__ */
