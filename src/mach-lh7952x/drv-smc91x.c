/* drv-smc91x.c
     $Id$

   written by Marc Singer
   26 Jul 2005

   Copyright (C) 2005 Marc Singer


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

   Driver for the SMSC 91CX series of integrated Ethernet interfaces,
   MAC and PHY.

   NOTES
   =====

   RESET

     o The chip requires that the reset pin be held high for 100ns in
       order to be recognized [7.8].
     o There doesn't appear to be a timing constraint on the use of
       the soft reset.  Moreover, the soft reset appears to be
       identitical to the hard reset in terms of functionality.
     o The PHY is ready for normal operation 50ms after a reset.

*/

#include <apex.h>
#include <config.h>
#include <driver.h>
#include <service.h>
#include <command.h>
#include <error.h>
#include <linux/string.h>
#include "hardware.h"
#include <linux/kernel.h>

#include <mach/drv-smc91x.h>

#define TALK 1

#if defined (TALK)
#define PRINTF(f...)		printf (f)
#else
#define PRINTF(f...)		do {} while (0)
#endif

#if TALK > 2
#define PRINTF3(f...)		printf (f)
#else
#define PRINTF3(f...)		do {} while (0)
#endif

#if TALK > 0
# define DBG(l,f...)		if (l <= TALK) printf (f);
#else
# define DBG(l,f...)		do {} while (0)
#endif

#if TALK > 0
# define PRINT_PKT(p,l)		dump ((void*) p, l, 0)
#else
# define PRINT_PKT(p,l)		do {} while (0)
#endif

#define SMC_REG(b,r)	(r)		

#define SMC_TCR		SMC_REG(0,0x0) /* Transmit Control Register */
#define SMC_EPH		SMC_REG(0,0x2) /*  */
#define SMC_RCR		SMC_REG(0,0x4) /*  */
#define SMC_COUNTER	SMC_REG(0,0x6) /*  */
#define SMC_MIR		SMC_REG(0,0x8) /*  */
#define SMC_RPCR	SMC_REG(0,0xa) /*  */
#define SMC_BANK	(0xe)

#define SMC_CR		SMC_REG(1,0x0) /*  */
#define SMC_BAR		SMC_REG(1,0x2) /*  */
#define SMC_IA0_1	SMC_REG(1,0x4) /*  */
#define SMC_IA2_3	SMC_REG(1,0x6) /*  */
#define SMC_IA4_5	SMC_REG(1,0x8) /*  */
#define SMC_GPR		SMC_REG(1,0xa) /*  */
#define SMC_CTR		SMC_REG(1,0xc) /*  */

#define SMC_MMUCR	SMC_REG(2,0x0) /*  */
#define SMC_PNR		SMC_REG(2,0x2) /*  */
//#define SMC_ARR	SMC_REG(2,0x3) /* Allocation Result Register */
#define SMC_FIFO	SMC_REG(2,0x4) /*  */
#define SMC_POINTER	SMC_REG(2,0x6) /*  */
#define SMC_DATAL	SMC_REG(2,0x8) /*  */
#define SMC_DATAH	SMC_REG(2,0xa) /*  */
#define SMC_INTERRUPT	SMC_REG(2,0xc) /*  */

#define SMC_MT0_1	SMC_REG(3,0x0) /*  */
#define SMC_MT2_3	SMC_REG(3,0x2) /*  */
#define SMC_MT4_5	SMC_REG(3,0x4) /*  */
#define SMC_MT6_7	SMC_REG(3,0x6) /*  */
#define SMC_MGMT	SMC_REG(3,0x8) /*  */
#define SMC_REV		SMC_REG(3,0xa) /*  */
#define SMC_ERCV	SMC_REG(3,0xc) /*  */

