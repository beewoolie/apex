/* cp15.h

   written by Marc Singer
   19 Dec 2005

   Copyright (C) 2005 Marc Singer

   This program is licensed under the terms of the GNU General Public
   License as published by the Free Software Foundation.  Please refer
   to the file debian/copyright for further details.

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

#define MMU_DISABLE\
  ({ unsigned long l;\
     __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"\
		     "bic %0, %0, #(1<<0)\n\t"\
		     "mcr p15, 0, %0, c1, c0, 0\n\t"\
		     : "=&r" (l)); } )

#define MMU_ENABLE\
  ({ unsigned long l;\
     __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"\
		     "orr %0, %0, #(1<<0)\n\t"\
		     "mcr p15, 0, %0, c1, c0, 0\n\t"\
		     : "=&r" (l)); } )

#define WAIT_FOR_INTERRUPT\
  __asm volatile ("mcr p15, 0, %0, c7, c0, 4\n\t" :: "r" (0));


/* ----- Architecture specific coprocessor support */

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
