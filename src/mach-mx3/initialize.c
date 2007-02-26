/* initialize.c

   written by Marc Singer
   22 Jan 2007

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

   Hardware initializations.  Some initialization may be left to
   drivers, such as the serial interface initialization.

*/

#include <config.h>
#include <asm/bootstrap.h>
#include <service.h>
#include <sdramboot.h>
#include <asm/cp15.h>

#include "hardware.h"
#include <debug_ll.h>

#define SDRAM_CMD_NORMAL	(0x80000000)
#define SDRAM_CMD_PRECHARGEALL	(0x80000001)
#define SDRAM_CMD_MODE		(0x80000002)
#define SDRAM_CMD_NOP		(0x80000003)

// The charging time should be at least 100ns.  Longer is OK as will be
// the case for other HCLK frequencies.

#define NS_TO_HCLK(ns)	((((ns)*((HCLK)/1000) + (1000000 - 1))/1000000))
#define WST(ns)		(NS_TO_HCLK((ns))-1)

#define SDRAM_REFRESH_CHARGING	(NS_TO_HCLK(100))
#define SDRAM_REFRESH		(HCLK/64000 - 1) // HCLK/64KHz - 1

#define SDCSC_CASLAT(v)		((v - 1)<<16) /* CAS latency */
#define SDCSC_RASTOCAS(v)	(v<<20) /* RAS to CAS latency */
#define SDCSC_BANKCOUNT_2	(0<<3) /* BankCount, 2 bank devices */
#define SDCSC_BANKCOUNT_4	(1<<3) /* BankCount, 4 bank devices */
#define SDCSC_EBW_16		(1<<2) /* ExternalBuswidth, 16 bit w/burst 8 */
#define SDCSC_EBW_32		(0<<2) /* ExternalBusWidth, 32 bit w/burst 4 */
#define SDCRC_AUTOPRECHARGE	(1<24)

#define SDRAM_CAS		2 /* By Micron specification */
#define SDRAM_RAS		3

#define SDRAM_CHIP_MODE		(((SDRAM_CAS << 4) | 2)<<10) /* BURST4 */

#define SDRAM_MODE_SETUP	( ((SDRAM_CAS - 1)<<16)\
				 | (SDRAM_RAS << 20)\
				 | SDCSC_EBW_32\
				 | SDCSC_BANKCOUNT_4\
				 | SDRAM_MODE_SROMLL)
#define SDRAM_MODE		(SDRAM_MODE_SETUP | SDCRC_AUTOPRECHARGE)


/* usleep

   this function accepts a count of microseconds and will wait at
   least that long before returning.  It depends on the timer being
   setup.

   Note that this function is neither __naked nor static.  It is
   available to the rest of the application as is.

 */

void __section (.bootstrap) usleep (unsigned long us)
{
  unsigned long c = (us - us/2);
  __asm volatile ("str %2, [%1, #8]\n\t"
		  "str %0, [%1, #0]\n\t"
		  "str %3, [%1, #8]\n\t"
	       "0: ldr %0, [%1, #4]\n\t"
		  "tst %0, %4\n\t"
		  "beq 0b\n\t"
		  : "+r" (c)
		  : "r" (PHYS_EPIT1),
		    "r" (0),
		    "r" ((1<<7)|(1<<3)), /* Enable, Free, 508 KHz */
		    "I" (0x8000)
		  : "cc");
}


/* initialize_bootstrap

   performs vital SDRAM initialization as well as some other memory
   controller initializations.  It will perform no work if we are
   already running from SDRAM.

   The return value is true if SDRAM has been initialized and false if
   this initialization has already been performed.  Note that the
   non-SDRAM initializations are performed regardless of whether or
   not we're running in SDRAM.

*/

