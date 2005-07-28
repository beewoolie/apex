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

   MII

     o Data is clocked into the PHY on the rising edge of MCLK.

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

#define ENTRY	PRINTF("%s\n", __FUNCTION__)

#define PHY_CONTROL	0
#define PHY_STATUS	1
#define PHY_ID1		2
#define PHY_ID2		3
#define PHY_ANEG_ADV	4
#define PHY_ANEG_CAP	5

#define PHY_CONTROL_RESET		(1<<15)
#define PHY_CONTROL_LOOPBACK		(1<<14)
#define PHY_CONTROL_POWERDOWN		(1<<11)
#define PHY_CONTROL_ANEN_ENABLE		(1<<12)
#define PHY_CONTROL_MII_DISABLE		(1<<10)
#define PHY_CONTROL_RESTART_ANEN	(1<<9)
#define PHY_STATUS_ANEN_COMPLETE	(1<<5)
#define PHY_STATUS_100FULL		(1<<14)
#define PHY_STATUS_100HALF		(1<<13)
#define PHY_STATUS_10FULL		(1<<12)
#define PHY_STATUS_10HALF		(1<<11)
#define PHY_STATUS_LINK			(1<<2)

#define PHY_CFG1_REG		16
#define PHY_CFG1_LNKDIS		(1<<15)	/* Link detect disable (override) */
#define PHY_CFG1_XMTDIS		(1<<14)	/* Transmitter disable */
#define PHY_CFG1_XMTPDN		(1<<13) /* Transmitter power-down */
#define PHY_CFG1_BYPSCR		(1<<10)	/* Scrambler bypass */
#define PHY_CFG1_UNSCDS		(1<<9)	/* Autoneg. disable with unscr. idle */
#define PHY_CFG1_EQLZR		(1<<8)	/* Receiver equalizer disable */
#define PHY_CFG1_CABLE		(1<<7)	/* STP select (150 Ohm) */
#define PHY_CFG1_RLVL0		(1<<6)	/* Receive level adjust (squelch) */
#define PHY_CFG1_TLVL_SHIFT	2	/* Transmit level shift */
#define PHY_CFG1_TLVL_MASK	0x3c	/*  and mask */
#define PHY_CFG1_TRF_SHIFT	0	/* Transmit rise/fall time shift */
#define PHY_CFG1_TRF_MASK	0x3	/*  and mask */

#define PHY_CFG2_REG		17
#define PHY_CFG2_APOLDIS	(1<<5) /* Auto MDI/MDIX disable */
#define PHY_CFG2_JABDIS		(1<<4) /* Jabber disable */
#define PHY_CFG2_MREG		(1<<3) /* Multiple register access enable */
#define PHY_CFG2_INTMDIO	(1<<2) /* Interrupt signaled with MDIO */

#define PHY_INT			18	/* Interrupt/status register */
#define PHY_MASK		19	/* Interrupt mask register */
#define PHY_INT_INT		(1<<15)	/* Status changed */
#define PHY_INT_LNKFAIL		(1<<14)
#define PHY_INT_LOSSSYNC	(1<<13)
#define PHY_INT_CWRD		(1<<12)
#define PHY_INT_SSD		(1<<11)
#define PHY_INT_ESD		(1<<10)
#define PHY_INT_RPOL		(1<<9)
#define PHY_INT_JAB		(1<<8)
#define PHY_INT_SPDDET		(1<<7)
#define PHY_INT_DPLXDET		(1<<6)

// PHY Interrupt/Status Mask Register
// Uses the same bit definitions as PHY_INT_REG




static int phy_address;
static unsigned long phy_id;	/* ID read from PHY */

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
#define SMC_PTR		SMC_REG(2,0x6) /*  */
#define SMC_DATAL	SMC_REG(2,0x8) /*  */
#define SMC_DATAH	SMC_REG(2,0xa) /*  */
#define SMC_INTERRUPT	SMC_REG(2,0xc) /*  */
//#define SMC_IST		SMC_REG(2,0xc) /*  */
//#define SMC_ACK		SMC_REG(2,0xc) /*  */
//#define SMC_MSK		SMC_REG(2,0xd) /*  */

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