#define SMC_TCR_SWFDUP		(1<<15) /* Force full-duplex mode */
#define SMC_TCR_EPH_LOOP	(1<<13) /* Enable internal loopback */
#define SMC_TCR_STP_SQET	(1<<12)
#define SMC_TCR_FDUPLX		(1<<11)	/* Receive frames from our MAC */
#define SMC_TCR_MON_CSN		(1<<10)	/* Monitor carrier sense */
#define SMC_TCR_NOCRC		(1<<8)	/* Disables CRC frame check sequence */
#define SMC_TCR_PAD_EN		(1<<7)	/* Pad to 64 bytes minimum on tx */
#define SMC_TCR_FORCOL		(1<<2)	/* Force collision */
#define SMC_TCR_LOOP		(1<<1)	/* Controls LBK pin */
#define SMC_TCR_TXENA		(1<<0)	/* Transmit enable */

#define SMC_EPH_TX_UNRN		(1<<15)	/* Transmit underrun on last tx */
#define SMC_EPH_LINK_OK		(1<<14)	/* Link is good */
#define SMC_EPH_CTR_ROL		(1<<12)	/* Counter rollover */
#define SMC_EPH_EXC_DEF		(1<<11)	/* Excessive deferrals */
#define SMC_EPH_LOST_CARR	(1<<10) /* Lost Carrier Sense */
#define SMC_EPH_LATCOL		(1<<9)	/* Late collision on last transmit */
#define SMC_EPH_TX_DEFR		(1<<7)	/* Transmit deferred */
#define SMC_EPH_LTX_BRD		(1<<6)	/* Last transmit was broadcast */
#define SMC_EPH_SQET		(1<<5)	/* Signal Quality Error Test */
#define SMC_EPH_16COL		(1<<4)	/* 16 collisions detected last tx */
#define SMC_EPH_LTX_MULT	(1<<3)	/* Last transmit was multicast */
#define SMC_EPH_MUL_COL		(1<<2)	/* Multuple collisions on last tx */
#define SMC_EPH_SNGL_COL	(1<<1)	/* Single collision on last transmit */
#define SMC_EPH_TX_SUC		(1<<0)	/* Last transmit was successful */

#define SMC_RCR_SOFTRST		(1<<15)	/* Software activated reset */
#define SMC_RCR_FILT_CAR	(1<<14) /* Filter leading 12 bits of carrier */
#define SMC_RCR_ABORT_ENB	(1<<13) /* Enable receive abort on collision */
#define SMC_RCR_STRIP_CRC	(1<<9)	/* Strip CRC from received frames */
#define SMC_RCR_RXEN		(1<<8)	/* Receive enable */
#define SMC_RCR_ALMUL		(1<<2)	/* Accept all multicast frames */
#define SMC_RCR_PRMS		(1<<1)	/* Promiscuous receive mode */
#define SMC_RCR_RX_ABORT	(1<<0)	/* Excessively long frame aborted rx */

#define SMC_RPC_SPEED		(1<<13)	/* Speed select */
#define SMC_RPC_DPLX		(1<<12)	/* Duplex select */
#define SMC_RPC_ANEG		(1<<11)	/* Auto negotiation mode select */
#define SMC_RPC_LSA_SHIFT	5
#define SMC_RPC_LSB_SHIFT	2
#define SMC_RPC_LS_LINK		(0x00)
#define SMC_RPC_LS_10MB		(0x02)
#define SMC_RPC_LS_DUPLEX	(0x03)
#define SMC_RPC_LS_ACT		(0x04)
#define SMC_RPC_LS_100MB	(0x05)
#define SMC_RPC_LS_TX		(0x06)
#define SMC_RPC_LS_RX		(0x07)

#define SMC_CR_EPH_POWER_EN	(1<<15) /* Not EPH low-power mode */
#define SMC_CR_NO_WAIT		(1<<12)
#define SMC_CR_GPCNTRL		(1<<10)	/* GPIO control */
#define SMC_CR_EXT_PHY		(1<<9)	/* External PHY enable */

