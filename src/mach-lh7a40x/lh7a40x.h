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

	/* Physical registers of the LH7A404 */

#define VIC1_PHYS	(0x80008000)	/* Vectored Interrupt Controller 1 */
#define USBH_PHYS	(0x80009000)	/* USB OHCI host controller */
#define VIC2_PHYS	(0x8000a000)	/* Vectored Interrupt Controller 2 */


#define CSC_PWRSR	(0x00)	/* Power reset */
#define CSC_PWRCNT	(0x04)	/* Power control */
#define CSC_CLKSET	(0x20)	/* Clock speed control */ 

#define CSC_PWRSR_CHIPMAN_SHIFT	(24)
#define CSC_PWRSR_CHIPMAN_MASK	(0xff)
#define CSC_PWRSR_CHIPID_SHIFT	(16)
#define CSC_PWRSR_CHIPID_MASK	(0xff)

#define CSC_CLKSET_200_100_50	(0x0004ee39)
#define CSC_CLKSET_150_75_37	(0x00048eb1)
#define CSC_CLKSET_200_66_33	(0x0004ee3a)


#define CSC_PWRCNT_USBH_EN	(1<<28)	/* USB Host power enable */

#define SMC_BCR0		(0x00)
#define SMC_BCR1		(0x04)
#define SMC_BCR2		(0x08)
#define SMC_BCR3		(0x0c)
#define SMC_BCR6		(0x18)
#define SMC_BCR7		(0x1c)

#define SDRC_GBLCNFG		(0x04)
#define SDRC_RFSHTMR		(0x08)
#define SDRC_BOOTSTAT		(0x0c)
#define SDRC_SDCSC0		(0x10)
#define SDRC_SDCSC1		(0x14)
#define SDRC_SDCSC2		(0x18)
#define SDRC_SDCSC3		(0x1c)

#define SDRAM_BANK0_PHYS	0xc0000000
#define SDRAM_BANK1_PHYS	0xd0000000
#define SDRAM_BANK2_PHYS	0xe0000000

#define TIMER1_PHYS		(TIMER_PHYS | 0x00)
#define TIMER2_PHYS		(TIMER_PHYS | 0x20)
#define TIMER3_PHYS		(TIMER_PHYS | 0x80)

#define TIMER_LOAD		(0x00)
#define TIMER_VALUE		(0x04)
#define TIMER_CONTROL		(0x08)

#define UART_DATA		(0x00)
#define UART_FCON		(0x04)
#define UART_BRCON		(0x08)
#define UART_CON		(0x0c)
#define UART_STATUS		(0x10)
#define UART_RAWISR		(0x14)
#define UART_INTEN		(0x18)
#define UART_ISR		(0x1c)

#define UART_DATA_FE		(1<<8)
#define UART_DATA_PE		(1<<9)
#define UART_DATA_DATAMASK	(0xff)

#define UART_STATUS_TXFE	(1<<7)
#define UART_STATUS_RXFF	(1<<6)
#define UART_STATUS_TXFF	(1<<5)
#define UART_STATUS_RXFE	(1<<4)
#define UART_STATUS_BUSY	(1<<3)
#define UART_STATUS_DCD		(1<<2)
#define UART_STATUS_DSR		(1<<1)
#define UART_STATUS_CTS		(1<<0)

#define UART_FCON_WLEN8		(3<<5)
#define UART_FCON_FEN		(1<<4)

#define UART_CON_SIRDIS		(1<<1)
#define UART_CON_ENABLE		(1<<0)

#define UART_PHYS		UART2_PHYS

#endif  /* __LH7A40X_H__ */
