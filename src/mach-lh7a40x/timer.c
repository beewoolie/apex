/* timer.c
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

   There isn't a good timing source available on the LH7A40X CPUs.  We
   satisfy ourselves with a 16 bit, 2KHz timer that does some
   primitive checking for the timer wrapping.  This works as long as
   the interval between reads is less than half of the period of the
   timer, or 15 seconds.  This turns out to be an OK assumption since
   the timer tends to be used for checking very short timeout values.

*/

#include <service.h>
#include "lh7a40x.h"

static void lh7a40x_timer_init (void)
{
	/* 2kHz free running timer*/
  __REG (TIMER1_PHYS + TIMER_CONTROL) = (1<<7) | (0<<6) | (0<<3);
  __REG (TIMER1_PHYS + TIMER_VALUE) = 0xffff;
  __REG (TIMER2_PHYS + TIMER_CONTROL) = 0;
  __REG (TIMER3_PHYS + TIMER_CONTROL) = 0;
}

static void lh7a40x_timer_release (void)
{
  __REG (TIMER1_PHYS + TIMER_CONTROL) = 0;
}

unsigned long timer_read (void)
{
  static unsigned long valueLast;
  static unsigned long wrap;

  unsigned long value = __REG (TIMER1_PHYS + TIMER_VALUE);

  if (value > valueLast)
    ++wrap;
  valueLast = value;

  return (wrap*0x10000) + 0x10000 - value;
}

/* timer_delta

   returns the difference in time in milliseconds.
  
 */

unsigned long timer_delta (unsigned long start, unsigned long end)
{
  return (end - start)/2;
}

static __service_2 struct service_d lh7a40x_timer_service = { 
  .init    = lh7a40x_timer_init,
  .release = lh7a40x_timer_release,
};
