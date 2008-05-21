/* arch.c

   written by Marc Singer
   5 Dec 2004

   Copyright (C) 2004 Marc Singer

   This program is licensed under the terms of the GNU General Public
   License as published by the Free Software Foundation.  Please refer
   to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   Architecture support

*/

#include <config.h>
#include "hardware.h"

/* determine_arch_number

   queries the CPU to determine which architecture ID to give to the
   kernel.  As the LPD7A40X boards are similar enough for the
   bootloader, there can be one bootloader binary for both.

*/

int determine_arch_number (void)
{
  switch (((CSC_PWRSR >> CSC_PWRSR_CHIPID_SHIFT)
	   & CSC_PWRSR_CHIPID_MASK)
	  & 0xf0) {
  default:
  case 0x00:
    return 389;			/* LPD7A400 */
  case 0x20:
    return 390;			/* LPD7A404 */
  }
}