#define SMC_RPCR_SPEED		(1<<13)	/* Speed select */
#define SMC_RPCR_DPLX		(1<<12)	/* Duplex select */
#define SMC_RPCR_ANEG		(1<<11)	/* Auto negotiation mode select */
#define SMC_RPCR_LSA_SHIFT	5
#define SMC_RPCR_LSB_SHIFT	2
#define SMC_RPCR_MASK		(0x7)
#define SMC_RPCR_LS_LINK	(0x00)
#define SMC_RPCR_LS_10MB	(0x02)
#define SMC_RPCR_LS_DUPLEX	(0x03)
#define SMC_RPCR_LS_ACT		(0x04)
#define SMC_RPCR_LS_100MB	(0x05)
#define SMC_RPCR_LS_TX		(0x06)
#define SMC_RPCR_LS_RX		(0x07)

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
#define SMC_PTR_AUTO_INCR 	(1<<14)
#define SMC_PTR_READ		(1<<13)
#define SMC_PTR_ETEN		(1<<12)
#define SMC_PTR_NOT_EMPTY	(1<<11)

#define SMC_INT_MDINT		(1<<7)	/* PHY MI register 18 interrupt */
#define SMC_INT_ERCV_INT	(1<<6)	/* Early receive interrupt */
#define SMC_INT_EPH_INT		(1<<5)	/* Ethernet Protocol Handler int */
#define SMC_INT_RX_OVRN_INT	(1<<4)	/* Receiver overrun interrupt */
#define SMC_INT_ALLOC_INT	(1<<3)	/* Allocation interrupt */
#define SMC_INT_TX_EMPTY_INT	(1<<2)	/* TX FIFO empty interrupt  */
#define SMC_INT_TX_INT		(1<<1)	/* TX interrupt */
#define SMC_INT_RCV_INT		(1<<0)	/* RX interrupt */

#define SMC_MGMT_MSK_CRS100	(1<<14)	/* Disable CRS100 detection */
#define SMC_MGMT_MDOE		(1<<3)	/* MII output enable */
#define SMC_MGMT_MCLK		(1<<2)	/* MII clock */
#define SMC_MGMT_MDI		(1<<1)	/* MII input */
#define SMC_MGMT_MDO		(1<<0)  /* MII output */

#define SMC_ERCV_RCV_DISCRD	(1<<7)
#define SMC_ERCV_THRESHOLD	(0x001F)

#define SMC_PKTCONTROL_ODD	(1<<13)	/* Last byte of packet in control */
#define SMC_PKTCONTROL_CRC	(1<<12)	/* Generate CRC on transmit */

#define PRINT_REG ({\
  printf ("regs 0 %04x 2 %04x 4 %04x 6 %04x\n",\
	  read_reg (0), read_reg (2),\
	  read_reg (4), read_reg (6));\
  printf ("     8 %04x a %04x c %04x e %04x\n",\
	  read_reg (8), read_reg (10),\
	  read_reg (12), read_reg (14)); })

#define US_MII_DELAY	 (5)	/* Half-cycle time for MII clock */

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

static inline void clear_interrupt (int v)
{
  write_reg (SMC_INTERRUPT, (read_reg (SMC_INTERRUPT) & 0xff) | (v & 0xff));
}

static inline void wait_mmu (void)
{
  while (read_reg (SMC_MMUCR) & SMC_MMUCR_BUSY)
    ;
}

static void smc91x_mii_write (unsigned long value, int length)
{
  int c;
  unsigned long v = (read_reg (SMC_MGMT)
		     & ~(SMC_MGMT_MCLK | SMC_MGMT_MDO))
    | SMC_MGMT_MDOE;

//  ENTRY;
//  PRINTF (" writing 0x%lx for %d (%lx)\n", value, length, v);
  
  for (c = 0; c < length; ++c) {
//    PRINTF (".");
    v = v & ~SMC_MGMT_MDO;
    /* *** FIXME: depends on MD0 being the lowest bit */
    v |= (value >> (length - c - 1)) & 1;
//    printf ("  0x%lx \n",  v);
    write_reg (SMC_MGMT, v);
    usleep (US_MII_DELAY);
    write_reg (SMC_MGMT, v | SMC_MGMT_MCLK);
    usleep (US_MII_DELAY);
  }
}

