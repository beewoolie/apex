/* serial.c
     $Id$

   written by Marc Singer
   1 Nov 2004

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

   This serial driver is nearly identitical for the lh79520 and the
   lh79524.  Since the only real difference is in the setup, the code
   is shared.

   *** FIXME: the device selection should be part of the platform
   *** header and not the CPU header.  The choice of the console
   *** device is made with the board layout.

*/

#include <config.h>
#include <driver.h>
#include <service.h>

#if defined (CONFIG_MACH_LH79524)
# include "lh79524.h"
#endif

#if defined (CONFIG_MACH_LH79520)
# include "lh79520.h"
#endif

extern struct driver_d* console_driver;

static struct driver_d lh7952x_serial_driver;

void lh7952x_serial_init (void)
{
  u32 baudrate = 115200;
  u32 divisor_i = 0;
  u32 divisor_f = 0;

#if defined (CONFIG_MACH_LH79524)
  switch (baudrate) {
  case 115200: 
    divisor_i = 6; divisor_f = 8; break;

  default:
    return;
  }
#endif

#if defined (CONFIG_MACH_LH79520)
  
  /* Enable ALL uarts since we don't know which we're using */
  __REG (IOCON_PHYS | IOCON_UARTMUX) = 0xf;
  __REG (RCPC_PHYS | RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS | RCPC_PERIPHCLKCTRL) &= 
    ~(RCPC_PERIPHCLK_U0 | RCPC_PERIPHCLK_U1 | RCPC_PERIPHCLK_U2);    
  __REG (RCPC_PHYS | RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;

  switch (baudrate) {
  case 115200: 
    divisor_i = 8; divisor_f = 0; break;

  default:
    return;
  }
#endif

//  while (__REG (UART + UART_FR) & UART_FR_BUSY)
//    ;
  
  __REG (UART + UART_CR) = UART_CR_EN; /* Enable UART without drivers */
  
  __REG (UART + UART_IBRD) = divisor_i;
  __REG (UART + UART_FBRD) = divisor_f;
  
  __REG (UART + UART_LCR_H) = UART_LCR_FEN | UART_LCR_WLEN8;
    
  __REG (UART + UART_IMSC) = 0x00; /* Mask interrupts */
  __REG (UART + UART_ICR)  = 0xff; /* Clear interrupts */

  __REG (UART + UART_CR) = UART_CR_EN | UART_CR_TXE | UART_CR_RXE;

  if (console_driver == 0)
    console_driver = &lh7952x_serial_driver;
}

ssize_t lh7952x_serial_poll (struct descriptor_d* d, size_t cb)
{
  return cb ? ((__REG (UART + UART_FR) & UART_FR_RXFE) ? 0 : 1) : 0;
}

ssize_t lh7952x_serial_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cRead = 0;
  unsigned char* pb;
  for (pb = (unsigned char*) pv; cb--; ++pb) {
    u32 v;

    while (__REG (UART + UART_FR) & UART_FR_RXFE)
      ;				/* block until character is available */

    v = __REG (UART + UART_DR);
    if (v & (UART_DR_FE | UART_DR_PE))
      return -1;		/* -ESERIAL */
    *pb = v & UART_DR_DATAMASK;
    ++cRead;
  }

  return cRead;
}

ssize_t lh7952x_serial_write (struct descriptor_d* d, 
			      const void* pv, size_t cb)
{
  ssize_t cWrote = 0;
  const unsigned char* pb = pv;
  for (pb = (unsigned char*) pv; cb--; ++pb) {

    while ((__REG (UART + UART_FR) & UART_FR_TXFF))
      ;

    __REG (UART + UART_DR) = *pb;

    ++cWrote;
  }
  return cWrote;
}

static __driver_0 struct driver_d lh7952x_serial_driver = {
  .name = "serial-lh7952x",
  .description = "lh7952x serial driver",
  .flags = DRIVER_SERIAL | DRIVER_CONSOLE,
  .read = lh7952x_serial_read,
  .write = lh7952x_serial_write,
  .poll = lh7952x_serial_poll,
};

static __service_1 struct service_d lh7952x_serial_service = {
  .init = lh7952x_serial_init,
};
