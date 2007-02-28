/* nor-cfi.h
     $Id$

   written by Marc Singer
   26 Feb 2007

   Copyright (C) 2007 Marc Singer

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

#if !defined (__NOR_CFI_H__)
#    define   __NOR_CFI_H__

/* ----- Includes */

#include <config.h>
#include <mach/hardware.h>

/* ----- Constants */

#undef CONFIG_DRIVER_NOR_CFI_TYPE_INTEL
#undef CONFIG_DRIVER_NOR_CFI_TYPE_SPANSION
#define CONFIG_DRIVER_NOR_CFI_TYPE_SPANSION

#if !defined (NOR_WIDTH)
# define NOR_WIDTH		(16)
#endif
#if !defined (NOR_0_PHYS)
# define NOR_0_PHYS		(0xa0000000)
#endif

#if !defined (NOR_CHIP_MULTIPLIER)
# define NOR_CHIP_MULTIPLIER	(1)	/* Number of chips at REGA */
#endif

#endif  /* __NOR_CFI_H__ */
