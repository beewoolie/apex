/* drv-dm9000.h

   written by Marc Singer
   2 June 2006

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

*/

#if !defined (__MACH_DRV_DM9000_H__)
#    define   __MACH_DRV_DM9000_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#define DM_WIDTH	16

#define C_DM		(2)

#define DM_PHYS		(0x20000000) /* Dev MAC/PHY */
#define DM_PHYS_INDEX	(DM_PHYS + 0)
#define DM_PHYS_DATA	(DM_PHYS + 4)

#define DM1_PHYS	(0x10000000) /* Modem MAC/PHY */
#define DM1_PHYS_INDEX	(DM1_PHYS + 0)
#define DM1_PHYS_DATA	(DM1_PHYS + 4)

#endif  /* __MACH_DRV_DM9000_H__ */
