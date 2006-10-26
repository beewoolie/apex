/* nand.h

   written by Marc Singer
   13 Oct 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__NAND_H__)
#    define   __NAND_H__

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

#endif  /* __NAND_H__ */
