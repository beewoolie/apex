/* nand.h
     $Id$

   written by Marc Singer
   9 Nov 2004

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

#if !defined (__NAND_H__)
#    define   __NAND_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#define NAND_PHYS	(0x40800000)
#define NAND_DATA	(NAND_PHYS + 0x00)
#define NAND_CLE	(NAND_PHYS + 0x10)
#define NAND_ALE	(NAND_PHYS + 0x08)

#define Reset		(0xff)
#define ReadID		(0x90)
#define Status		(0x70)
#define Read1		(0x00)	/* Start address in first 256 bytes of page */
#define Read2		(0x01)	/* Start address in second 256 bytes of page */
#define Read3		(0x50)	/* Start address in last 16 bytes, (ECC?) */
#define Erase		(0x60)
#define EraseConfirm	(0xd0)
#define AutoProgram	(0x10)
#define SerialInput	(0x80)

#define Fail		(1<<0)
#define Ready		(1<<6)
#define Writable	(1<<7)


/* The NAND address map converts the boot code from the CPU into the
   number of address bytes used by the device.  Refer to the LH79524
   documentation for the boot codes.  The map is a 32 bit number where
   each boot code is represented by two bits.  Boot code zero occupies
   the least significant bits of the map.  The values in the map are 0
   for non-NAND boot, 1 for 3 address bytes, 2 for 4 address bytes,
   and 3 for five address bytes.  The simple decode macro will return
   2 for non-NAND boot codes.

   The reason for this is two-fold.  First, we want to be space
   efficient when we can.  This stragety yields a correct result in
   only a few instructions.  Second, a lookup using a byte array is
   infeasible when the loader is still trying to get itself into
   SDRAM.  The compiler wants to put the array into constant storage
   and not in the code segment.  Most importantly, though, this method
   is compact.

 */

#define _NAM(bootcode,address_bytes) \
	    ((((address_bytes) - 2)&0x3)<<(((bootcode) & 0xf)*2))
#define NAND_ADDR_MAP ( _NAM(4,3) | _NAM(5,4)  | _NAM(6,5)\
		      | _NAM(7,3) | _NAM(12,4) | _NAM(13,5))

#define NAM_DECODE(bootcode) ((NAND_ADDR_MAP >> ((bootcode)*2)) + 2)

#endif  /* __NAND_H__ */