#define SMC_CTR_RCV_BAD		(1<<14) /* Receive packets with bad CRC */
#define SMC_CTR_AUTO_RELEASE	(1<<11)	/* TX buffers automatically released */
#define SMC_CTR_LE_ENABLE	(1<<7)	/* Link error detection enable */
#define SMC_CTR_CR_ENABLE	(1<<6)	/* Counter rollover enable */
#define SMC_CTR_TE_ENABLE	(1<<5)	/* Transmit error enable */
#define SMC_CTR_EEPROM_SELECT	(1<<2)
#define SMC_CTR_RELOAD		(1<<1)	/* Initiate reload from eeprom */
#define SMC_CTR_STORE		(1<<0)	/* Initiate store to eeprom */

#define SMC_MMUCR_NOP		(0<<5)
#define SMC_MMUCR_ALLOC		(1<<5)	/* Allocate for TX */
#define SMC_MMUCR_RESET		(2<<5)	/* Reset MMU */
#define SMC_MMUCR_REMOVE	(3<<5)	/* Remove top-most RX packet */
#define SMC_MMUCR_REMOVERELEASE	(4<<5)	/* Remove & release top-most RX */
#define SMC_MMUCR_RELEASE	(5<<5)	/* Release specific packet */
#define SMC_MMUCR_ENQUEUE	(6<<5)	/* Queue packet for TX */
#define SMC_MMUCR_RESETTX	(7<<5)	/* Reset TX FIFO */
#define SMC_MMUCR_BUSY		(1<<0)	/* MMU is busy */

#define SMC_ARR_FAILED		(1<<7)	/* Allocation failed */

#define SMC_FIFO_TEMPTY		(1<<7)	/* Transmit FIFO empty */
#define SMC_FIFO_REMPTY		(1<<15)	/* Receive FIFO empty */

#define SMC_PTR_RCV		(1<<15)
#define SMC_PTR_AUTOINC 	(1<<14)
#define SMC_PTR_READ		(1<<13)
#define SMC_PTR_ETEN		(1<<12)
#define SMC_PTR_NOT_EMPTY	(1<<11)

#define SMC_IM_MDINT		(1<<7)	/* PHY MI register 18 interrupt */
#define SMC_IM_ERCV_INT		(1<<6)	/* Early receive interrupt */
#define SMC_IM_EPH_INT		(1<<5)	/* Ethernet Protocol Handler int */
#define SMC_IM_RX_OVRN_INT	(1<<4)	/* Receiver overrun interrupt */
#define SMC_IM_ALLOC_INT	(1<<3)	/* Allocation interrupt */
#define SMC_IM_TX_EMPTY_INT	(1<<2)	/* TX FIFO empty interrupt  */
#define SMC_IM_TX_INT		(1<<1)	/* TX interrupt */
#define SMC_IM_RCV_INT		(1<<0)	/* RX interrupt */

#define SMC_MII_MSK_CRS100	(1<<14)	/* Disable CRS100 detection */
#define SMC_MII_MDOE		(1<<3)	/* MII output enable */
#define SMC_MII_MCLK		(1<<2)	/* MII clock */
#define SMC_MII_MDI		(1<<1)	/* MII input */
#define SMC_MII_MDO		(1<<0)  /* MII output */

#define SMC_ERCV_RCV_DISCRD	(1<<7)
#define SMC_ERCV_THRESHOLD	(0x001F)

#define PRINT_REG ({\
  printf ("regs 0 %04x 2 %04x 4 %04x 6 %04x\n",\
	  read_reg (0), read_reg (2),\
	  read_reg (4), read_reg (6));\
  printf ("     8 %04x a %04x c %04x e %04x\n",\
	  read_reg (8), read_reg (10),\
	  read_reg (12), read_reg (14)); })



#define write_reg(r,v) SMC_outw (SMC_IOBASE, (r), (v))
#define read_reg(r)    SMC_inw  (SMC_IOBASE, (r))

#if defined (CONFIG_ETHERNET)
extern char host_mac_address[];
#else
static char host_mac_address[6];
#endif

static inline void select_bank (int bank)
{
  write_reg (SMC_BANK, bank & 0xf);
}

static inline int current_bank (void)
{
  return read_reg (SMC_BANK) & 0xf;
}

static inline void wait_mmu (void)
{
  while (read_reg (SMC_MMUCR) & SMC_MMUCR_BUSY)
    ;
}

