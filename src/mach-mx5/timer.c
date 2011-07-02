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

static __service_2 struct service_d mx5x_timer_service = {
  .init    = mx5x_timer_init,
  .release = mx5x_timer_release,
};
