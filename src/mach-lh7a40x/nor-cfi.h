/* nor-cfi.h
     $Id$

   written by Marc Singer
   9 Mar 2005

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

#if !defined (__NOR_CFI_H__)
#    define   __NOR_CFI_H__

/* ----- Includes */

#include <config.h>
#include <mach/hardware.h>

/* ----- Constants */

#define NOR_WIDTH	(32)
#define NOR_0_PHYS	(0x00000000)
#define NOR_0_LENGTH	(16*1024*1024)

#define NOR_CHIP_MULTIPLIER	(2)	/* Number of chips at REGA */

#if defined (CPLD_FLASH)
# define VPEN_ENABLE		(CPLD_FLASH |=  CPLD_FLASH_FL_VPEN)
# define VPEN_DISABLE		(CPLD_FLASH &= ~CPLD_FLASH_FL_VPEN)
#endif

#endif  /* __NOR_CFI_H__ */
