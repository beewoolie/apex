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

   Using a timer
   -------------

   The timers can be clocked, at the slowest, at HCLK/128 which is
   ~2.52us given the standard HCLK of 50803200 Hz.  The counters count
   up and can be programmed to automatically clear when they reach a
   set value.  The longest this can be is ~165ms which should be
   adequate.

   We can compute the timer limit by multiplying the HCLK base
   frequency by the desired interrupt interval, in this case 10ms
   (.01), and dividing that by the clock scalar of 128.

*/

#include <config.h>
#include <service.h>
#include "hardware.h"
#include <asm/interrupts.h>

#if defined (CONFIG_INTERRUPTS)
# include <linux/compiler-gcc.h>
#endif

#define TIMER_CTRL	TIMER2_CTRL
#define TIMER_INTEN	TIMER2_INTEN
#define TIMER_STATUS	TIMER2_STATUS
#define TIMER_CNT	TIMER2_CNT
#define TIMER_CMP0	TIMER2_CMP0
#define TIMER_CMP1	TIMER2_CMP1
#define TIMER_CAPA	TIMER2_CAPA
#define TIMER_CAPB	TIMER2_CAPB
#define TIMER_IRQ	IRQ_TIMER2

#define UART_DR		__REG(UART + 0x00)

#define TIMER_LIMIT	(HCLK/(128*100))

#if defined (CONFIG_INTERRUPTS)
static irq_return_t timer_isr (int irq)
{
  static int v;

//  UART_DR = '!';
  TIMER_STATUS = 0xf;		/* Clear interrupt */

  if (v++ %100 == 0) {
    UART_DR = '.';
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
   interrupt_handlers[TIMER_IRQ] = timer_isr;
   barrier ();

   TIMER_CMP1 = TIMER_LIMIT;
   TIMER_CTRL
     = TIMER_CTRL_SCALE_128
     | TIMER_CTRL_CS
     | TIMER_CTRL_TC
     | TIMER_CTRL_CCL;
   TIMER_INTEN = TIMER_INTEN_CMP1;

   VIC_INTSELECT = 0;
   VIC_INTENABLE |= (1<<TIMER_IRQ);
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
