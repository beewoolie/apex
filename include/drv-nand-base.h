/* drv-nand-base.h

   written by Marc Singer
   15 Feb 2007

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

#if !defined (__DRV_NAND_BASE_H__)
#    define   __DRV_NAND_BASE_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#if defined (CONFIG_DRIVER_NAND_TYPE_ST)

#define NAND_Reset			(0xff)
#define NAND_ReadID			(0x90)
#define NAND_Status			(0x70)
#define NAND_ReadSetup			(0x00)
#define NAND_Read			(0x30)
#define NAND_Erase			(0x60)
#define NAND_EraseConfirm		(0xd0)
#define NAND_PageProgram		(0x80)
#define NAND_PageProgramConfirm		(0x10)

//#define NAND_AutoProgram (0x10)
//#define NAND_SerialInput (0x80)
//#define NAND_CopyBack	 (0x8a)

#define NAND_Fail			(1<<0)
#define NAND_CacheErr			(1<<1)
#define NAND_CacheReady			(1<<5)
#define NAND_Ready			(1<<6)
#define NAND_Writable			(1<<7)

#endif

#if defined (CONFIG_DRIVER_NAND_TYPE_TOSHIBA)

#define NAND_Reset	(0xff)
#define NAND_ReadID	(0x90)
#define NAND_Status	(0x70)
#define NAND_Read1	(0x00)	/* Start address in first 256 bytes of page */
#define NAND_Read2	(0x01)	/* Start address in second 256 bytes of page */
#define NAND_Read3	(0x50)	/* Start address in last 16 bytes of page */
#define NAND_Erase	(0x60)
#define NAND_EraseConfirm (0xd0)
#define NAND_AutoProgram (0x10)
#define NAND_SerialInput (0x80)
#define NAND_CopyBack	 (0x8a)

#define NAND_Fail	(1<<0)
#define NAND_Ready	(1<<6)
#define NAND_Writable	(1<<7)

#endif


#endif  /* __DRV_NAND_BASE_H__ */
