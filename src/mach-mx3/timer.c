/* timer.c

   written by Marc Singer
   16 Dec 2006

   Copyright (C) 2006 Marc Singer

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

#include <service.h>
#include "hardware.h"

static void mx3x_timer_init (void)
{
  CCM_CGR0 |= CCM_CGR_RUN << CCM_CGR0_GPT_SH;	/* Enable GPT clock  */
  GPT_PR = 32 - 1;		/* 32000/32 -> 1.00 ms/cycle */
  GPT_CR = GPT_CR_EN | GPT_CR_CLKSRC_32K | GPT_CR_FREERUN;
}

static void mx3x_timer_release (void)
{
  GPT_CR = 0;
}

unsigned long timer_read (void)
{
  return GPT_CNT;
}


/* timer_delta

   returns the difference in time in milliseconds.

 */

unsigned long timer_delta (unsigned long start, unsigned long end)
{
  return end - start;
}

static __service_2 struct service_d mx3x_timer_service = {
  .init    = mx3x_timer_init,
  .release = mx3x_timer_release,
};
