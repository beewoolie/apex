/* nand.h
     $Id$

   written by Marc Singer
   9 Nov 2004

   Copyright (C) 2004 The Buici Company

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
#define NAND_DATA	(NAND_PHYS | 0x00)
#define NAND_CLE	(NAND_PHYS | 0x10)
#define NAND_ALE	(NAND_PHYS | 0x08)

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

#define CPLD_REG_FLASH	(0x4c800000)
#define RDYnBSY		(1<<2)



#endif  /* __NAND_H__ */
