/* arch.c
     $Id$

   written by Marc Singer
   5 Dec 2004

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
  switch (((__REG (CSC_PHYS + CSC_PWRSR) >> CSC_PWRSR_CHIPID_SHIFT)
	   & CSC_PWRSR_CHIPID_MASK)
	  & 0xf0) {
  default:
  case 0x00:
    return 389;			/* LPD7A400 */
  case 0x20:
    return 390;			/* LPD7A404 */
  }
}

