/* lh7a404.h
     $Id$

   written by Marc Singer
   8 Jul 2005

   Copyright (C) 2005 Marc Singer

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

#if !defined (__LH7A404_H__)
#    define   __LH7A404_H__

/* ----- Includes */

#include <asm/reg.h>

/* ----- Types */

#define ADC_PHYS	(0x80001300)
#define ADC_HW		__REG(ADC_PHYS + 0x00)	/* High Word (RO) */
#define ADC_LW		__REG(ADC_PHYS + 0x04)	/* Low Word (RO) */
#define ADC_RR		__REG(ADC_PHYS + 0x08)	/* Results (RO) */
#define ADC_IM		__REG(ADC_PHYS + 0x0c)	/* Interrupt Masking */
#define ADC_PC		__REG(ADC_PHYS + 0x10)	/* Power Configuration */
#define ADC_GC		__REG(ADC_PHYS + 0x14)	/* General Configuration */
#define ADC_GS		__REG(ADC_PHYS + 0x18)	/* General Status */
#define ADC_IS		__REG(ADC_PHYS + 0x1c)	/* Interrupt Status */
#define ADC_FS		__REG(ADC_PHYS + 0x20)	/* FIFO Status */
#define ADC_LWC_BASE	__REG(ADC_PHYS + 0x64)	/* Low Word Control (0-15) */
#define ADC_HWC_BASE_PHYS	(ADC_PHYS + 0x24)
#define ADC_HWC_BASE	__REG(ADC_PHYS + 0x24)	/* High Word Control (0-15) */
#define ADC_LWC_BASE_PHYS	(ADC_PHYS + 0x64)
#define ADC_IHWCTRL	__REG(ADC_PHYS + 0xa4)	/* Idle High Word Control */
#define ADC_ILWCTRL	__REG(ADC_PHYS + 0xa8)	/* Idle Low word Control */
#define ADC_MIS		__REG(ADC_PHYS + 0xac)	/* Masked Interrupt Status  */
#define ADC_IC		__REG(ADC_PHYS + 0xb0)	/* Interrupt clear */

/* ----- Globals */

/* ----- Prototypes */

#endif  /* __LH7A404_H__ */
