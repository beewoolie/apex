/* cmd-sleep.c

   written by Marc Singer
   30 May 2007

   Copyright (C) 2007 Marc Singer

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
#include <apex.h>
#include <command.h>
#include <service.h>
#include <error.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <asm/mmu.h>
#include <asm/cp15.h>

#include "hardware.h"


static int cmd_sleep (int argc, const char** argv)
{
  unsigned long ccmr;
  int mode;
  int well_bias = 0;

  if (argc < 2)
    return ERROR_PARAM;

  mode = simple_strtoul (argv[1], NULL, 10);

  switch (mode) {
  default:
  case 0:
    printf ("wait mode\n");
    mode = 0;
    break;
  case 1:
    printf ("doze mode\n");
    mode = 1;
    break;
  case 2:
    printf ("state retention mode\n");
    mode = 2;
    well_bias = 1;
    break;
  case 3:
    printf ("deep sleep mode\n");
    mode = 3;
    well_bias = 1;
    break;
  }

  ccmr = CCM_CCMR;

  MASK_AND_SET (ccmr, 3<<14, mode<<14); /* Set the mode we'll enter */
  MASK_AND_SET (ccmr, 1<<27, well_bias<<27); /* Set the mode we'll enter */
  ccmr |= 1<<28;		/* VSTBY */

  printf ("CCM_CCMR 0x%08lx\n", ccmr);
  CCM_CCMR = ccmr;

  CCM_WIMR0 = 0;
  CCM_CGR0 = 0xffffffff;
  CCM_CGR1 = 0xffffffff;
  CCM_CGR2 = 0xffffffff;

		/* Flush serial device */
  release_services ();

  WAIT_FOR_INTERRUPT;

#if 0
    // Wait few cycles
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("nop");

    __asm("mcr 15, 0, r0, c7, c7, 0\n\t");        /* invalidate I cache and D cache */
    __asm("mcr 15, 0, r0, c8, c7, 0\n\t");        /* invalidate TLBs */
    __asm("mcr 15, 0, r0, c7, c10, 4\n\t");       /* Drain the write buffer */

    // WFI
    __asm("MCR p15,0,r1,c7,c0,4\n\t");


    // Wait few cycles
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("nop");
    __asm("nop");
#endif

    return 0;
}


static __command struct command_d c_sleep = {
  .command = "sleep",
  .func = (command_func_t) cmd_sleep,
  COMMAND_DESCRIPTION ("sleep test ")
  COMMAND_HELP(
"sleep\n"
"  sleep tests.\n"
"  0		enter wait mode\n"
"  1		enter doze mode\n"
"  2		enter state retention mode\n"
"  3		enter deep sleep mode\n"
  )
};