void __naked __section (.bootstrap) initialize_bootstrap (void)
{
  unsigned long lr;
  __asm volatile ("mov %0, lr" : "=r" (lr));

  STORE_REMAP_PERIPHERAL_PORT (0x40000000 | 0x15); /* 1GiB @ 1GiB */

  __REG (PHYS_L2CC + 0x08) &= ~1; /* Disable L2CC, should be redundant */

  CCM_SET  = CCM_CTRL_399_V;
  CCM_L2MV = CCM_L2MV_399_V;

  //WM32  0x53FC0000 0x040                  ; setup ipu
  //WM32  0x53F80000 0x074B0B7D             ; init_ccm

  //;WM32  0xb8002050 0x0000dcf6            ; Configure PSRAM on CS5
  //;WM32  0xb8002054 0x444a4541
  //;WM32  0xb8002058 0x44443302
  //;WM32  0xB6000000 0xCAFECAFE


  CCM_CGR0 |= CCM_CGR_RUN << CCM_CGR0_EPIT1_SH;	/* Enable EPIT1 clock  */

  /* NOR flash initialization, though this shouldn't be necessary
     unless we're going to reduce timing latencies.  */
  WEIM_UCR(0) = 0x0000CC03; // ; Start 16 bit NorFlash on CS0
  WEIM_LCR(0) = 0xa0330D01; //
  WEIM_ACR(0) = 0x00220800; //
  WEIM_UCR(4) = 0x0000DCF6; // ; Configure CPLD on CS4
  WEIM_LCR(4) = 0x444A4541; //
  WEIM_ACR(4) = 0x44443302; //

#if defined (CONFIG_STARTUP_UART)
# define _DIVISOR (SC_UART_CLK/(115200*16))
  SC_UART_LCR = SC_UART_LCR_WLEN_8 | SC_UART_LCR_DLE;
  SC_UART_DLL = _DIVISOR & 0xff;
  SC_UART_DLM = ((_DIVISOR >> 8) & 0xff);
  SC_UART_LCR = SC_UART_LCR_WLEN_8;
  SC_UART_IER = 0;			/* Mask all interrupts */
  SC_UART_FCR = SC_UART_FCR_FEN | SC_UART_FCR_RX_RESET | SC_UART_FCR_TX_RESET;
  SC_UART_LSR = 0;

  PUTC('A');
# undef _DIVISOR
#endif

  LED_ON (0);

  __asm volatile ("cmp %0, %1\n\t"
#if defined (CONFIG_SDRAMBOOT_REPORT)
		  "movhi r0, #1\n\t"
		  "strhi r0, [%2]\n\t"
#endif
		  "movhi r0, #0\n\t"
		  "movhi pc, %0\n\t"
		  "1:" :: "r" (lr), "I" (SDRAM_BANK0_PHYS)
#if defined (CONFIG_SDRAMBOOT_REPORT)
		  , "r" (&fSDRAMBoot)
#endif
		  : "cc");

  PUTC_LL ('S');

	/* Initialize SDRAM */
  __REG (0x43FAC26C) = 0; // ; SDCLK
  __REG (0x43FAC270) = 0; // ; CAS
  __REG (0x43FAC274) = 0; // ; RAS
  __REG (0x43FAC27C) = 0x1000; // ; CS2 (CSD0)
  __REG (0x43FAC284) = 0; // ; DQM3
  __REG (0x43FAC288) = 0; // ; DQM2, DQM1, DQM0, SD31-SD0, A25-A0, MA10 (0x288..0x2DC)
  __REG (0x43FAC28C) = 0; //
  __REG (0x43FAC290) = 0; //
  __REG (0x43FAC294) = 0; //
  __REG (0x43FAC298) = 0; //
  __REG (0x43FAC29C) = 0; //
  __REG (0x43FAC2A0) = 0; //
  __REG (0x43FAC2A4) = 0; //
  __REG (0x43FAC2A8) = 0; //
  __REG (0x43FAC2AC) = 0; //
  __REG (0x43FAC2B0) = 0; //
  __REG (0x43FAC2B4) = 0; //
  __REG (0x43FAC2B8) = 0; //
  __REG (0x43FAC2BC) = 0; //
  __REG (0x43FAC2C0) = 0; //
  __REG (0x43FAC2C4) = 0; //
  __REG (0x43FAC2C8) = 0; //
  __REG (0x43FAC2CC) = 0; //
  __REG (0x43FAC2D0) = 0; //
  __REG (0x43FAC2D4) = 0; //
  __REG (0x43FAC2D8) = 0; //
  __REG (0x43FAC2DC) = 0; //
  __REG (0xB8001010) = 0x00000004; // ; Initialization script for 32 bit DDR on Tortola EVB
  __REG (0xB8001004) = 0x006ac73a; //
  __REG (0xB8001000) = 0x92100000; //
  __REG (0x80000f00) = 0x12344321; //
  __REG (0xB8001000) = 0xa2100000; //
  __REG (0x80000000) = 0x12344321; //
  __REG (0x80000000) = 0x12344321; //
  __REG (0xB8001000) = 0xb2100000; //
  __REG (0x80000033) = 0xda; //
  __REG (0x81000000) = 0xff; //
  __REG (0xB8001000) = 0x82226080; //
  __REG (0x80000000) = 0xDEADBEEF; //
  __REG (0xB8001010) = 0x0000000c; //
  //WGPR  15         0x83F00000             ; boot secret

  PUTC_LL ('s');

#if defined (CONFIG_SDRAMBOOT_REPORT)
  barrier ();
  fSDRAMBoot = 0;
#endif

  __asm volatile ("mov r0, #-1\t\n"
		  "mov pc, %0" : : "r" (lr));
}


/* target_init

   performs the rest of the hardware initialization that didn't have
   to be performed during the bootstrap phase.

*/

static void target_init (void)
{
}


static void target_release (void)
{
}

static __service_0 struct service_d mx3x_target_service = {
  .init    = target_init,
  .release = target_release,
};
