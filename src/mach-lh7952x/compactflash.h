/* compactflash.h
     $Id$

   written by Marc Singer
   7 Feb 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__COMPACTFLASH_H__)
#    define   __COMPACTFLASH_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#if !defined (CF_PHYS)
# define CF_PHYS	(0x48200000)
#endif

/* LPD79524 specifics */
#define CF_WIDTH	16
#define CF_ADDR_MULT	2
#define CF_REG		(1<<13)
#define CF_ALT		(1<<12)
#define CF_ATTRIB	(1<<10)

#endif  /* __COMPACTFLASH_H__ */
