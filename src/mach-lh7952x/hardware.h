/* hardware.h

   written by Marc Singer
   15 Nov 2004

   Copyright (C) 2004 Marc Singer

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

#if !defined (__HARDWARE_H__)
#    define   __HARDWARE_H__

/* ----- Includes */

#include <config.h>

#if defined (CONFIG_INTERRUPTS)
//#define IRQ_COUNT	32
#endif

#if defined (CONFIG_ARCH_LH79520)
# include "lh79520.h"
#endif

#if defined (CONFIG_ARCH_LH79524) || defined (CONFIG_ARCH_LH79525)
# include "lh79524.h"
#endif

#if defined (CONFIG_MACH_LNODE80)
# include "lnode80.h"
#endif

#if defined (CONFIG_MACH_LPD79520)
# include "lpd79520.h"
#endif

#if defined (CONFIG_MACH_LPD79524)
# include "lpd79524.h"
#endif

#if defined (CONFIG_MACH_KEV79524) || defined (CONFIG_MACH_KEV79525)
# include "kev79524.h"
#endif

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */



#endif  /* __HARDWARE_H__ */
