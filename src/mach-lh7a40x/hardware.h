/* hardware.h
     $Id$

   written by Marc Singer
   4 Dec 2004

   Copyright (C) 2004 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__HARDWARE_H__)
#    define   __HARDWARE_H__

/* ----- Includes */

#include <config.h>

#if defined (CONFIG_MACH_LH7A40X)
# include "lh7a40x.h"
#endif

#if defined (CONFIG_MACH_LPD7A400) || defined (CONFIG_MACH_LPD7A404)
# include "lpd7a40x.h"
#endif

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */



#endif  /* __HARDWARE_H__ */
