/* timer-lh79524.c
     $Id$

   written by Marc Singer
   1 Nov 2004

   Copyright (C) 2004 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <service.h>
#include "hardware.h"
#include <asm/interrupts.h>

#if defined (CONFIG_INTERRUPTS)
# include <linux/compiler-gcc.h>
#endif

#if defined (CONFIG_INTERRUPTS)
static irq_return_t timer_isr (int irq)
{
  static int v;

  __REG (TIMER1_PHYS + TIMER_STATUS1) = 0xf; /* Clear interrupt */

  if (v++ %8 == 0) {
    __REG (UART + UART_DR) = '.';
  }
  return IRQ_HANDLED;
}
#endif

void lh79524_timer_init (void)
{
  RCPC_CTRL      |= RCPC_CTRL_UNLOCK;
  RCPC_PCLKSEL0  |= 3<<7; /* 32KHz oscillator for RTC */
  RCPC_CTRL      &= ~RCPC_CTRL_UNLOCK;
  RTC_CR	  = RTC_CR_EN;  

#if defined (CONFIG_INTERRUPTS)
 {
   interrupt_handlers[5] = timer_isr;
   barrier ();

   __REG (TIMER1_PHYS + TIMER_CTRL) = (7<<2)|(1<<1)|(1<<0);
   __REG (TIMER1_PHYS + TIMER_INTEN1) = (1<<0);

   VIC_INTENABLE |= (1<<5);
 }
#endif
}

static void lh79524_timer_release (void)
{
  RTC_CR &= ~RTC_CR_EN;  
  RCPC_CTRL      |= RCPC_CTRL_UNLOCK;
  RCPC_PCLKSEL0  &= ~(3<<7); /* 1Hz RTC */
  RCPC_CTRL      &= ~RCPC_CTRL_UNLOCK;

}

unsigned long timer_read (void)
{
  return RTC_DR;
}


/* timer_delta

   returns the difference in time in milliseconds.
  
 */

unsigned long timer_delta (unsigned long start, unsigned long end)
{
  return (end - start)*1000/32768;
}

static __service_2 struct service_d lh79524_timer_service = { 
  .init    = lh79524_timer_init,
  .release = lh79524_timer_release,
};