static unsigned int smc91x_mii_read (int length)
{
  unsigned long v = (read_reg (SMC_MGMT)
		     & ~(SMC_MGMT_MDOE | SMC_MGMT_MCLK | SMC_MGMT_MDO));
  int c = 0;
  unsigned long value = 0;

//  ENTRY;

  write_reg (SMC_MGMT, v);
  for (c = 0; c < length; ++c) {
    /* *** FIXME: depends on MDI being the second lowest bit */
    value = (value << 1) | ((read_reg (SMC_MGMT) >> 1) & 1);
    write_reg (SMC_MGMT, v);
    usleep (US_MII_DELAY);
    write_reg (SMC_MGMT, v | SMC_MGMT_MCLK);
    usleep (US_MII_DELAY);
  }

  return value;
}

static void smc91x_mii_disable (void)
{
  write_reg (SMC_MGMT, read_reg (SMC_MGMT) 
	     & ~(SMC_MGMT_MCLK | SMC_MGMT_MDOE | SMC_MGMT_MDO));
}

static void smc91x_phy_write (int phy_address, int phy_register, int phy_data)
{
  ENTRY;

  select_bank (3);

  /* Idle the channel */
  smc91x_mii_write (0xffffffff, 32);

  /* Write the data */
  smc91x_mii_write ((1<<30) | (1<<28)
		    |((phy_address & 0x1f)<<23)
		    |((phy_register & 0x1f)<<18)
		    |(2<<16)
		    |(phy_data & 0xffff), 32);

  smc91x_mii_disable ();
}

static int smc91x_phy_read (int phy_address, int phy_register)
{
  unsigned long value;

//  ENTRY;

  select_bank (3);

  /* Idle the channel */
  smc91x_mii_write (0xffffffff, 32);

  /* Issue read request */
  smc91x_mii_write ((1<<12)|(2<<10)
		    | ((phy_address  & 0x1f)<<5)
		    | ((phy_register & 0x1f)<<0), 14);

  /* Read result include two high status bits */
  value = smc91x_mii_read (18);

  smc91x_mii_disable ();

  return value & 0xffff;
}

static void smc91x_phy_detect (void)
{
  ENTRY;

  for (phy_address = 1; phy_address < 33; ++phy_address) {
    unsigned int id1;
    unsigned int id2;

    id1 = smc91x_phy_read (phy_address & 0x1f, PHY_ID1);
    id2 = smc91x_phy_read (phy_address & 0x1f, PHY_ID2);

//    printf (" #%02d phy_id1=0x%x, phy_id2=0x%x\n", 
//	    phy_address & 0x1f, id1, id2);

    if (   id1 != 0x0000 && id1 != 0xffff && id1 != 0x8000
	&& id2 != 0x0000 && id2 != 0xffff && id2 != 0x8000) {
      phy_id = (id1 << 16) | id2;
      phy_address &= 0x1f;
      printf ("smc91x: phy_detect 0x%x  0x%lx\n", phy_address, phy_id);
      break;
    }
  }
}

static void smc91x_phy_configure (void)
{
  int v = smc91x_phy_read (phy_address, PHY_CONTROL);
  v &= ~PHY_CONTROL_MII_DISABLE;
  v |= PHY_CONTROL_ANEN_ENABLE;
  printf ("Writing PHY_CONTROL 0x%x\n", v);
  smc91x_phy_write (phy_address, PHY_CONTROL, v);
  v = smc91x_phy_read (phy_address, PHY_CONTROL);
  printf ("Read back PHY_CONTROL 0x%x\n", v);
}


#if 0
#if defined (USE_PHY_RESET)
static void smc91x_phy_reset (int phy_address)
{

  PRINTF ("emac: phy_reset\n");
  smc91x_phy_write (phy_address, 0,
		  PHY_CONTROL_RESET
		  | smc91x_phy_read (phy_address, 0));
  while (smc91x_phy_read (phy_address, 0) & PHY_CONTROL_RESET)
    ;
}
#else
# define smc91x_phy_reset(v) do { } while (0)
#endif

static void smc91x_phy_configure (int phy_address)
{
  PRINTF ("emac: phy_configure\n");

#if defined USE_DISABLE_AUTOMDI_MDIX
  if (phy_id == 0x00225521){
    unsigned long l = smc91x_phy_read (phy_address, 23);
    smc91x_phy_write (phy_address, 23,
		    (l & ~((1<<7)|(1<<6)))|(1<<7)); /* Force MDI. */
  }
#endif

#if defined USE_DISABLE_100MB
  {
    unsigned long l = smc91x_phy_read (phy_address, 0);
    smc91x_phy_write (phy_address, 0, l & ~(1<<13));
  }
#endif
}


