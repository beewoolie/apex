/* lh79520.h
     $Id$

   written by Marc Singer
   14 Nov 2004

   Copyright (C) 2004 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__LH79520_H__)
#    define   __LH79520_H__

/* ----- Includes */

#include <asm/reg.h>

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#define RCPC_PHYS	0xfffe2000

#define RCPC_CTRL			0x00
#define RCPC_CHIPID			0x04
#define RCPC_REMAP			0x08
#define RCPC_SOFTRESET			0x0c
#define RCPC_RESETSTATUS		0x10
#define RCPC_RESETSTATUSCLR		0x14
#define RCPC_HCLKCLKPRESCALE		0x18
#define RCPC_CPUCLKPRESCALE		0x1C
#define RCPC_PERIPHCLKCTRL		0x24
#define RCPC_PERIPHCLKCTRL2		0x28
#define RCPC_AHBCLKCTRL			0x2C
#define RCPC_PERIPHCLKSEL		0x30
#define RCPC_PERIPHCLKSEL2		0x34
#define RCPC_CORECLKCONFIG		0x88

#define RCPC_CTRL			0x00
#define RCPC_SOFTRESET			0x0c

#define RCPC_CTRL_UNLOCK		(1<<9)
#define RCPC_AHBCLK_SDC			(1<<1)
#define RCPC_AHBCLK_DMA			(1<<0)
#define RCPC_PERIPHCLK_T02		(1<<5)
#define RCPC_PERIPHCLK_T01		(1<<4)
#define RCPC_PERIPHCLK_U2		(1<<2)
#define RCPC_PERIPHCLK_U1		(1<<1)
#define RCPC_PERIPHCLK_U0		(1<<0)
#define RCPC_PERIPHCLK_RTC_32KHZ	(3<<7)
#define RCPC_PERIPHCLK_RTC_1HZ		(0<<7)
#define RCPC_PERIPHCLK_RTC_MASK		(3<<7)

#define RCPC_CPUCLKPRESCALE_78		0x2
#define RCPC_CPUCLKPRESCALE_52		0x3
#define RCPC_CPUCLKPRESCALE_39		0x4
#define RCPC_HCLKCLKPRESCALE_52		0x3
#define RCPC_HCLKCLKPRESCALE_39		0x4
//#define RCPC_CPUCLK_PRESCALE_DEFAULT		RCPC_CPUCLK_PRESCALE_52
//#define RCPC_HCLK_PRESCALE_DEFAULT		RCPC_HCLK_PRESCALE_52

#if 0 // run the CPU at 52 MHz, and the bus at 52 MHz
# define RCPC_CPUCLKPRESCALE_V	RCPC_CPUCLKPRESCALE_52
# define RCPC_HCLKHCLKPRESCALE_V	RCPC_HCLKCLKPRESCALE_52
# define RCPC_CORECLKCONFIG_V	3			/* FastBus */
# define HCLK_FREQ		(309657600/6)
#else // run the CPU at 78 MHz, and the Bus at 52 MHz
# define RCPC_CPUCLKPRESCALE_V	RCPC_CPUCLKPRESCALE_78
# define RCPC_HCLKCLKPRESCALE_V	RCPC_HCLKCLKPRESCALE_52
# define RCPC_CORECLKCONFIG_V	0			/* std mode, async */
# define HCLK_FREQ		(309657600/6)
#endif

#define SDRC_REFTIMER_V	((HCLK_FREQ*16)/1000000)

#define IOCON_PHYS	0xfffe5000
#define IOCON_MEMMUX	0x00
#define IOCON_UARTMUX	0x10

#define SMC_PHYS	0xffff1000
#define SMC_BCR0	0x00
#define SMC_BCR1	0x04
#define SMC_BCR2	0x08
#define SMC_BCR3	0x0c
#define SMC_BCR4	0x10
#define SMC_BCR5	0x14
#define SMC_BCR6	0x18

#define SDRAM_BANK0_PHYS	0x20000000
#define SDRAM_BANK1_PHYS	0x28000000

#define SDRC_PHYS	0xffff2000
#define SDRC_CONFIG0	0x00
#define SDRC_CONFIG1	0x04
#define SDRC_REFTIMER	0x08
#define SDRC_WBTIMEOUT	0x0c

#define TIMER0_PHYS	0xfffc4000
#define TIMER1_PHYS	0xfffc4020
#define TIMER2_PHYS	0xfffc5000
#define TIMER3_PHYS	0xfffc4020

#define TIMER_LOAD	0x00
#define TIMER_VALUE	0x04
#define TIMER_CONTROL	0x08
#define TIMER_CLEAR	0x0c

#define TIMER_ENABLE	(1<<7)
#define TIMER_PERIODIC	(1<<6)	// !FREERUNNING
#define TIMER_CASCADE	(1<<4)
#define TIMER_SCALE_MASK (3<<2)
#define TIMER_SCALE_1	 (0<<2)
#define TIMER_SCALE_16	 (1<<2)
#define TIMER_SCALE_256	 (2<<2)

#define RTC_PHYS	(0xfffe0000)
#define RTC_DR		(0x00)

#define UART_PHYS	(0xfffc0000)

#define UART0_PHYS	(UART_PHYS + 0x0000)
#define UART1_PHYS	(UART_PHYS + 0x1000)
#define UART2_PHYS	(UART_PHYS + 0x2000)
#define UART		(UART1_PHYS)
#define UART_DR		(0x00)
#define UART_IBRD	(0x24)
#define UART_FBRD	(0x28)
#define UART_LCR_H	(0x2c)
#define UART_CR		(0x30)
#define UART_FR		(0x18)
#define UART_IMSC	(0x38)
#define UART_ICR	(0x44)

#define UART_FR_TXFE		(1<<7)
#define UART_FR_RXFF		(1<<6)
#define UART_FR_TXFF		(1<<5)
#define UART_FR_RXFE		(1<<4)
#define UART_FR_BUSY		(1<<3)
#define UART_DR_PE		(1<<9)
#define UART_DR_OE		(1<<11)
#define UART_DR_FE		(1<<8)
#define UART_CR_EN		(1<<0)
#define UART_CR_TXE		(1<<8)
#define UART_CR_RXE		(1<<9)
#define UART_LCR_WLEN8		(3<<5)
#define UART_LCR_FEN		(1<<4)
#define UART_DR_DATAMASK	(0xff)

#endif  /* __LH79520_H__ */
