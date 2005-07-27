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

#define SMC_CONFIG	SMC_REG(1,0x0) /*  */
#define SMC_BASE	SMC_REG(1,0x2) /*  */
#define SMC_IA0_1	SMC_REG(1,0x4) /*  */
#define SMC_IA2_3	SMC_REG(1,0x6) /*  */
#define SMC_IA4_5	SMC_REG(1,0x8) /*  */
#define SMC_GENERAL	SMC_REG(1,0xa) /*  */
#define SMC_CONTROL	SMC_REG(1,0xc) /*  */

#define SMC_MMU		SMC_REG(2,0x0) /*  */
#define SMC_PNR		SMC_REG(2,0x2) /*  */
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
#define SMC_REVISION	SMC_REG(3,0xa) /*  */
#define SMC_ERCV	SMC_REG(3,0xc) /*  */

#define SMC_TCR_SWFDUP	(1<<15) /* Force full-duplex mode */
#define SMC_TCR_EPH_LOOP (1<<13) /* Enable internal loopback */
#define SMC_TCR_STP_SQET (1<<12)
#define SMC_TCR_FDUPLX  (1<<11)	/* Receive frames from our MAC */
#define SMC_TCR_MON_CSN	(1<<10)	/* Monitor carrier sense */
#define SMC_TCR_NOCRC	(1<<8)	/* Disables frame check sequence, CRC */
#define SMC_TCR_PAD_EN	(1<<7)	/* Pad to 64 bytes minimum on transmit */
#define SMC_TCR_FORCOL	(1<<2)	/* Force collision */
#define SMC_TCR_LOOP	(1<<1)	/* Controls LBK pin */
#define SMC_TCR_TXENA	(1<<0)	/* Transmit enable */

#define SMC_EPH_TX_UNRN	(1<<15)	/* Transmit underrun on last transmit */
#define SMC_EPH_LINK_OK (1<<14)	/* Link is good */
#define SMC_EPH_CTR_ROL	(1<<12)	/* Counter rollover */
#define SMC_EPH_EXC_DEF	(1<<11)	/* Excessive deferrals */
#define SMC_EPH_LOST_CARR (1<<10) /* Lost Carrier Sense */
#define SMC_EPH_LATCOL	(1<<9)	/* Late collision on last transmit */
#define SMC_EPH_TX_DEFR	(1<<7)	/* Transmit deferred */
#define SMC_EPH_LTX_BRD	(1<<6)	/* Last transmit was broadcast */
#define SMC_EPH_SQET	(1<<5)	/* Signal Quality Error Test */
#define SMC_EPH_16COL	(1<<4)	/* 16 collisions detected last transmit  */
#define SMC_EPH_LTX_MULT (1<<3)	/* Last transmit was multicast */
#define SMC_EPH_MUL_COL (1<<2)	/* Multuple collisions on last transmit */
#define SMC_EPH_SNGL_COL (1<<1)	/* Single collision on last transmit */
#define SMC_EPH_TX_SUC	(1<<0)	/* Last transmit was successful */

#define SMC_RCR_SOFTRST	(1<<15)	/* Software activated reset */
#define SMC_RCR_FILT_CAR (1<<14) /* Filter leading 12 bits of carrier */
#define SMC_RCR_ABORT_ENB (1<<13) /* Enable receive abort on collision */
#define SMC_RCR_STRIP_CRC (1<<9) /* Strip CRC from received frames */
#define SMC_RCR_RXEN	(1<<8)	/* Receive enable */
#define SMC_RCR_ALMUL	(1<<2)	/* Accept all multicast frames */
#define SMC_RCR_PRMS	(1<<1)	/* Promiscuous mode, receive all frames */
#define SMC_RCR_RX_ABORT (1<<0)	/* Excessively long frame aborted receipt */

#define SMC_RPC_SPEED	(1<<13)	/* Speed select */
#define SMC_RPC_DPLX	(1<<12)	/* Duplex select */
#define SMC_RPC_ANEG	(1<<11)	/* Auto negotiation mode select */
#define SMC_RPC_LSA_SHIFT 5
#define SMC_RPC_LSB_SHIFT 2
#define SMC_RPC_LS_LINK	(0x00)
#define SMC_RPC_LS_10MB	(0x02)
#define SMC_RPC_LS_DUPLEX (0x03)
#define SMC_RPC_LS_ACT	(0x04)
#define SMC_RPC_LS_100MB (0x05)
#define SMC_RPC_LS_TX	(0x06)
#define SMC_RPC_LS_RX	(0x07)

#define SMC_CONFIG_EPH_POWER_EN (1<<15) /* Not EPH low-power mode */
#define SMC_CONFIG_NO_WAIT	(1<<12)
#define SMC_CONFIG_GPCNTRL	(1<<10)	/* GPIO control */
#define SMC_CONFIG_EXT_PHY	(1<<9)	/* External PHY enable */

#define CTL_RCV_BAD	(1<<14) // When 1 bad CRC packets are received
#define CTL_AUTO_RELEASE (1<<11) // When 1 tx pages are released automatically
#define CTL_LE_ENABLE	(1<<7) // When 1 enables Link Error interrupt
#define CTL_CR_ENABLE	(1<<6) // When 1 enables Counter Rollover interrupt
#define CTL_TE_ENABLE	(1<<5) // When 1 enables Transmit Error interrupt
#define CTL_EEPROM_SELECT (1<<2) // Controls EEPROM reload & store
#define CTL_RELOAD	(1<<1) // When set reads EEPROM into registers
#define CTL_STORE	(1<<0) // When set stores registers into EEPROM

#define AR_FAILED	0x80	// Alocation Failed

#define TXFIFO_TEMPTY	0x80	// TX FIFO Empty

#define RXFIFO_REMPTY	0x80	// RX FIFO Empty

#define PTR_RCV		(1<<15) // 1=Receive area, 0=Transmit area
#define PTR_AUTOINC 	(1<<14) // Auto increment the pointer on each access
#define PTR_READ	(1<<13) // When 1 the operation is a read

#define IM_MDINT	0x80 // PHY MI Register 18 Interrupt
#define IM_ERCV_INT	0x40 // Early Receive Interrupt
#define IM_EPH_INT	0x20 // Set by Ethernet Protocol Handler section
#define IM_RX_OVRN_INT	0x10 // Set by Receiver Overruns
#define IM_ALLOC_INT	0x08 // Set when allocation request is completed
#define IM_TX_EMPTY_INT	0x04 // Set if the TX FIFO goes empty
#define IM_TX_INT	0x02 // Transmit Interrupt
#define IM_RCV_INT	0x01 // Receive Interrupt

#define MII_MSK_CRS100	(1<<14) // Disables CRS100 detection during tx half dup
#define MII_MDOE	(1<<3) // MII Output Enable
#define MII_MCLK	(1<<2) // MII Clock, pin MDCLK
#define MII_MDI		(1<<1) // MII Input, pin MDI
#define MII_MDO		(1<<0) // MII Output, pin MDO

#define ERCV_RCV_DISCRD	0x0080 // When 1 discards a packet being received
#define ERCV_THRESHOLD	0x001F // ERCV Threshold Mask