static void smc91x_reset (void)
{
  u16 v;

  DBG (1, "%s\n", __FUNCTION__);

  select_bank (2);
  write_reg (SMC_INTERRUPT, 0);	/* Mask all interrupts  */

  /* Perform a soft reset.  Nico thinks this is unnecessary and probably is. */
  select_bank (0);
  write_reg (SMC_RCR, SMC_RCR_SOFTRST);

  /* The reason for this delay isn't well documented.  The kernel
     driver waits here for 1us by RMKs hand. */
  usleep (1);

  /* Disable reset as well as both the receiver and the transmitter */
  select_bank (0);
  write_reg (SMC_RCR, 0);
  write_reg (SMC_TCR, 0);

  /* Configure */
  select_bank (1);
  v = SMC_CR_EPH_POWER_EN;
  v |= SMC_CR_NO_WAIT;	/* Optional */
  write_reg (SMC_CR, v);

  /* Control  */
  select_bank (1);
  v = read_reg (SMC_CTR); // | SMC_CTR_LE_ENABLE;
  v |= SMC_CTR_AUTO_RELEASE;	/* Helps limit interrupts...er yeah */
  write_reg (SMC_CTR, v);

  /* Reset MMU */
  select_bank (2);
  write_reg (SMC_MMUCR, SMC_MMUCR_RESET);
  wait_mmu ();
}

void smc91x_init (void)
{
  u16 v;

  DBG (1, "%s\n", __FUNCTION__);

#if defined (CPLD_CONTROL_WLPEN)
 {
   unsigned long l;
   CPLD_CONTROL |= CPLD_CONTROL_WLPEN; /* Disable the SMC91x chip */
   usleep (1);	/* Allow at least 100ns for reset pin to be recognized */
   CPLD_CONTROL &= ~CPLD_CONTROL_WLPEN; /* Enable the SMC91x chip */
   l = timer_read ();
   while (timer_delta (l, timer_read ()) < 50) /* PHY requires 50ns */
     ;
 }
#endif

  v = read_reg (SMC_BANK);
  if ((v & 0xff00) != 0x3300) {
    printf ("chip not detected\n");
    /* Chip not detected */
    return;
  }
  select_bank (0);

  /* Get revision */
  select_bank (3);
  v = read_reg (SMC_REV);
  printf ("smc91x chip 0x%x rev 0x%x\n", (v >> 4) & 0xf, v & 0xf);

  /* Get MAC address */
  select_bank (1);
  v = read_reg (SMC_IA0_1);
  host_mac_address[0] = v & 0xff;
  host_mac_address[1] = (v >> 8) & 0xff;
  v = read_reg (SMC_IA2_3);
  host_mac_address[2] = v & 0xff;
  host_mac_address[3] = (v >> 8) & 0xff;
  v = read_reg (SMC_IA4_5);
  host_mac_address[4] = v & 0xff;
  host_mac_address[5] = (v >> 8) & 0xff;

  smc91x_reset ();

  select_bank (0);
  write_reg (SMC_TCR, SMC_TCR_TXENA); /* Enable transmitter */
}

#if !defined (CONFIG_SMALL)
static void smc91x_report (void)
{
  printf ("  smc91x:" //   phy_addr %d  phy_id 0x%lx"
	  " mac_addr %02x:%02x:%02x:%02x:%02x:%02x\n",
//	  phy_address, phy_id,
	  host_mac_address[0], host_mac_address[1],
	  host_mac_address[2], host_mac_address[3],
	  host_mac_address[4], host_mac_address[5]);
}
#endif

static __driver_4 struct driver_d smc91x_driver = {
  .name = "eth-smc91x",
  .description = "SMSC smc91x Ethernet driver",
  .flags = DRIVER_NET,
//  .open = smc91x_open,
//  .close = close_helper,
//  .read = smc91x_read,
//  .write = smc91x_write,
};

static __service_6 struct service_d smc91x_service = {
  .init = smc91x_init,
#if !defined (CONFIG_SMALL)
  .report = smc91x_report,
#endif
};
