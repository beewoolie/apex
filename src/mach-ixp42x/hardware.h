/* hardware.h
     $Id$

   written by Marc Singer
   14 Jan 2005

   Copyright (C) 2005 Marc Singer

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

#if defined (CONFIG_MACH_IXP42X)
# include "ixp42x.h"
#endif

#if defined (CONFIG_MACH_NSLU2)
/* *** not yet needed */
//# include "nslu2.h"
#endif

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#endif  /* __HARDWARE_H__ */