#endif


static ssize_t smc91x_read (struct descriptor_d* d, void* pv, size_t cb)
{
  return 0; 
}

static int smc91x_write (struct descriptor_d* d, const void* pv, size_t cb)
{
  int pkt;

  select_bank (2);
  write_reg (SMC_MMUCR, SMC_MMUCR_ALLOC);

  while (!(read_reg (SMC_INTERRUPT) & SMC_INT_ALLOC_INT))
    ;
  
  clear_interrupt (SMC_INT_ALLOC_INT);

  pkt = read_reg (SMC_PNR) >> 8;
  if (pkt & SMC_ARR_FAILED) {
    printf ("unable to send.  No packet buffers available\n");
    return 0;
  }

  printf ("packet #%d %d bytes\n", pkt, cb);

  write_reg (SMC_PNR, pkt);
  write_reg (SMC_PTR, SMC_PTR_AUTO_INCR);

  /* write packet header */
  write_reg (SMC_DATAL, 0);	/* status */
  write_reg (SMC_DATAL, (cb + 6)&~1); /* packet length + control */
  /* write data */
  SMC_outsw (SMC_IOBASE, SMC_DATAL, pv, cb >> 1);
  /* write control and last byte if the length was odd */
  SMC_outw (SMC_IOBASE, SMC_DATAL, 
	    (cb & 1) ? ((unsigned char*) pv)[cb - 1] | SMC_PKTCONTROL_ODD : 0);

  write_reg (SMC_MMUCR, SMC_MMUCR_ENQUEUE);
  clear_interrupt (SMC_INT_TX_EMPTY_INT);

  printf ("waiting for transmit\n");
  while (!(read_reg (SMC_INTERRUPT) & SMC_INT_TX_EMPTY_INT))
    ;
  
  printf ("transmit complete\n");
  return cb;
}

