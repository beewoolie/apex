/* debug_ll.h

   written by Marc Singer
   22 Feb 2007

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

#if !defined (__DEBUG_LL_H__)
#    define   __DEBUG_LL_H__

/* ----- Includes */

#include <config.h>
#include <asm/reg.h>
#include <mach/uart.h>

#if defined (CONFIG_MX31_UART1)
# define PUTC(c)\
  ({ __REG (UART + UART_TXD) = c;\
     while (!(__REG (UART + UART_SR2) & UART_SR2_TXDC)) ; })
#endif

#if defined (CONFIG_MX31ADS_UARTA)
# include "sc16c652.h"
# define PUTC(c)\
  ({ SC_UART_HR = c;\
     while (!(SC_UART_LSR & SC_UART_LSR_TXE)) ; })
#endif

#endif  /* __DEBUG_LL_H__ */
