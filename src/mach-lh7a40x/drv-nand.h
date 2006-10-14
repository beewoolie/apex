/* drv-nand.h

   written by Marc Singer
   13 October 2006

   Copyright (C) 2006 Marc Singer

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

   NAND flash hooks.

*/

#if !defined (__DRV_NAND_H__)
#    define   __DRV_NAND_H__

/* ----- Includes */

#include "mach/hardware.h"

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#define NAND_PHYS	(0x60000000)
#define NAND_DATA	__REG8(NAND_PHYS + 0x00)
#define NAND_CLE	__REG8(NAND_PHYS + (1<<21))
#define NAND_ALE	__REG8(NAND_PHYS + (1<<20))

#define NAND_CS_ENABLE\
	({ GPIO_PCD &= ~(1<<6); })
#define NAND_CS_DISABLE\
	({ GPIO_PCD |= 1<<6; })

#define NAND_ISBUSY   ({ NAND_CLE = Status; (NAND_DATA & Ready) == 0; })

#endif  /* __DRV_NAND_H__ */
