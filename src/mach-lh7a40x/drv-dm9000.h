/* drv-dm9000.h

   written by Marc Singer
   2 June 2006

   Copyright (C) 2006 Marc Singer

   This program is licensed under the terms of the GNU General Public
   License as published by the Free Software Foundation.  Please refer
   to the file debian/copyright for further details.

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

#if defined (CONFIG_MACH_COMPANION)

# define DM_WIDTH	16

# define C_DM		(2)

# define DM_PHYS	(0x20000000)		/* Dev MAC/PHY */
# define DM_PHYS_INDEX	(DM_PHYS + 0)
# define DM_PHYS_DATA	(DM_PHYS + 4)
# define DM_NAME	"dev"

# define DM1_PHYS	(0x10000000)		/* Modem MAC/PHY */
# define DM1_PHYS_INDEX	(DM1_PHYS + 0)
# define DM1_PHYS_DATA	(DM1_PHYS + 4)
# define DM1_NAME	"modem"

#endif

#if defined (CONFIG_MACH_KARMA)

# define DM_WIDTH	16

# define DM_PHYS	(0x10000000)
# define DM_PHYS_INDEX	(DM_PHYS + 0)
# define DM_PHYS_DATA	(DM_PHYS + 4)
# define DM_NAME	"dev"

#endif

#endif  /* __MACH_DRV_DM9000_H__ */
