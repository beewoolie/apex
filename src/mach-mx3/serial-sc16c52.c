/* serial-sc16c652.c

   written by Marc Singer
   16 December 2006

   Copyright (C) 2006 Marc Singer

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

   Serial driver for the external DUART on the MX31ADS development
   board.

*/

#include <driver.h>
#include <service.h>
#include <apex.h>		/* printf, only */
#include "hardware.h"

		/* External DUART SC16C652 */
#define UARTA_PHYS		(0xb4010000) /* DUART port A */
#define UARTB_PHYS		(0xb4010010) /* DUART port B */
#define UART_PHYS		UARTA_PHYS

#define UART_HR			__REG8(UART + 0x00)
#define UART_IER		__REG8(UART + 0x01)
#define UART_ISR		__REG8(UART + 0x02)
#define UART_FCR		__REG8(UART + 0x02)
#define UART_LCR		__REG8(UART + 0x03)
#define UART_MCR		__REG8(UART + 0x04)
#define UART_LSR		__REG8(UART + 0x05)
#define UART_MSR		__REG8(UART + 0x06)
#define UART_SPR		__REG8(UART + 0x07)

#define UART_DLL		__REG8(UART + 0x00)
#define UART_DLM		__REG8(UART + 0x01)

#define UART_EFR		__REG8(UART + 0x02)
#define UART_XON1		__REG8(UART + 0x04)
#define UART_XON2		__REG8(UART + 0x05)
#define UART_XOFF1		__REG8(UART + 0x06)
#define UART_XOFF2		__REG8(UART + 0x07)

#define UART_FCR_FEN		(1<<0)
#define UART_FCR_RX_RESET	(1<<1)
#define UART_FCR_TX_RESET	(1<<2)
#define UART_FCR_DMA		(1<<3)

#define UART_LCR_WLEN_SH	(0) /* word length shift */
#define UART_LCR_WLEN_MASK	(0x3 << 0)
#define UART_LCR_WLEN_8		(0x3 << 0)
#define UART_LCR_STOP		(1<<2)
#define UART_LCR_PEN		(1<<3) /* parity enable */
#define UART_LCR_P_EVEN		(1<<4)
#define UART_LCR_P_SET		(1<<5)
#define UART_LCR_BREAK		(1<<6)
#define UART_LCR_DLE		(1<<7) /* divisor latch enable */

#define UART_LSR_RDR		(1<<0) /* receive data ready */
#define UART_LSR_OVR		(1<<1) /* overrun */
#define UART_LSR_PE		(1<<2) /* parity error */
#define UART_LSR_FE		(1<<3) /* framing error */
#define UART_LSR_BI		(1<<4) /* break interrupt */
#define UART_LSR_TXE		(1<<5) /* transmit fifo empty */
#define UART_LSR_TXIDLE		(1<<6) /* transmitter idle */
#define UART_LSR_FIFOE		(1<<7) /* fifo data error */

#define UART_EFR_CTS		(1<<7) /* auto cts */
#define UART_EFR_RTS		(1<<6) /* auto rts */
#define UART_EFR_EFEN		(1<<4) /* enhanced function enable */

#define UART_CLK		((int) (1.8432*1000*1000))

//#include <debug_ll.h>

extern struct driver_d* console_driver;

static struct driver_d sc16c652_serial_driver;

void sc16c652_serial_init (void)
{
  u32 baudrate = 115200;
  u32 divisor = UART_CLK/(baudrate*16);

  u8 lcr = UART_LCR_WLEN_8;

  UART_LCR = lcr | UART_LCR_DLE;
  UART_DLL = divisor & 0xff;
  UART_DLM = ((divisor >> 8) & 0xff);
  UART_LCR = lcr;

  UART_IER = 0;			/* Mask all interrupts */
  UART_FCR = UART_FCR_FEN | UART_FCR_RX_RESET | UART_FCR_TX_RESET;
  UART_LSR = 0;

  if (console_driver == 0)
    console_driver = &sc16c652_serial_driver;

//  sc16c652_serial_driver.write (NULL, "serial console initialized\r\n", 28);
//  printf ("Really, it is\n");
}

void sc16c652_serial_release (void)
{
	/* flush serial output */
  while (!(UART_LSR & UART_LSR_TXIDLE))
    ;
}

ssize_t sc16c652_serial_poll (struct descriptor_d* d, size_t cb)
{
  return cb
    ? ((UART_LSR & UART_LSR_RDR) ? 1 : 0)
    : 0;
}

ssize_t sc16c652_serial_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cRead = 0;
  unsigned char* pb;
  for (pb = (unsigned char*) pv; cb--; ++pb) {
    u8 v;
    u8 lsr;
    while (!(UART_LSR & UART_LSR_RDR))
      ;				/* block until character is available */

    lsr = UART_LSR;
    v = UART_HR;
    if (lsr & (UART_LSR_OVR | UART_LSR_PE | UART_LSR_FE))
      return -1;		/* -ESERIAL */
    *pb = v;
    ++cRead;
  }

  return cRead;
}

ssize_t sc16c652_serial_write (struct descriptor_d* d,
			      const void* pv, size_t cb)
{
  ssize_t cWrote = 0;
  const unsigned char* pb = pv;
  for (pb = (unsigned char*) pv; cb--; ++pb) {

    while (!(UART_LSR & UART_LSR_TXE))
      ;

    UART_DATA = *pb;

    ++cWrote;
  }

#if 1
	/* flush serial output */
  while (!(UART_LSR & UART_LSR_TXIDLE))
    ;
#endif

  return cWrote;
}

static __driver_0 struct driver_d sc16c652_serial_driver = {
  .name = "serial-sc16c652",
  .description = "sc16c652 serial driver",
  .flags = DRIVER_SERIAL | DRIVER_CONSOLE,
  .read = sc16c652_serial_read,
  .write = sc16c652_serial_write,
  .poll = sc16c652_serial_poll,
};

static __service_3 struct service_d sc16c652_serial_service = {
  .init = sc16c652_serial_init,
  .release = sc16c652_serial_release,
};
