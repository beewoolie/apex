/* debug_ll.h
     $Id$

   written by Marc Singer
   10 Mar 2005

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

#if !defined (RCPC_PHYS)

#define RCPC_PHYS		(0xfffe2000)
#define RCPC_CTRL		__REG(RCPC_PHYS + 0x00)
#define RCPC_CTRL_UNLOCK	(1<<9)
#define RCPC_PCLKCTRL0		__REG(RCPC_PHYS + 0x00)
#define RCPC_PCLKCTRL0_U0	(1<<0)

#endif

#if !defined (UART_PHYS)

#define UART1_PHYS	(0x80000600)
#define UART2_PHYS	(0x80000700)
#define UART3_PHYS	(0x80000800)

#define UART		(UART2_PHYS)

#endif

#if !defined (UART_DATA)

#define UART_DATA		__REG(UART + 0x00)
#define UART_FCON		__REG(UART + 0x04)
#define UART_BRCON		__REG(UART + 0x08)
#define UART_CON		__REG(UART + 0x0c)
#define UART_STATUS		__REG(UART + 0x10)
#define UART_RAWISR		__REG(UART + 0x14)
#define UART_INTEN		__REG(UART + 0x18)
#define UART_ISR		__REG(UART + 0x1c)

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

#endif

#define PUTC_LL(c)	({ UART_DATA = c; \
			   while (UART_STATUS & UART_STATUS_BUSY) ; })

#endif  /* __DEBUG_LL_H__ */
