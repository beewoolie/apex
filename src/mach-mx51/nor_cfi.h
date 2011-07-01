/** @file nor_cfi.h

   written by Marc Singer
   30 Jun 2011

   Copyright (C) 2011 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (NOR_CFI_H_INCLUDED)
#    define   NOR_CFI_H_INCLUDED

/* ----- Includes */

#include <config.h>
#include <mach/hardware.h>

/* ----- Constants */

#if !defined (NOR_WIDTH)
# define NOR_WIDTB	(16)
#endif
#if !defined (NOR_0_PHYS)
# define NOR_0_PHYS		(PHYS_CS0)
#endif

#if !defined (NOR_CHIP_MULTIPLIER)
# define NOR_CHIP_MULTIPLIER	(1)	/* Number of chips at REGA */
#endif

#endif  /* NOR_CFI_H_INCLUDED */
