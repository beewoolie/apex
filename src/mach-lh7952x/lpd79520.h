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
#define CPLD_CONTROL		__REG16 (0x54200000)

#define CPLD_I2S_PHYS		(0x54400000)

#define CPLD_SPID		__REG16 (0x54600000)
#define CPLD_SPIC		__REG16 (0x54800000)
#define CPLD_SPI		__REG16 (0x54a00000)

#define CPLD_SPIC_LOADED	(1<<5)
#define CPLD_SPIC_LOAD		(1<<4)
#define CPLD_SPIC_DONE		(1<<3)
#define CPLD_SPIC_READ		(1<<2)
#define CPLD_SPIC_CS_TOUCH	(1<<1)
#define CPLD_SPIC_CS_CODEC	(1<<0)

#define CPLD_SPI_CS_EEPROM	(1<<3)
#define CPLD_SPI_SCLK		(1<<2)
#define CPLD_SPI_TX_SHIFT	(1)
#define CPLD_SPI_TX		(1<<CPLD_SPI_TX_SHIFT)
#define CPLD_SPI_RX_SHIFT	(0)
#define CPLD_SPI_RX		(1<<CPLD_SPI_RX_SHIFT)

#define CPLD_FLASH_VPEN		(1<<0)
#define CPLD_FLASH_FL_VPEN	CPLD_FLASH_VPEN

#define CPLD_CONTROL_WLPEN	(1<<0)

#define CPLD_GPIO		__REG16 (0x55600000)

#endif  /* __LPD79520_H__ */
