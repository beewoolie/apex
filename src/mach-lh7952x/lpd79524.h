/* lpd79524.h
     $Id$

   written by Marc Singer
   12 Nov 2004

   Copyright (C) 2004 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__LPD79524_H__)
#    define   __LPD79524_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */


#define CPLD_REVISION		(0x4ca00000)

#define CPLD_FLASH		(0x4c800000)
#define CPLD_FLASH_RDYnBSY	(1<<2)
#define CPLD_FLASH_FL_VPEN	(1<<0)
#define CPLD_FLASH_STS1		(1<<1)
#define CPLD_FLASH_NANDSPD	(1<<6)


#endif  /* __LPD79524_H__ */
