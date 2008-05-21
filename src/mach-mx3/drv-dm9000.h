/* drv-dm9000.h

   written by Marc Singer
   6 April 2006

   Copyright (C) 2006 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

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

/* There are two versions of the NFF board, one has a shifted address
   bus on the OneNAND and Ethernet controller and one does not.  The
   correct data offset is +4.  The older, broken, and shifted version
   uses +8. */

#define DM_WIDTH	16

#define DM_PHYS		(0xb4000000) /* Dev MAC/PHY */
#define DM_PHYS_INDEX	(DM_PHYS + 0)
#define DM_PHYS_DATA	(DM_PHYS + 4)
//#define DM_NAME		"dev"

#endif  /* __MACH_DRV_DM9000_H__ */
