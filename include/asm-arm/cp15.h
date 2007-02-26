/* cp15.h

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

   The macros here are for accessing registers and functions of the
   standard CP15 coprocessor.  These registers control the MMU and
   cache functions of most ARM microprocessors.

   Standard MACROS are here that will tend to be a superset of those
   supported by any given core.  Be careful to #undef macros that are
   not available on a given architecture.

*/

#if !defined (__CP15_H__)
#    define   __CP15_H__

/* ----- Includes */

#include <linux/types.h>

#define LOAD_CP15_ID(id)\
  __asm volatile ("mrc p15, 0, %0, c0, c0, 0" : "=r" (id))
#define LOAD_CP15_CACHE(id)\
  __asm volatile ("mrc p15, 0, %0, c0, c0, 1" : "=r" (cache))
#define LOAD_CP15_CTRL(id)\
  __asm volatile ("mrc p15, 0, %0, c1, c0, 0" : "=r" (ctrl))

#define STORE_TTB(a)\
  __asm volatile ("mcr p15, 0, %0, c2, c0, 0\n\t" :: "r" (a))
#define LOAD_TTB(a)\
  __asm volatile ("mrc p15, 0, %0, c2, c0, 0\n\t" :  "=r" (a))

#define STORE_DOMAIN(a)\
  __asm volatile ("mcr p15, 0, %0, c3, c0, 0\n\t" :: "r" (a))
#define LOAD_DOMAIN(a)\
  __asm volatile ("mrc p15, 0, %0, c3, c0, 0\n\t" :  "=r" (a))

	/* ---- Cache control */

#define INVALIDATE_ICACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 0\n\t" :: "r" (0))
#define INVALIDATE_ICACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 1\n\t" :: "r" (a))
#define INVALIDATE_ICACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c5, 2\n\t" :: "r" (i))

#define INVALIDATE_DCACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c6, 0\n\t" :: "r" (0))
#define INVALIDATE_DCACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c6, 1\n\t" :: "r" (a))
#define INVALIDATE_DCACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c6, 2\n\t" :: "r" (i))

#define INVALIDATE_CACHE\
  __asm volatile ("mcr p15, 0, %0, c7, c7, 0\n\t" :: "r" (0))
#define INVALIDATE_CACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c7, 1\n\t" :: "r" (a))
#define INVALIDATE_CACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c7, 2\n\t" :: "r" (i))

#define CLEAN_DCACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 1\n\t" :: "r" (a))
#define CLEAN_DCACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 2\n\t" :: "r" (i))
#define DRAIN_WRITE_BUFFER\
  __asm volatile ("mcr p15, 0, %0, c7, c10, 4\n\t" :: "r" (0))

#define CLEAN_CACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c11, 1\n\t" :: "r" (a))
#define CLEAN_CACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c11, 2\n\t" :: "r" (i))

#define PREFETCH_ICACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c13, 1\n\t" :: "r" (a))

#define CLEAN_INV_DCACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c14, 1\n\t" :: "r" (a))
#define CLEAN_INV_DCACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c14, 2\n\t" :: "r" (i))

#define CLEAN_INV_CACHE_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c7, c15, 1\n\t" :: "r" (a))
#define CLEAN_INV_CACHE_I(i)\
  __asm volatile ("mcr p15, 0, %0, c7, c15, 2\n\t" :: "r" (i))

	/* ---- Cache lockdown */

#define UNLOCK_CACHE\
  __asm volatile ("mcr p15, 0, %0, c9, c1, 1\n\t"\
		  "mcr p15, 0, %0, c9, c2, 1\n\t" :: "r" (0))

	/* ---- TLB control */

#define INVALIDATE_TLB\
  __asm volatile ("mcr p15, 0, %0, c8, c7, 0\n\t" :: "r" (0))
#define INVALIDATE_ITLB\
  __asm volatile ("mcr p15, 0, %0, c8, c5, 0\n\t" :: "r" (0))
#define INVALIDATE_ITLB_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c8, c5, 1\n\t" :: "r" (a))
#define INVALIDATE_DTLB\
  __asm volatile ("mcr p15, 0, %0, c8, c6, 0\n\t" :: "r" (0))
#define INVALIDATE_DTLB_VA(a)\
  __asm volatile ("mcr p15, 0, %0, c8, c6, 1\n\t" :: "r" (a))


	/* ----- Architecture specific functions */

#if defined (CONFIG_CPU_ARMV4)
# include <asm/cp15-armv4.h>
#endif

#if defined (CONFIG_CPU_XSCALE)
# include <asm/cp15-xscale.h>
#endif

#if defined (CONFIG_CPU_ARMV6)
# include <asm/cp15-armv6.h>
#endif

#if !defined (CP15_WAIT)
# define CP15_WAIT
#endif

#endif  /* __CP15_H__ */
