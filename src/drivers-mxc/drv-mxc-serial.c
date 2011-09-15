/* drivers-mxc/drv-mxc-serial.c

   written by Marc Singer
   15 Sep 2011

   Copyright (C) 2011 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   Serial (UART) driver for the internal UART of MX31 and MX51 SoCs.
   This is probably a workable driver for all iMX parts.  But, it is
   acceptable for a new port to clone the driver at first until the
   platform is up.

   UARTB support exists s.t. output is sent to both UART and UARTB.
   This was added for the sake of a platform that had more than one
   UART that was typically used to support the console.  It is
   unlikely to be useful for most designs.

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <apex.h>		/* printf, only */
#include "drv-mxc-serial.h"
#include <mach/drv-mxc-serial.h>

#if !defined (INITIALIZE_CONSOLE_UARTB)
# define INITIALIZE_CONSOLE_UARTB
#endif

void mxc_serial_init (void)
{
  INITIALIZE_CONSOLE_UART;
  INITIALIZE_CONSOLE_UARTB;     /* May be NOP */
}

void mxc_serial_release (void)
{
  /* Wait for transmitter to go idle */
  while (!(__REG (UART + UART_SR2) & UART_SR2_TXDC))
    ;
}

ssize_t mxc_serial_poll (struct descriptor_d* d, size_t cb)
{
  return cb
    ? ((__REG (UART + UART_SR2) & UART_SR2_RDR) ? 1 : 0)
    : 0;
}

ssize_t mxc_serial_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cRead = 0;
  unsigned char* pb;
  for (pb = (unsigned char*) pv; cb--; ++pb) {
    u32 v;
    while (!(__REG (UART + UART_SR2) & UART_SR2_RDR))
      ;				/* block until character is available */

    v = __REG (UART + UART_RXD);
    if (v & UART_RXD_ERR)
      return -1;	/* -ESERIAL */
    *pb = v;
    ++cRead;
  }

  return cRead;
}

ssize_t mxc_serial_write (struct descriptor_d* d, const void* pv, size_t cb)
{
  ssize_t cWrote = 0;
  const unsigned char* pb = pv;
  for (pb = (unsigned char*) pv; cb--; ++pb) {

    while (!(__REG (UART + UART_SR1) & UART_SR1_TRDY))
      ;				/* Wait for room in the FIFO */

    __REG (UART + UART_TXD) = *pb;
#if defined (UARTB)
    __REG (UARTB + UART_TXD) = *pb;
#endif

    ++cWrote;
  }

  return cWrote;
}

static __driver_0 struct driver_d mxc_serial_driver = {
  .name = "serial-mxc",
  .description = "mxc serial driver",
  .flags = DRIVER_SERIAL | DRIVER_CONSOLE,
  .open = open_helper,          /* Always succeed */
  .close = close_helper,
  .read = mxc_serial_read,
  .write = mxc_serial_write,
  .poll = mxc_serial_poll,
};

static __service_3 struct service_d mxc_serial_service = {
  .init = mxc_serial_init,
  .release = mxc_serial_release,
};
