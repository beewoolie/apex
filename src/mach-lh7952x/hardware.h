/* hardware.h
     $Id$

   written by Marc Singer
   15 Nov 2004

   Copyright (C) 2004 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__HARDWARE_H__)
#    define   __HARDWARE_H__

/* ----- Includes */

#include <config.h>

#if defined (CONFIG_INTERRUPTS)
extern void (*interrupt_handlers[32])(void);
#endif

#if defined (CONFIG_MACH_LH79520)
# include "lh79520.h"
#endif

#if defined (CONFIG_MACH_LH79524)
# include "lh79524.h"
#endif

#if defined (CONFIG_MACH_LPD79520)
# include "lpd79520.h"
#endif

#if defined (CONFIG_MACH_LPD79524)
# include "lpd79524.h"
#endif

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */



#endif  /* __HARDWARE_H__ */
