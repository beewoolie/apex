/* serial-mx51.c

   written by Marc Singer
   2 July 2011

   Copyright (C) 2011 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   Serial driver for the internal UART on the MX51x chips.  This is a
   replica of the serial-mx3 driver, but don't let that lull you into
   thinking it is a good idea to merge them.  This is a small piece of
   code.  Let it be OK that there is some duplication for the sake of
   stability.


EfikaSB# md 0x73fbc080 1                             
73fbc080: 00000001    ....                           
EfikaSB#                       md 0x73fbc080 1
73fbc080: 00000001    ....                    
EfikaSB# md 0x73fbc084 1                 
73fbc084: 00004027    '@..               
EfikaSB# md 0x73fbc088 1                 
73fbc088: 00000704    ....               
EfikaSB# md 0x73fbc08c 1                 
73fbc08c: 00008000    ....               
EfikaSB# md 0x73fbc0a4 1                 
73fbc0a4: 0000000f    ....               
EfikaSB# md 0x73fbc0a8 1                 
73fbc0a8: 00000120     ...
EfikaSB# md 0x73fbc0ac 1                                    
73fbc0ac: 00000004    ....                   
EfikaSB#    md 0x73fbc090 1              
73fbc090: 00000200    ....               


*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <apex.h>		/* printf, only */
#include "hardware.h"

#include "uart.h"


void mx51_serial_init (void)
{
  GPIO_CONFIG_FUNC (MX51_PIN_UART1_RXD, 0);
  GPIO_CONFIG_PAD  (MX51_PIN_UART1_RXD,
                    GPIO_PAD_HYST_EN | GPIO_PAD_PKE | GPIO_PAD_PUE
                    | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_SLEW_FAST);
  GPIO_CONFIG_FUNC (MX51_PIN_UART1_TXD, 0);
  GPIO_CONFIG_PAD  (MX51_PIN_UART1_TXD,
                    GPIO_PAD_HYST_EN | GPIO_PAD_PKE | GPIO_PAD_PUE
                    | GPIO_PAD_DRIVE_HIGH | GPIO_PAD_SLEW_FAST);
  GPIO_CONFIG_FUNC (MX51_PIN_UART1_RTS, 0);
  GPIO_CONFIG_PAD  (MX51_PIN_UART1_RTS,
                    GPIO_PAD_HYST_EN | GPIO_PAD_PKE | GPIO_PAD_PUE
                    | GPIO_PAD_DRIVE_HIGH);
  GPIO_CONFIG_FUNC (MX51_PIN_UART1_CTS, 0);
  GPIO_CONFIG_PAD  (MX51_PIN_UART1_CTS,
                    GPIO_PAD_HYST_EN | GPIO_PAD_PKE | GPIO_PAD_PUE
                    | GPIO_PAD_DRIVE_HIGH);

  INITIALIZE_CONSOLE_UART;
}

void mx51_serial_release (void)
{
  /* Wait for transmitter to go idle */
  while (!(__REG (UART + UART_SR2) & UART_SR2_TXDC))
    ;
}

ssize_t mx51_serial_poll (struct descriptor_d* d, size_t cb)
{
  return cb
    ? ((__REG (UART + UART_SR2) & UART_SR2_RDR) ? 1 : 0)
    : 0;
}

ssize_t mx51_serial_read (struct descriptor_d* d, void* pv, size_t cb)
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

ssize_t mx51_serial_write (struct descriptor_d* d, const void* pv, size_t cb)
{
  ssize_t cWrote = 0;
  const unsigned char* pb = pv;
  for (pb = (unsigned char*) pv; cb--; ++pb) {

//    while (!(__REG (UART + UART_SR2) & UART_SR2_TXFE))
    while (!(__REG (UART + UART_SR1) & UART_SR1_TRDY))
      ;				/* Wait for room in the FIFO */

    __REG (UART + UART_TXD) = *pb;

    ++cWrote;
  }

  return cWrote;
}

static __driver_0 struct driver_d mx51_serial_driver = {
  .name        = "serial-mx51",
  .description = "mx51 serial driver",
  .flags       = DRIVER_SERIAL | DRIVER_CONSOLE,
  .open        = open_helper,   /* Always succeed */
  .close       = close_helper,
  .read        = mx51_serial_read,
  .write       = mx51_serial_write,
  .poll        = mx51_serial_poll,
};

static __service_3 struct service_d mx51_serial_service = {
  .init        = mx51_serial_init,
  .release     = mx51_serial_release,
};
