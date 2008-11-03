/* nor-cfi.h

   written by Marc Singer
   2 Nov 2008

   Copyright (C) 2008 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__NOR_CFI_H__)
#    define   __NOR_CFI_H__

/* ----- Includes */

#include <config.h>
#include <mach/hardware.h>

/* ----- Constants */

#if !defined (NOR_WIDTH)
# define NOR_WIDTH		(16)
#endif
#if !defined (NOR_0_PHYS)
# define NOR_0_PHYS		(0xfff80000)
#endif

#if !defined (NOR_CHIP_MULTIPLIER)
# define NOR_CHIP_MULTIPLIER	(1)	/* Number of chips at REGA */
#endif



#endif  /* __NOR_CFI_H__ */
