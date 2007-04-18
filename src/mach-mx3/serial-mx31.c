/* serial-mx31.c

   written by Marc Singer
   1 March 2007

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

   Serial driver for the internal UART on the MX31 chip.

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <apex.h>		/* printf, only */
#include "hardware.h"

#include "uart.h"

extern struct driver_d* console_driver;

static struct driver_d mx31_serial_driver;

void mx31_serial_init (void)
{
  INITIALIZE_CONSOLE_UART;

  if (console_driver == 0)
    console_driver = &mx31_serial_driver;
}

void mx31_serial_release (void)
{
  /* Wait for transmitter to go idle */
  while (!(__REG (UART + UART_SR2) & UART_SR2_TXDC))
    ;
}

ssize_t mx31_serial_poll (struct descriptor_d* d, size_t cb)
{
  return cb
    ? ((__REG (UART + UART_SR2) & UART_SR2_RDR) ? 1 : 0)
    : 0;
}

ssize_t mx31_serial_read (struct descriptor_d* d, void* pv, size_t cb)
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

ssize_t mx31_serial_write (struct descriptor_d* d,
			      const void* pv, size_t cb)
{
  ssize_t cWrote = 0;
  const unsigned char* pb = pv;
  for (pb = (unsigned char*) pv; cb--; ++pb) {

    while (!(__REG (UART + UART_SR1) & UART_SR1_TRDY))
      ;				/* Wait for room in the FIFO */

    __REG (UART + UART_TXD) = *pb;

    ++cWrote;
  }

  return cWrote;
}

static __driver_0 struct driver_d mx31_serial_driver = {
  .name = "serial-mx31",
  .description = "mx31 serial driver",
  .flags = DRIVER_SERIAL | DRIVER_CONSOLE,
  .read = mx31_serial_read,
  .write = mx31_serial_write,
  .poll = mx31_serial_poll,
};

static __service_3 struct service_d mx31_serial_service = {
  .init = mx31_serial_init,
  .release = mx31_serial_release,
};
