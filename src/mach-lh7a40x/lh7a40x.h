/* lh7a40x.h
     $Id$

   written by Marc Singer
   10 Nov 2004

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

#if !defined (__LH7A40X_H__)
#    define   __LH7A40X_H__

/* ----- Includes */

#include <asm/reg.h>

/* ----- Prototypes */

/* ----- Macros */

	/* Physical register base addresses */

#define AC97_PHYS	(0x80000000)	/* AC97 Controller */
#define MMC_PHYS	(0x80000100)	/* Multimedia Card Controller */
#define USB_PHYS	(0x80000200)	/* USB Client */
#define SCI_PHYS	(0x80000300)	/* Secure Card Interface */
#define CSC_PHYS	(0x80000400)	/* Clock/State Controller  */
#define INTC_PHYS	(0x80000500)	/* Interrupt Controller */
#define UART1_PHYS	(0x80000600)	/* UART1 Controller */
#define SIR_PHYS	(0x80000600)	/* IR Controller, same are UART1 */
#define UART2_PHYS	(0x80000700)	/* UART2 Controller */
#define UART3_PHYS	(0x80000800)	/* UART3 Controller */
#define DCDC_PHYS	(0x80000900)	/* DC to DC Controller */
#define ACI_PHYS	(0x80000a00)	/* Audio Codec Interface */
#define SSP_PHYS	(0x80000b00)	/* Synchronous ... */
#define TIMER_PHYS	(0x80000c00)	/* Timer Controller */
#define RTC_PHYS	(0x80000d00)	/* Real-time Clock */
#define GPIO_PHYS	(0x80000e00)	/* General Purpose IO */
#define BMI_PHYS	(0x80000f00)	/* Battery Monitor Interface */
#define HRTFTC_PHYS	(0x80001000)	/* High-res TFT Controller */
#define WDT_PHYS	(0x80001400)	/* Watchdog Timer */
#define SMC_PHYS	(0x80002000)	/* Static Memory Controller */
#define SDRC_PHYS	(0x80002400)	/* SDRAM Controller */
#define DMAC_PHYS	(0x80002800)	/* DMA Controller */
#define CLCDC_PHYS	(0x80003000)	/* Color LCD Controller */

#define UART		(UART2_PHYS)

	/* Physical registers of the LH7A404 */

#define VIC1_PHYS	(0x80008000)	/* Vectored Interrupt Controller 1 */
#define USBH_PHYS	(0x80009000)	/* USB OHCI host controller */
#define VIC2_PHYS	(0x8000a000)	/* Vectored Interrupt Controller 2 */


#define CSC_PWRSR	__REG(CSC_PHYS + 0x00)	/* Power reset */
#define CSC_PWRCNT	__REG(CSC_PHYS + 0x04)	/* Power control */
#define CSC_CLKSET	__REG(CSC_PHYS + 0x20)	/* Clock speed control */ 

#define CSC_PWRSR_CHIPMAN_SHIFT	(24)
#define CSC_PWRSR_CHIPMAN_MASK	(0xff)
#define CSC_PWRSR_CHIPID_SHIFT	(16)
#define CSC_PWRSR_CHIPID_MASK	(0xff)

#define CSC_CLKSET_200_100_50	(0x0004ee39)
#define CSC_CLKSET_150_75_37	(0x00048eb1)
#define CSC_CLKSET_200_66_33	(0x0004ee3a)


#define CSC_PWRCNT_USBH_EN	(1<<28)	/* USB Host power enable */

#define SMC_BCR0		__REG (SMC_PHYS + 0x00)
#define SMC_BCR1		__REG (SMC_PHYS + 0x04)
#define SMC_BCR2		__REG (SMC_PHYS + 0x08)
#define SMC_BCR3		__REG (SMC_PHYS + 0x0c)
#define SMC_BCR6		__REG (SMC_PHYS + 0x18)
#define SMC_BCR7		__REG (SMC_PHYS + 0x1c)
#define SMC_PCMCIACON		__REG (SMC_PHYS + 0x40)

#define SDRC_GBLCNFG		__REG (SDRC_PHYS + 0x04)
#define SDRC_RFSHTMR		__REG (SDRC_PHYS + 0x08)
#define SDRC_BOOTSTAT		__REG (SDRC_PHYS + 0x0c)
#define SDRC_SDCSC0		__REG (SDRC_PHYS + 0x10)
#define SDRC_SDCSC1		__REG (SDRC_PHYS + 0x14)
#define SDRC_SDCSC2		__REG (SDRC_PHYS + 0x18)
#define SDRC_SDCSC3		__REG (SDRC_PHYS + 0x1c)

#define SDRAM_BANK0_PHYS	0xc0000000
#define SDRAM_BANK1_PHYS	0xd0000000
#define SDRAM_BANK2_PHYS	0xe0000000

#define TIMER1_PHYS		(TIMER_PHYS + 0x00)
#define TIMER2_PHYS		(TIMER_PHYS + 0x20)
#define TIMER3_PHYS		(TIMER_PHYS + 0x80)

#define TIMER_LOAD		(0x00)
#define TIMER_VALUE		(0x04)
#define TIMER_CONTROL		(0x08)

#define CLCDC_TIMING0		(0x00)
#define CLCDC_TIMING1		(0x04)
#define CLCDC_TIMING2		(0x08)
#define CLCDC_TIMING3		(0x0c)
#define CLCDC_UPBASE		(0x10)
#define CLCDC_LPBASE		(0x14)
#define CLCDC_INTREN		(0x18)
#define CLCDC_CTRL		(0x1c)
#define CLCDC_STATUS		(0x20)
#define CLCDC_INTERRUPT		(0x24)
#define CLCDC_UPCURR		(0x28)
#define CLCDC_LPCURR		(0x2c)
#define CLCDC_PALETTE		(0x200)

#define HRTFTC_SETUP		(0x00)
#define HRTFTC_CON		(0x04)
#define HRTFTC_TIMING1		(0x08)
#define HRTFTC_TIMING2		(0x0c)

#define GPIO_PINMUX		(0x2c)
#define GPIO_PADD		(0x10)
#define GPIO_PAD		(0x00)


#endif  /* __LH7A40X_H__ */
