/* drv-ata.h

   written by Marc Singer
   13 Sep 2011

   Copyright (C) 2011 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__MACH_DRV_ATA_H__)
#    define   __MACH_DRV_ATA_H__

/* ----- Includes */

#include "hardware.h"

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

# if !defined (ATA_PHYS)
#  define ATA_PHYS		PHYS_PATA_PIO
# endif

# define ATA_WIDTH		(16)
# define ATA_ADDR_MULT		(4)
/* *** FIXME: need to distinguish between paired and unpaired registers */
# define ATA_REG_BASE           (0xa0) /* Start of register file */
//# define ATA_REG			(1<<12)	/* REG line for register access */
# define ATA_ALT_BASE           (0xa0)
//# define ATA_ALT			(1<<10)	/* A10 line for data read */
//# define ATA_ATTRIB		(1<<9) /* A9 line for attribute access */
# define ATA_ATTR_BASE          (0xa0)

void mx5_ata_init (void);
#define ATA_PLATFORM_INIT       ({ mx5_ata_init (); })

#endif  /* __MACH_DRV_ATA_H__ */
