/* hardware.h
     $Id$

   written by Marc Singer
   14 Jan 2005

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
# include "nslu2.h"
#endif

#if defined (CONFIG_DEBUG_LL)
# include "debug_ll.h"
#endif

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#endif  /* __HARDWARE_H__ */
