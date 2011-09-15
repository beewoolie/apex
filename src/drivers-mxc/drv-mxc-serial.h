/** @file drivers-mxc/drv-mxc-serial.h

   written by Marc Singer
   15 Sep 2011

   Copyright (C) 2011 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (DRIVERS_MXC_DRV_MXC_SERIAL_H_INCLUDED)
#    define   DRIVERS_MXC_DRV_MXC_SERIAL_H_INCLUDED

/* ----- Includes */

/* ----- Macros */

#define UART_RXD		0x00
#define UART_TXD		0x40
#define UART_CR1		0x80
#define UART_CR2		0x84
#define UART_CR3		0x88
#define UART_CR4		0x8c
#define UART_FCR		0x90
#define UART_SR1		0x94
#define UART_SR2		0x98
#define UART_ESC		0x9c
#define UART_TIM		0xa0
#define UART_BIR		0xa4
#define UART_BMR		0xa8
#define UART_BCR		0xac
#define UART_ONEMS		0xb0
#define UART_TEST		0xb4

#define UART_RXD_CHARRDY	(1<<15)
#define UART_RXD_ERR		(1<<14)
#define UART_RXD_OVRRUN		(1<<13)
#define UART_RXD_FRMERR		(1<<12)
#define UART_RXD_BRK		(1<<11)
#define UART_RXD_PRERR		(1<<10)

#define UART_CR1_EN		(1<<0)		/* UART Enable */
#define UART_CR2_IRTS		(1<<14)		/* Ignore RTS */
#define UART_CR2_CTSC		(1<<13)		/* Receiver controlled CTS */
#define UART_CR2_WS		(1<<5)		/* Word size 8 */
#define UART_CR2_TXEN		(1<<2)		/* Transmitter enable */
#define UART_CR2_RXEN		(1<<1)		/* Receiver enable */
#define UART_CR2_NSRST		(1<<0)		/* No software reset */
#define UART_CR3_RXDMUXSEL	(1<<2)		/* Enable MUX pins */
#define UART_CR4_LPBYP		(1<<4)		/* Low-power bypass */
#define UART_CR4_DREN		(1<<0)		/* Data ready intr. enable */
#define UART_CR4_CTSTL_SH	(10)

#define UART_FCR_RXTL_SH	(0)
#define UART_FCR_RFDIV_SH	(7)
#define UART_FCR_TXTL_SH	(10)
#define UART_FCR_DTE		(1<<6)		/* Select DTE mode */

#define UART_SR1_TRDY		(1<<13)		/* Transmit ready */
#define UART_SR1_RRDY		(1<<9)		/* Receiver ready */
#define UART_SR2_TXFE		(1<<14)		/* Transmit fifo empty */
#define UART_SR2_TXDC		(1<<3)		/* Transmission complete */
#define UART_SR2_RDR		(1<<0)		/* Receive Data Ready */

/* ----- Globals */

/* ----- Prototypes */



#endif  /* DRIVERS_MXC_DRV_MXC_SERIAL_H_INCLUDED */
