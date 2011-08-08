/** @file clocks.h

   written by Marc Singer
   9 Aug 2011

   Copyright (C) 2011 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (CLOCKS_H_INCLUDED)
#    define   CLOCKS_H_INCLUDED

/* ----- Includes */

#include "linux/types.h"

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

u32 ahb_clk (void);
u32 cspi_clk (void);
u32 esdhcx_clk (int index);
u32 ipg_clk (void);
u32 ipg_per_clk (void);
u32 periph_clk (void);


#endif  /* CLOCKS_H_INCLUDED */
