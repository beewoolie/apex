/* lh79524.h
     $Id$

   written by Marc Singer
   31 Oct 2004

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

#if !defined (__LH79524_H__)
#    define   __LH79524_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

typedef struct { volatile unsigned long  offset[4096]; } __regbase;
# define __REG(x)	((__regbase   *)((x)&~4095))->offset[((x)&4095)>>2]
typedef struct { volatile unsigned short offset[4096]; } __regbase16;
# define __REG16(x)	((__regbase16 *)((x)&~4095))->offset[((x)&4095)>>1]
typedef struct { volatile unsigned char  offset[4096]; } __regbase8;
# define __REG8(x)	((__regbase8  *)((x)&~4095))->offset[(x)&4095]

	/* Registers */

#define RCPC_PHYS	(0xfffe2000)

#define RCPC_CTRL	(0x00)
#define RCPC_CHIPID	(0x04)
#define RCPC_SYSCLKPRE	(0x18)
#define RCPC_CPUCLKPRE	(0x1c)
#define RCPC_CPUCLKPRE	(0x1c)
#define RCPC_PCLKCTRL0	(0x24)
#define RCPC_PCLKCTRL1	(0x28)
#define RCPC_AHBCLKCTRL	(0x2c)
#define RCPC_PCLKSEL0	(0x30)
#define RCPC_PCLKSEL1	(0x34)
#define RCPC_INTCONFIG	(0x80)
#define RCPC_INTCLR	(0x84)
#define RCPC_CORECONFIG	(0x88)
#define RCPC_SYSPLLCNTL	(0xc0)

#define EMC_PHYS	(0xffff1000)

#define EMC_CONTROL	(0x00)
#define EMC_STATUS	(0x04)
#define EMC_CONFIG	(0x08)

#define EMC_DYNMCTRL	(0x20)
#define EMC_DYNMREF	(0x24)
#define EMC_READCONFIG	(0x28)	/* Undocumented */
#define EMC_PRECHARGE	(0x30)	/* rp */
#define EMC_DYNM2PRE	(0x34)	/* ras */
#define EMC_REFEXIT	(0x38)	/* rex */
#define EMC_DOACTIVE	(0x3c)	/* apr */
#define EMC_DIACTIVE	(0x40)	/* dal */
#define EMC_DWRT	(0x44)	/* wr */

#define EMC_DYNACTCMD	(0x48)	/* rc */
#define EMC_DYNAUTO	(0x4c)	/* rfc */
#define EMC_DYNREFEXIT	(0x50)	/* xsr */
#define EMC_DYNACTIVEAB	(0x54)	/* rrd */
#define EMC_DYNAMICTMRD	(0x58)	/* mrd */

#define EMC_WAIT	(0x80)

#define EMC_DYNCFG0	(0x100)
#define EMC_DYNRASCAS0	(0x104)
#define EMC_DYNCFG1	(0x108)
#define EMC_DYNRASCAS1	(0x10c)

#define EMC_SCONFIG0	(0x200)
#define EMC_SWAITWEN0	(0x204)
#define EMC_SWAITOEN0	(0x208)
#define EMC_SWAITRD0	(0x20c)
#define EMC_SWAITPAGE0	(0x210)
#define EMC_SWAITWR0	(0x214)
#define EMC_STURN0	(0x218)

#define EMC_SCONFIG1	(0x220)
#define EMC_SWAITWEN1	(0x224)
#define EMC_SWAITOEN1	(0x228)
#define EMC_SWAITRD1	(0x22c)
#define EMC_SWAITPAGE1	(0x230)
#define EMC_SWAITWR1	(0x234)
#define EMC_STURN1	(0x238)

#define EMC_SCONFIG2	(0x240)
#define EMC_SWAITWEN2	(0x244)
#define EMC_SWAITOEN2	(0x248)
#define EMC_SWAITRD2	(0x24c)
#define EMC_SWAITPAGE2	(0x250)
#define EMC_SWAITWR2	(0x254)
#define EMC_STURN2	(0x258)

#define EMC_SCONFIG3	(0x260)
#define EMC_SWAITWEN3	(0x264)
#define EMC_SWAITOEN3	(0x268)
#define EMC_SWAITRD3	(0x26c)
#define EMC_SWAITPAGE3	(0x270)
#define EMC_SWAITWR3	(0x274)
#define EMC_STURN3	(0x278)

#define IOCON_PHYS	(0xfffe5000)

#define IOCON_MUXCTL5	(0x20)
#define IOCON_MUXCTL6	(0x28)
#define IOCON_MUXCTL7	(0x30)
#define IOCON_RESCTL7	(0x34)
#define IOCON_MUXCTL10	(0x48)
#define IOCON_MUXCTL11	(0x50)
#define IOCON_MUXCTL12	(0x58)
#define IOCON_MUXCTL14	(0x68)
#define IOCON_MUXCTL19	(0x90)
#define IOCON_MUXCTL20	(0x98)

#define UART_PHYS	(0xfffc0000)

