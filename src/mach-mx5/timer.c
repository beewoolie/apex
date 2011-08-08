/* timer.c

   written by Marc Singer
   20 Jun 2011

   Copyright (C) 2011 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   The mx51 GPT IP block is essentially the same as the mx31 version.
   Clock gating is different.

*/

#include <service.h>
#include "hardware.h"

static void mx5x_timer_init (void)
{
	/* Enable GPT clock  */
  MASK_AND_SET (                 CCM_CCGR2,
                CCM_CCGR_MASK << CCM_CCGR2_GPT_IPG_CLK_SH,
                CCM_CCGR_RUN  << CCM_CCGR2_GPT_IPG_CLK_SH);
  GPT_PR    = 32 - 1;                           /* 32000/32 -> 1.00 ms/cycle */
  GPT_CR    = GPT_CR_EN | GPT_CR_CLKSRC_32K | GPT_CR_FREERUN;
}

static void mx5x_timer_release (void)
{
  GPT_CR = 0;
}

unsigned long timer_read (void)
{
  return GPT_CNT;
}


/** compute the difference between two read timer values and return the
   difference in milliseconds. */

unsigned long timer_delta (unsigned long start, unsigned long end)
{
  return end - start;
}


/* usleep

   this function accepts a count of microseconds and will wait at
   least that long before returning.  It depends on the timer being
   setup.

   The source for the timer is really the IPG clock and not the high
   clock regardless of the setting for CLKSRC as either HIGH or IPG.
   In both cases, the CPU uses the IPG clock.  The value for IPGCLK
   comes from the main PLL and is available in a constant.  We divide
   down to get a 1us interval.

   ipg_clk() is 66.5 MHz, so we use instead a count of 10s of
   microseconds.  I believe that this can be fixed by using the
   ipg_hifreq clock but I need to figure out if this is, indeed, the
   ipg_perclk.

 */

void usleep (unsigned long us)
{
  unsigned long mode =
    EPT_CR_CLKSRC_IPG
    | EPT_CR_IOVW
    | ((ipg_clk ()/100000 - 1) << EPT_CR_PRESCALE_SH)
    | EPT_CR_DBGEN
    | EPT_CR_SWR
    ;
  us = (us + 5)/10;
  __asm volatile ("str %1, [%2, #0]\n\t"	/* EPITCR <- mode w/reset */
		  "orr %1, %1, #1\n\t"		/* |=  EPT_CR_EN */
		  "bic %1, %1, #0x10000\n\t"	/* &= ~EPT_CR_SWR  */
		  "str %1, [%2, #0]\n\t"	/* EPITCR <- mode w/EN */
		  "str %0, [%2, #8]\n\t"	/* EPITLR <- count */
	       "0: ldr %0, [%2, #4]\n\t"	/* EPITSR */
		  "tst %0, #1\n\t"
		  "beq 0b\n\t"
		  : "+r" (us)
		  :  "r" (mode),
		     "r" (PHYS_EPIT1)
		  : "cc");
}


static __service_2 struct service_d mx5x_timer_service = {
  .init    = mx5x_timer_init,
  .release = mx5x_timer_release,
};
