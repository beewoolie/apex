/* lpd79520.h
     $Id$

   written by Marc Singer
   14 Nov 2004

   Copyright (C) 2004 Marc Singer

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

#define CPLD_FLASH		0x55000000
#define CPLD_FLASH_VPEN		(1<<0)
#define CPLD_FLASH_VPEN		(1<<0)
#define CPLD_FLASH_FL_VPEN	CPLD_FLASH_VPEN

#define NOR_0_PHYS	(0x00000000)
#define NOR_0_LENGTH	(8*1024*1024)
#define WIDTH		(16)	/* Device width in bits */

#endif  /* __LPD79520_H__ */