#define UART0_PHYS	(UART_PHYS + 0x0000)
#define UART1_PHYS	(UART_PHYS + 0x1000)
#define UART2_PHYS	(UART_PHYS + 0x2000)
#define UART		(UART0_PHYS)
#define UART_DR		(0x00)
#define UART_IBRD	(0x24)
#define UART_FBRD	(0x28)
#define UART_LCR_H	(0x2c)
#define UART_CR		(0x30)
#define UART_FR		(0x18)
#define UART_IMSC	(0x38)
#define UART_ICR	(0x44)

#define TIMER_PHYS	(0xfffc4000)

#define TIMER0_PHYS	(TIMER_PHYS + 0x00)
#define TIMER1_PHYS	(TIMER_PHYS + 0x30)
#define TIMER2_PHYS	(TIMER_PHYS + 0x50)

#define TIMER_CTRL	(0x00)
#define TIMER1_CNT	(0xc)

#define RTC_PHYS	(0xfffe0000)
#define RTC_DR		(0x00)
#define RTC_LR		(0x08)
#define RTC_CR		(0x0c)

#define GPIO_AB_PHYS	(0xfffdf000)
#define GPIO_CD_PHYS	(0xfffde000)
#define GPIO_EF_PHYS	(0xfffdd000)
#define GPIO_GH_PHYS	(0xfffdc000)
#define GPIO_IJ_PHYS	(0xfffdb000)
#define GPIO_KL_PHYS	(0xfffda000)
#define GPIO_MN_PHYS	(0xfffd9000)

 /* -- */

    /* Register values and bits */

#define RCPC_CTRL_UNLOCK	(0x263)	/* Unlock; CLKOUT <= HCLK; Active */
#define RCPC_CTRL_LOCK		(0x63)	/* Lock; CLKOUT <= HCLK; Active */
#define RCPC_CORECONFIG_FASTBUS	(3)

#if 1 // run the CPU at 50.8032 MHz, and the bus at 50.8032 MHz
# define RCPC_SYSPLLCNTL_V	(0x3049) /* 101.6064 MHz <= 11.2896 MHz * 9 */
# define RCPC_SYSCLKPRE_V	(1)		/* HCLK = PLL/2 */
# define RCPC_CPUCLKPRE_V	(1)		/* FCLK = PLL/2 */
# define RCPC_CORECONFIG_V	(0)		/* Standard, async clocking */
# define HCLK			(50803200) 	/* HCLK in Hz */
#else // run...faster?

#endif

#define RCPC_AHBCLKCTRL_V ((1<<4)|(1<<3)|(1<<2)) /* !LCD,!USB,!ETH,SDRAM,DMA */
#define RCPC_PCLKCTRL0_V  ((0<<9)|(1<<2)|(1<<1)|(0<<0)) /* RTC,!U2,!U1,U0 */
#define RCPC_PCLKCTRL1_V  ((1<<3)|(1<<2)|(1<<1)|(1<<0)) /*!USB,!ADC,!SSP,!LCD*/
#define RCPC_PCLKSEL0_V	  ((3<<7))			/* RTC 32KHz */

#define SDRAM_BANK0_PHYS	0x20000000
#define SDRAM_BANK1_PHYS	0x30000000

#define EMC_READCONFIG_CLKOUTDELAY (0<<0)	/* Undocumented */
#define EMC_READCONFIG_CMDDELAY	   (1<<0)	/* Undocumented */
#define EMC_READCONFIG_CMDDELAY_P1 (2<<0)	/* Undocumented */
#define EMC_READCONFIG_CMDDELAY_P2 (3<<0)	/* Undocumented */

#define IOCON_MUXCTL5_V		(0x0a00)	/* Enable UART0 */
#define IOCON_MUXCTL6_V		(0x000a)	/* Enable UART0 */
#define IOCON_MUXCTL7_V		(0x5555) 	/* Enable A23-A19 */
#define IOCON_MUXCTL10_V	(0x5555)	/* Enable D25-D21, D15-D13 */
#define IOCON_MUXCTL11_V	(0x5555) 	/* Enable D20-D17, D12-D9 */
#define IOCON_MUXCTL12_V	(0x5000)	/* Enable D16, D8 */
#define IOCON_MUXCTL14_V	(0x0000)	/* Normalize nCS0 */
#define IOCON_MUXCTL19_V	(0x0441)	/* Enable D31-D29 */
#define IOCON_MUXCTL20_V	(0x1110)	/* Enable D28-D26 */

		/* *** FIXME: works with 1:1 clock mode only */
#define NS_TO_HCLK(ns)	((ns)*(HCLK/1000)/1000000)

#define UART_FR_RXFE		(1<<4)
#define UART_FR_TXFF		(1<<5)
#define UART_FR_TXFE		(1<<7)
#define UART_DR_PE		(1<<9)
#define UART_DR_OE		(1<<11)
#define UART_DR_FE		(1<<8)
#define UART_FR_BUSY		(1<<3)
#define UART_CR_EN		(1<<0)
#define UART_CR_TXE		(1<<8)
#define UART_CR_RXE		(1<<9)
#define UART_LCR_WLEN8		(3<<5)
#define UART_LCR_FEN		(1<<4)
#define UART_DR_DATAMASK	(0xff)

#define RTC_CR_EN	(1)


#endif  /* __LH79524_H__ */
