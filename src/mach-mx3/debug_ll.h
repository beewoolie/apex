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

# define INITIALIZE_CONSOLE_UART\
  ({ MASK_AND_SET (__REG (0x43fac080), 0xffff, 0x1210); /* txd1/rxd1 */\
     __REG (UART + UART_CR1) = 0;\
     __REG (UART + UART_CR2) = UART_CR2_IRTS | UART_CR2_CTSC | UART_CR2_WS\
			     | UART_CR2_TXEN | UART_CR2_RXEN;\
     __REG (UART + UART_CR3) = UART_CR3_RXDMUXSEL;\
     __REG (UART + UART_CR4) = (32<<UART_CR4_CTSTL_SH)\
			     | UART_CR4_LPBYP | UART_CR4_DREN;\
     __REG (UART + UART_FCR) = (1<<UART_FCR_RXTL_SH)\
			     | (0<<UART_FCR_RFDIV_SH)\
			     | (16<<UART_FCR_TXTL_SH);\
     __REG (UART + UART_SR1) = ~0;\
     __REG (UART + UART_SR2) = ~0;\
     __REG (UART + UART_BIR) = UART_BIR_115200;\
     __REG (UART + UART_BMR) = UART_BMR_115200;\
     __REG (UART + UART_CR1) = UART_CR1_EN;\
     CPLD_CTRL1_CLR = 1<<4;\
     CPLD_CTRL2_SET = 1<<2;\
   })

#endif

#if defined (CONFIG_MX31ADS_UARTA)
# include "sc16c652.h"
# define PUTC(c)\
  ({ SC_UART_HR = c;\
     while (!(SC_UART_LSR & SC_UART_LSR_TXE)) ; })

# define UART_DIVISOR (SC_UART_CLK/(115200*16))
//  CPLD_CTRL1_SET = CPLD_CTRL1_XUART_RST;
//  usleep (50);
//  CPLD_CTRL1_CLR = CPLD_CTRL1_XUART_RST;
//  usleep (50);
  //  CPLD_CTRL1_CLR = CPLD_CTRL1_UARTC_EN;
# define INITIALIZE_CONSOLE_UART\
  ({ SC_UART_LCR = SC_UART_LCR_WLEN_8 | SC_UART_LCR_DLE;\
     SC_UART_DLL = UART_DIVISOR & 0xff;\
     SC_UART_DLM = ((UART_DIVISOR >> 8) & 0xff);\
     SC_UART_LCR = SC_UART_LCR_WLEN_8;\
     SC_UART_IER = 0;\
     SC_UART_FCR = SC_UART_FCR_FEN | SC_UART_FCR_RX_RESET\
       | SC_UART_FCR_TX_RESET;\
     SC_UART_LSR = 0; })

#endif

#endif  /* __DEBUG_LL_H__ */
