/* timer.c
     $Id$

   written by Marc Singer
   1 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <service.h>
#include "lh79524.h"

static void lh79524_timer_init (void)
{
  __REG (RCPC_PHYS + RCPC_PCLKSEL0) |= 3<<7; /* 32KHz oscillator for RTC */
  __REG (RTC_PHYS + RTC_CR) = RTC_CR_EN;  
}

static void lh79524_timer_release (void)
{
  __REG (RTC_PHYS + RTC_CR) &= ~RTC_CR_EN;  
  __REG (RCPC_PHYS + RCPC_PCLKSEL0) &= ~(3<<7); /* 1Hz RTC */
}

unsigned long timer_read (void)
{
  return __REG (RTC_PHYS + RTC_DR);
}

/* timer_delta

   returns the difference in time in milliseconds.
  
 */

unsigned long timer_delta (unsigned long start, unsigned long end)
{
  return (end - start)*1000/32768;
}

static __service_0 struct service_d lh79524_timer_service = { 
  .init = lh79524_timer_init,
  .release = lh79524_timer_release,
};
