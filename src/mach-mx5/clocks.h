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

uint32_t ahb_clk (void);
uint32_t cspi_clk (void);
uint32_t esdhcx_clk (int index);
uint32_t ipg_clk (void);
uint32_t ipg_per_clk (void);
uint32_t periph_clk (void);
uint32_t uart_clk (void);


#endif  /* CLOCKS_H_INCLUDED */
