/* serial.c
     $Id$

   written by Marc Singer
   1 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#include <driver.h>
#include <service.h>
#include "lh79524.h"

extern struct driver_d* console_driver;

static struct driver_d lh79524_serial_driver;

void lh79524_serial_init (void)
{
  u32 baudrate = 115200;
  u32 divisor_i = 0;
  u32 divisor_f = 0;

  switch (baudrate) {
  case 115200: 
    divisor_i = 6; divisor_f = 8; break;

  default:
    return;
  }

  while (__REG (UART + UART_FR) & UART_FR_BUSY)
    ;
  
  __REG (UART + UART_CR) = UART_CR_EN; /* Enable UART without drivers */
  
  __REG (UART + UART_IBRD) = divisor_i;
  __REG (UART + UART_FBRD) = divisor_f;
  
  __REG (UART + UART_LCR_H) = UART_LCR_FEN | UART_LCR_WLEN8;
    
  __REG (UART + UART_IMSC) = 0x00; /* Mask interrupts */
  __REG (UART + UART_ICR)  = 0xff; /* Clear interrupts */

  __REG (UART + UART_CR) = UART_CR_EN | UART_CR_TXE | UART_CR_RXE;

  if (console_driver == 0)
    console_driver = &lh79524_serial_driver;
}

ssize_t lh79524_serial_poll (struct descriptor_d* d, size_t cb)
{
  return cb ? ((__REG (UART + UART_FR) & UART_FR_RXFE) ? 0 : 1) : 0;
}

ssize_t lh79524_serial_read (struct descriptor_d* d, void* pv, size_t cb)
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

ssize_t lh79524_serial_write (struct descriptor_d* d, 
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

static __driver_0 struct driver_d lh79524_serial_driver = {
  .name = "serial-lh79524",
  .description = "lh79524 serial driver",
  .flags = DRIVER_SERIAL | DRIVER_CONSOLE,
  .read = lh79524_serial_read,
  .write = lh79524_serial_write,
  .poll = lh79524_serial_poll,
};

static __service_1 struct service_d lh79524_serial_service = {
  .init = lh79524_serial_init,
};