static void smc91x_reset (void)
{
  u16 v;

  DBG (1, "%s\n", __FUNCTION__);

  select_bank (2);
  write_reg (SMC_INTERRUPT, 0);	/* Mask all interrupts  */
  clear_interrupt (0xff);	/* Acknowledge all interrupts */

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

static int smc91x_open (struct descriptor_d* d)
{
  return 0;			/* No problems */
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

  smc91x_phy_detect ();

  smc91x_reset ();

  smc91x_phy_configure ();

  select_bank (0);
  write_reg (SMC_TCR, SMC_TCR_TXENA); /* Enable transmitter */
  {
    int v = read_reg (SMC_RPCR);
    v &= ~(  (SMC_RPCR_MASK << SMC_RPCR_LSA_SHIFT) 
	   | (SMC_RPCR_MASK << SMC_RPCR_LSB_SHIFT));
    v |= SMC_RPCR_LS_ACT    << SMC_RPCR_LSA_SHIFT;
    v |= SMC_RPCR_LS_LINK   << SMC_RPCR_LSB_SHIFT;
    write_reg (SMC_RPCR, v);
  }
  
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
  .open = smc91x_open,
  .close = close_helper,
  .read = smc91x_read,
  .write = smc91x_write,
};

static __service_6 struct service_d smc91x_service = {
  .init = smc91x_init,
#if !defined (CONFIG_SMALL)
  .report = smc91x_report,
#endif
};


#if defined (CONFIG_CMD_ETH_SMC91X)

static int cmd_eth (int argc, const char** argv)
{
  int result = 0;

  if (argc == 1) {
    {
      unsigned i;
      struct regs {
	int reg;
	char label[5];
      };
      static struct regs rgRegs[] = {
	{  0, "ctrl" },
	{  1, "stat" },
	{  2, "id1" },
	{  3, "id2" },
	{  4, "anadv" },
	{  5, "anpar" },
	{  6, "anexp" },
	{  7, "anpag" },

#if 1
	{ 16, "cfg1" },
	{ 17, "cfg2" },
	{ 18, "int" },
	{ 19, "mask" },
#endif
#if 0
	/* Altima */
	{ 16, "btic" },
	{ 17, "intr" },
	{ 18, "diag" },
	{ 19, "loop" },
	{ 20, "cable" },
	{ 21, "rxer" },
	{ 22, "power" },
	{ 23, "oper" },
	{ 24, "rxcrc" },
	{ 28|(1<<8)|(0<<12), "mode" },
	{ 29|(1<<8)|(0<<12), "test" },
	{ 29|(1<<8)|(1<<12), "blnk" },
	{ 30|(1<<8)|(1<<12), "l0s1" },
	{ 31|(1<<8)|(1<<12), "l0s2" },
	{ 29|(1<<8)|(2<<12), "l1s1" },
	{ 30|(1<<8)|(2<<12), "l1s2" },
	{ 31|(1<<8)|(2<<12), "l2s1" },
	{ 29|(1<<8)|(3<<12), "l2s2" },
	{ 30|(1<<8)|(3<<12), "l3s1" },
	{ 31|(1<<8)|(3<<12), "l3s2" },
#endif
#if 0
	/* NatSemi */
	{ 16, "phsts" },
	{ 20, "fcscr" },
	{ 21, "recr" },
	{ 22, "pcsr" },
	{ 25, "phctl" },
	{ 26, "100sr" },
	{ 27, "cdtst" },
#endif
      };
#if 1
#define WIDE 5
#define ROWS (sizeof (rgRegs)/sizeof (rgRegs[0]) + WIDE - 1)/WIDE
#define COUNT ROWS*WIDE
      for (i = 0; i < COUNT; ++i) {
	int index = (i%WIDE)*ROWS + i/WIDE;
	if (index >= sizeof (rgRegs)/sizeof (rgRegs[0]))
	  continue;
	if (i && (i % WIDE) == 0)
	  printf ("\n");
	if (rgRegs[index].reg & (1<<8))
	  smc91x_phy_write (phy_address, 28, 
			  (smc91x_phy_read (phy_address, 28) & 0x0fff)
			  | (rgRegs[index].reg & 0xf000));
	printf ("%5s-%-2d %04x  ", 
		rgRegs[index].label, rgRegs[index].reg & 0xff, 
		smc91x_phy_read (phy_address, rgRegs[index].reg & 0xff));
      }	
#else
      for (i = 0; i < 2*(((sizeof (rgRegs)/sizeof (rgRegs[0])) + 0x7)&~7);
	   ++i) {
	unsigned index = (i & 7) | ((i >> 1) & ~7);
	if (index >= sizeof (rgRegs)/sizeof (rgRegs[0]))
	  continue;
	//	printf ("i %d (%x) index %d (%x)\n", i, i, index, index);
	if (i & (1<<3)) {
	  if (i && (index & 0x7) == 0)
	    printf ("\n");
	  if (rgRegs[index].reg & (1<<8))
	    smc91x_phy_write (phy_address, 28, 
			    (smc91x_phy_read (phy_address, 28) & 0x0fff)
			    | (rgRegs[index].reg & 0xf000));
	  printf ("%04x    ", smc91x_phy_read (phy_address, 
					     rgRegs[index].reg & 0xff));
	}
	else {
	  if (i && (index & 0x7) == 0)
	    printf ("\n\n");
	  printf ("%s-%-2d ", rgRegs[index].label, rgRegs[index].reg & 0xff);
	}
      }
#endif

      printf ("\n");
    }
    {
      unsigned long l;
//      int i;
      l = smc91x_phy_read (phy_address, 0);
      PRINTF ("phy_control 0x%lx\n", l);
      l = smc91x_phy_read (phy_address, 1);
      PRINTF ("phy_status 0x%lx", l);
      if (l&PHY_STATUS_ANEN_COMPLETE)
	PRINTF (" anen_complete");
      if (l&PHY_STATUS_LINK)
	PRINTF (" link");
      if (l&PHY_STATUS_100FULL)
	PRINTF (" cap100F");
      if (l&PHY_STATUS_100HALF)
	PRINTF (" cap100H");
      if (l&PHY_STATUS_10FULL)
	PRINTF (" cap10F");
      if (l&PHY_STATUS_10HALF)
	PRINTF (" cap10H");
      printf ("\n");
      //      l = smc91x_phy_read (phy_address, 4);
      //      PRINTF ("phy_advertisement 0x%lx\n", l);
      l = smc91x_phy_read (phy_address, 5);
      PRINTF ("phy_partner 0x%lx", l);
      if (l & (1<<9))
	PRINTF ("  100Base-T4");
      if (l & (1<<8))
	PRINTF ("  100Base-TX-full");
      if (l & (1<<7))
	PRINTF ("  100Base-TX");
      if (l & (1<<6))
	PRINTF ("  10Base-T-full");
      if (l & (1<<5))
	PRINTF ("  10Base-T");
      PRINTF ("\n");
      l = smc91x_phy_read (phy_address, 6);
      PRINTF ("phy_autoneg_expansion 0x%lx", l);
      if (l & (1<<4))
	PRINTF (" parallel-fault");
      if (l & (1<<1))
	PRINTF (" page-received");
      if (l & (1<<0))
	PRINTF (" partner-ANEN-able");
      PRINTF ("\n");
      //      l = smc91x_phy_read (phy_address, 7);
      //      PRINTF ("phy_autoneg_nextpage 0x%lx\n", l);
      //      l = smc91x_phy_read (phy_address, 16);
      //      PRINTF ("phy_bt_interrupt_level 0x%lx\n", l);
      l = smc91x_phy_read (phy_address, 17);
      PRINTF ("phy_interrupt_control 0x%lx", l);
      if (l & (1<<4))
	PRINTF (" parallel_fault");
      if (l & (1<<2))
	PRINTF (" link_not_ok");
      if (l & (1<<0))
	PRINTF (" anen_complete");
      PRINTF ("\n");
      l = smc91x_phy_read (phy_address, 18);
      PRINTF ("phy_diagnostic 0x%lx", l);
      if (l & (1<<12))
	PRINTF (" force 100TX");
      if (l & (1<<13))
	PRINTF (" force 10BT");
      PRINTF ("\n");
      //      l = smc91x_phy_read (phy_address, 19);
      //      PRINTF ("phy_power_loopback 0x%lx\n", l);
      //      l = smc91x_phy_read (phy_address, 20);
      //      PRINTF ("phy_cable 0x%lx\n", l);
      //      l = smc91x_phy_read (phy_address, 21);
      //      PRINTF ("phy_rxerr 0x%lx\n", l);
      //      l = smc91x_phy_read (phy_address, 22);
      //      PRINTF ("phy_power_mgmt 0x%lx\n", l);
      //      l = smc91x_phy_read (phy_address, 23);
      //      PRINTF ("phy_op_mode 0x%lx\n", l);
      //      l = smc91x_phy_read (phy_address, 24);
      //      PRINTF ("phy_last_crc 0x%lx\n", l);

    }
  }
  else {
    if (strcmp (argv[1], "en") == 0) {
      smc91x_phy_configure ();
    }
  }

  return result;
}

static __command struct command_d c_eth = {
  .command = "eth",
  .description = "smc91x diagnostics",
  .func = cmd_eth,
  COMMAND_HELP(
"emac [SUBCOMMAND [PARAMETER]]\n"
"  Commands for the Ethernet MAC and PHY devices.\n"
"  Without a SUBCOMMAND, it displays diagnostics about the EMAC\n"
"    and PHY devices.  This information is for debugging the hardware.\n"
//"  clear - reset the EMAC.\n"
//"  anen  - restart auto negotiation.\n"
//"  send  - send a test packet.\n"
//"  loop  - enable loopback mode.\n"
//"  force - force power-up and restart auto-negotiation.\n"
//"  mac   - set the MAC address to PARAMETER.\n"
//"    PARAMETER has the form XX:XX:XX:XX:XX:XX where each X is a\n"
//"    hexadecimal digit.  Be aware that MAC addresses must be unique for\n"
//"    proper operation of the network.  This command may be added to the\n"
//"    startup commands to set the MAC address at boot-time.\n"
//"  save  - saves the MAC address to the mac: EEPROM device.\n"
//"    A saved MAC address will be used to automatically configure the MAC\n"
//"    at startup.  For this feature to work, there must be a mac: driver.\n"
//"  e.g.  emac mac 01:23:45:67:89:ab         # Never use this MAC address\n"
//"        emac save\n"
  )
};

#endif
