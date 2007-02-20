/* mmu.h
     $Id$

   written by Marc Singer
   13 Jan 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__MMU_H__)
#    define   __MMU_H__

/* ----- Includes */

#if defined (CONFIG_CPU_ARMV4)
# include "mmu-armv4.h"
#endif

#if defined (CONFIG_CPU_XSCALE)
# include "mmu-xscale.h"
#endif

#define TLB_PURGE\
  __asm volatile ("mcr p15, 0, %0, c8, c7, 0\n\t" :: "r" (0))

#if !defined (CACHE_FLUSH)
# define CACHE_FLUSH
#endif

#if !defined (CACHE_UNLOCK)
# define CACHE_UNLOCK
#endif

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */



#endif  /* __MMU_H__ */
