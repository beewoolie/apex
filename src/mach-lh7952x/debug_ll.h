/* debug_ll.h
     $Id$

   written by Marc Singer
   12 Feb 2005

   Copyright (C) 2005 The Buici Company

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

   This header is for debug only.  As such, it conflicts with the
   normal serial includes.  Thus these macros are protected from being
   included when the uart macros have previously been included.

   Do not include this header in source files.  Use 

      #include <debug_ll.h>

   which will get the global header that has additional macros.

*/

#if !defined (__DEBUG_LL_H__)
#    define   __DEBUG_LL_H__

/* ----- Includes */

#include <config.h>
#include <asm/reg.h>

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#if !defined (UART_PHYS)

#define UART_PHYS	(0xfffc0000)

#define UART0_PHYS	(UART_PHYS + 0x0000)
#define UART1_PHYS	(UART_PHYS + 0x1000)
#define UART2_PHYS	(UART_PHYS + 0x2000)

#if defined (CONFIG_MACH_LH79520)
# define UART		(UART1_PHYS)
#endif

#if defined (CONFIG_MACH_LH79524)
# define UART		(UART0_PHYS)
#endif

#endif

#if !defined (UART_DR)

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

#endif

#endif  /* __DEBUG_LL_H__ */
