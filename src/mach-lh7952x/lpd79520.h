/* lpd79520.h
     $Id$

   written by Marc Singer
   14 Nov 2004

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

#if !defined (__LPD79520_H__)
#    define   __LPD79520_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#define CPLD_FLASH		__REG16 (0x55000000)
#define CPLD_REVISION		__REG16 (0x55400000)
#define CPLD_FLASH_VPEN		(1<<0)
#define CPLD_FLASH_FL_VPEN	CPLD_FLASH_VPEN

//#define NOR_0_PHYS	(0x00000000)
//#define NOR_0_LENGTH	(8*1024*1024)
//#define WIDTH		(16)	/* Device width in bits */

#endif  /* __LPD79520_H__ */
