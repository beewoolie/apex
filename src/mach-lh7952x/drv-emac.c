/* drv-emac.c
     $Id$

   written by Marc Singer
   16 Nov 2004

   Copyright (C) 2004 Marc Singer

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

   Support for initialization of the embedded lh79524 Ethernet MAC and
   Altima AC101L PHY.


   MAC EEPROM
   ----------

   Let me propose a format for the eeprom data.

   Byte     0      1      2    2+n-1   2+n
        +------+------+------+------+------+
        | 0x94 |  OP  |    DATA[n]  |  OP  | ...
        +------+------+------+------+------+

   The first byte is 0x94 representing 79524, 9 for the series, and 4
   for the model.  Following this is a list of fields, each starting
   with a byte describing the field and a sequence of data bytes
   determined by the type.

     OP     Length (n)    Description
    ----    ----------    -----------

    0x01       0x6        Primary MAC address in network byte order
    0x08       0x1        Speed negotiation:  0, auto; 1, 10Mbps; 2, 100Mpbs
    0x09       0x1        Duplex negotiation: 0, auto; 1, half;   2, full
    0xff                  End of fields

   I recommend that we standardize on emitting the OPs in ascending
   order.  It shouldn't matter, but it does make it easy for the parser
   code to detect and read the MAC address.

   Only the MAC address should be required.  All other parameters
   default to AUTO when there is no OP.  This means that a minimal
   configuration for a default MAC address of 01:23:45:67:89:ab would
   look like this.

      +------+------+------+------+------+------+------+------+------+
      | 0x94 | 0x01 | 0x01 | 0x23 | 0x45 | 0x67 | 0x89 | 0xab | 0xff |
      +------+------+------+------+------+------+------+------+------+


   Setup Sequence
   --------------

   0) CPLD clear PHY power-down
   1) IOCON muxctl1, muxctl23, muxctl24 (to 1's)
   2) RCPC ahbclkctrl
   3) EMAC specaddr1_bottom, specaddr1_top
   4) EMAC rxqptr, rxstatus
   5) EMAC txqptr, txstatus
   6) EMAC netconfig
   7) EMAC netctrl
   8) Read EMAC irqstatus for LINK_INTR


  PHY Mamagement
  --------------

  It is possible to poll for the PHY management operation to complete.
  There is an interrupt bit as well as a status bit.

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

#define DO_RESET_PHY

#define PHY_CONTROL	0
#define PHY_STATUS	1
#define PHY_ID1		2
#define PHY_ID2		3

#define PHY_CONTROL_RESET		(1<<15)
#define PHY_CONTROL_LOOPBACK		(1<<14)
#define PHY_CONTROL_POWERDOWN		(1<<11)
#define PHY_CONTROL_ANEN_ENABLE		(1<<12)
#define PHY_CONTROL_RESTART_ANEN	(1<<9)
#define PHY_STATUS_ANEN_COMPLETE	(1<<5)
#define PHY_STATUS_LINK			(1<<2)
#define PHY_STATUS_100FULL		(1<<14)
#define PHY_STATUS_100HALF		(1<<13)
#define PHY_STATUS_10FULL		(1<<12)
#define PHY_STATUS_10HALF		(1<<11)

static int phy_address;

/* There are two sets of buffers, one for receive and one for transmit.
   The first half are rx and the second half are tx. */

#define C_BUFFER	2	/* Number of buffers for each rx/tx */
#define CB_BUFFER	1536
static long __attribute__((section("ethernet.bss"))) 
     rgl_rx_descriptor[C_BUFFER*2];
static long __attribute__((section("ethernet.bss"))) 
     rgl_tx_descriptor[C_BUFFER*2];
static char __attribute__((section("ethernet.bss")))
     rgbBuffer[CB_BUFFER*C_BUFFER];	/* Memory for buffers */

static void msleep (int ms)
{
  unsigned long time = timer_read ();
	
  do {
  } while (timer_delta (time, timer_read ()) < ms);
}

static void emac_setup (void)
{
  int i;
  for (i = 0; i < C_BUFFER; ++i) {
    /* RX */
    rgl_rx_descriptor[i*2] = ((unsigned long)(rgbBuffer + i*CB_BUFFER) & ~3)
      | ((i == C_BUFFER - 1) ? (1<<1) : 0);
    rgl_rx_descriptor[i*2 + 1] = 0;
    /* TX */
    rgl_tx_descriptor[i*2]     = 0;
    rgl_tx_descriptor[i*2 + 1] = (1<<31)|(1<<15)
      | ((i == C_BUFFER - 1) ? (1<<30) : 0);
  }
  PRINTF ("emac: setup rgl_rx_descriptor %p\r\n", rgl_rx_descriptor);
  __REG (EMAC_PHYS + EMAC_RXBQP) = (unsigned long) rgl_rx_descriptor;
  __REG (EMAC_PHYS + EMAC_RXSTATUS) &= 
    ~(  EMAC_RXSTATUS_RXCOVERRUN | EMAC_RXSTATUS_FRMREC 
      | EMAC_RXSTATUS_BUFNOTAVAIL);
  PRINTF ("emac: setup rgl_tx_descriptor %p\r\n", rgl_tx_descriptor);
  __REG (EMAC_PHYS + EMAC_TXBQP) = (unsigned long) rgl_tx_descriptor;
  __REG (EMAC_PHYS + EMAC_TXSTATUS) &=
    ~(EMAC_TXSTATUS_TXUNDER | EMAC_TXSTATUS_TXCOMPLETE | EMAC_TXSTATUS_BUFEX);
	 
  __REG (EMAC_PHYS + EMAC_NETCONFIG)
    = EMAC_NETCONFIG_DIV32 | EMAC_NETCONFIG_FULLDUPLEX
    | EMAC_NETCONFIG_100MB | EMAC_NETCONFIG_CPYFRM;
//  __REG (EMAC_PHYS + EMAC_NETCONFIG) |= EMAC_NETCONFIG_RECBYTE;
  __REG (EMAC_PHYS + EMAC_NETCTL) 
    = EMAC_NETCTL_RXEN | EMAC_NETCTL_TXEN | EMAC_NETCTL_CLRSTAT;
}

static int emac_phy_read (int phy_address, int phy_register)
{
  unsigned short result;

  PRINTF3 ("emac_phy_read %d %d\r\n", phy_address, phy_register);

  __REG (EMAC_PHYS + EMAC_NETCTL) |= EMAC_NETCTL_MANGEEN;
  __REG (EMAC_PHYS + EMAC_PHYMAINT)
    = (1<<30)|(2<<28)
    |((phy_address  & 0x1f)<<23)
    |((phy_register & 0x1f)<<18)
    |(2<<16);

  usleep (2000*1000*1000/HCLK); /* wait 2000 HCLK cycles  */
  while ((__REG (EMAC_PHYS + EMAC_NETSTATUS) & (1<<2)) == 0)
    ;

  result = __REG (EMAC_PHYS + EMAC_PHYMAINT) & 0xffff;

  __REG (EMAC_PHYS + EMAC_NETCTL) &= ~EMAC_NETCTL_MANGEEN;

  PRINTF3 ("emac_phy_read => 0x%x\r\n", result);

  return result;
}

static void emac_phy_write (int phy_address, int phy_register, int phy_data)
{
  __REG (EMAC_PHYS + EMAC_NETCTL) |= EMAC_NETCTL_MANGEEN;
  __REG (EMAC_PHYS + EMAC_PHYMAINT)
    = (1<<30)|(1<<28)
    |((phy_address & 0x1f)<<23)
    |((phy_register & 0x1f)<<18)
    |(2<<16)
    |(phy_data & 0xffff);

  usleep (2000*1000*1000/HCLK); /* wait 2000 HCLK cycles  */
  while ((__REG (EMAC_PHYS + EMAC_NETSTATUS) & (1<<2)) == 0)
    ;
  __REG (EMAC_PHYS + EMAC_NETCTL) &= ~EMAC_NETCTL_MANGEEN;
}

static void emac_phy_reset (int phy_address)
{
#if 1
  PRINTF ("emac: phy_reset\r\n");
#if 1
  emac_phy_write (phy_address, 0,
		  PHY_CONTROL_RESET
		  | emac_phy_read (phy_address, 0));
  while (emac_phy_read (phy_address, 0) & PHY_CONTROL_RESET)
    ;
#else
  printf ("emac: ctrl 0x%x\r\n", emac_phy_read (phy_address, 0));
  __REG8 (CPLD_CONTROL) &= ~CPLD_CONTROL_WRLAN_ENABLE;
  emac_phy_write (phy_address, 0,
		  PHY_CONTROL_POWERDOWN
		  | emac_phy_read (phy_address, 0));
  msleep (1000);
  printf ("emac: ctrl 0x%x\r\n", emac_phy_read (phy_address, 0));
  __REG8 (CPLD_CONTROL) |= CPLD_CONTROL_WRLAN_ENABLE;
  emac_phy_write (phy_address, 0,
		  emac_phy_read (phy_address, 0)
		  & ~PHY_CONTROL_POWERDOWN); 
  printf ("emac: ctrl 0x%x\r\n", emac_phy_read (phy_address, 0));
#endif
#endif

#if 0
  PRINTF ("emac: forcing power-up\r\n");
  emac_phy_write (phy_address, 22, 0); /* Force power-up */
#endif

#if 0
  printf ("emac: resetart anen\r\n");
  emac_phy_write (phy_address, 0,
		  PHY_CONTROL_RESTART_ANEN | PHY_CONTROL_ANEN_ENABLE
		  | emac_phy_read (phy_address, 0));
#endif
}


/* emac_phy_detect

   scans for a valid phy attached to the controller.

*/

static void emac_phy_detect (void)
{
				/* Scan PHYs, #1 first, #0 last */
  for (phy_address = 1; phy_address < 33; ++phy_address) {
    unsigned int id1;
    unsigned int id2;
    id1 = emac_phy_read (phy_address & 0x1f, PHY_ID1);
    id2 = emac_phy_read (phy_address & 0x1f, PHY_ID2);

    if (   id1 != 0x0000 && id1 != 0xffff && id1 != 0x8000
	&& id2 != 0x0000 && id2 != 0xffff && id2 != 0x8000) {
      PRINTF ("emac: phy_detect 0x%x  0x%x\r\n", 
	      phy_address & 0x1f, (id1 << 16) | id2);
      break;
    }
  }

#if defined (DO_RESET_PHY)
  emac_phy_reset (phy_address);
#endif
}


/* emac_read_mac

   pulls a MAC address from the MAC EEPROM if there is one.  The
   format of this EEPROM data is described in the comment above.

*/

static int emac_read_mac (char rgbResult[6])
{
  struct descriptor_d d;
  char rgb[8];
  int result = 0;

  if (parse_descriptor ("mac:0#8", &d)
      || open_descriptor (&d))
    return -1;			/* No driver */

  if (d.driver->read (&d, rgb, 8) == 8 
      && rgb[0] == 0x94
      && rgb[1] == 0x01)
    memcpy (rgbResult, rgb + 2, 6);
  else
    result = -1;

  close_descriptor (&d);

  return result;
}


void emac_init (void)
{
  PRINTF ("emac: init\r\n");
  
	/* Hardware setup */
  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL1),
		(3<<8)|(3<<6)|(3<<4),
		(1<<8)|(1<<6)|(1<<4));		
//  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_RESCTL1),
//		(3<<8)|(3<<6)|(3<<4),
//		(0<<8)|(0<<6)|(0<<4));
  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL23),
		(3<<14)|(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0),
		(1<<14)|(1<<12)|(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0));
//  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_RESCTL23),
//		(3<<14)|(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0),
//		(0<<14)|(0<<12)|(0<<10)|(0<<8)|(0<<6)|(0<<4)|(0<<2)|(0<<0));
  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL24),
		(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0),
		(1<<12)|(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0));
//  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_RESCTL24),
//		(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0),
//		(0<<12)|(0<<10)|(0<<8)|(0<<6)|(0<<4)|(0<<2)|(0<<0));

  PRINTF ("CPLD_CONTROL %x => %x\r\n", __REG8 (CPLD_CONTROL),
	  __REG8 (CPLD_CONTROL) | CPLD_CONTROL_WRLAN_ENABLE);
  __REG (RCPC_PHYS + RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS + RCPC_AHBCLKCTRL) &= ~(1<<2);
  __REG (RCPC_PHYS + RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;

  {
    unsigned char rgb[6];
    if (!emac_read_mac (rgb)) {
      __REG (EMAC_PHYS + EMAC_SPECAD1BOT) 
	= (rgb[2]<<24)|(rgb[3]<<16)|(rgb[4]<<8)|(rgb[5]<<0);
      __REG (EMAC_PHYS + EMAC_SPECAD1TOP) 
	= (rgb[0]<<8)|(rgb[1]<<0);
      printf ("emac: mac address\r\n"); 
      dump (rgb, 6, 0);
    }
  }

  emac_setup ();
  __REG8 (CPLD_CONTROL) |= CPLD_CONTROL_WRLAN_ENABLE;
  emac_phy_detect ();

  /* Disable advertisement except for 100Base-TX  */
#if 0
  {
    unsigned long l = emac_phy_read (phy_address, 4);
    emac_phy_write (phy_address, 4, l & ~((1<<8)|(1<<6)|(1<<5)));
  }
#endif

  if ((__REG (EMAC_PHYS + EMAC_INTSTATUS) & EMAC_INT_LINKCHG) == 0)
    PRINTF ("emac: no link detected\r\n");
  else
    PRINTF ("emac: link change detected\r\n");
}

static __service_6 struct service_d lh7952x_emac_service = {
  .init = emac_init,
};

#if defined (CONFIG_CMD_EMAC)

int cmd_emac (int argc, const char** argv)
{
  int result = 0;

  if (argc == 1) {
#if defined (TALK)
    {
      unsigned long l;
      l = emac_phy_read (phy_address, 0);
      PRINTF ("phy_control 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 1);
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
      printf ("\r\n");
      l = emac_phy_read (phy_address, 4);
      PRINTF ("phy_advertisement 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 5);
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
      PRINTF ("\r\n");
      l = emac_phy_read (phy_address, 6);
      PRINTF ("phy_autoneg_expansion 0x%lx", l);
      if (l & (1<<4))
	PRINTF (" parallel-fault");
      if (l & (1<<1))
	PRINTF (" page-received");
      if (l & (1<<0))
	PRINTF (" partner-ANEN-able");
      PRINTF ("\r\n");
      l = emac_phy_read (phy_address, 7);
      PRINTF ("phy_autoneg_nextpage 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 16);
      PRINTF ("phy_bt_interrupt_level 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 17);
      PRINTF ("phy_interrupt_control 0x%lx", l);
      if (l & (1<<4))
	PRINTF (" parallel_fault");
      if (l & (1<<2))
	PRINTF (" link_not_ok");
      if (l & (1<<0))
	PRINTF (" anen_complete");
      PRINTF ("\r\n");
      l = emac_phy_read (phy_address, 18);
      PRINTF ("phy_diagnostic 0x%lx", l);
      if (l & (1<<12))
	PRINTF (" force 100TX");
      if (l & (1<<13))
	PRINTF (" force 10BT");
      PRINTF ("\r\n");
      l = emac_phy_read (phy_address, 19);
      PRINTF ("phy_power_loopback 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 20);
      PRINTF ("phy_cable 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 21);
      PRINTF ("phy_rxerr 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 22);
      PRINTF ("phy_power_mgmt 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 23);
      PRINTF ("phy_op_mode 0x%lx\r\n", l);
    }
#endif
  }
  else {
    if (strcmp (argv[1], "loop") == 0) {
      emac_phy_reset (phy_address);
      printf ("emac: loopback and restart anen\r\n");
      emac_phy_write (phy_address, 0,
		      PHY_CONTROL_RESTART_ANEN
		      | PHY_CONTROL_ANEN_ENABLE 
		      | PHY_CONTROL_LOOPBACK
		      | emac_phy_read (phy_address, 0));
    }
    else if (strcmp (argv[1], "anen") == 0) {
      emac_phy_reset (phy_address);
      printf ("emac: restart anen\r\n");
      emac_phy_write (phy_address, 0,
		      PHY_CONTROL_RESTART_ANEN | PHY_CONTROL_ANEN_ENABLE
		      | emac_phy_read (phy_address, 0));
    }
    else if (strcmp (argv[1], "force") == 0) {
      emac_phy_reset (phy_address);
      PRINTF ("emac: forcing power-up and restarting anen\r\n");
      emac_phy_write (phy_address, 22, 0); /* Force power-up */
      emac_phy_write (phy_address, 0, /* restart anen */
		      PHY_CONTROL_RESTART_ANEN | PHY_CONTROL_ANEN_ENABLE
		      | emac_phy_read (phy_address, 0));
    }
    else if (strcmp (argv[1], "mac") == 0) {
      unsigned char rgb[9];
      if (argc != 3)
	return ERROR_PARAM;

      rgb[0] = 0x94;		/* Signature */
      rgb[1] = 0x01;		/* OP: MAC address */
      rgb[8] = 0xff;		/* OP: end */

      {
	int i;
	char* pb = (char*) argv[2];
	for (i = 2; i < 8; ++i) {
	  if (!*pb)
	    return ERROR_PARAM;
	  rgb[i] = simple_strtoul (pb, &pb, 16);
	  if (*pb)
	    ++pb;
	}
	if (*pb)
	  return ERROR_PARAM;
      }

    //    dump (rgb, 9, 0);

      {
	struct descriptor_d d;
	static const char sz[] = "mac:0#9";
	if (   !parse_descriptor (sz, &d)
	       && !open_descriptor (&d)) {
	  d.driver->erase (&d, 9);
	  d.driver->seek (&d, 0, SEEK_SET);
	  if (d.driver->write (&d, rgb, 9) != 9)
	    result = ERROR_FAILURE;
	  close_descriptor (&d);
	}
	else
	  result = ERROR_NODRIVER;
	if (result)
	  printf ("unable to write MAC address to <%s>\r\n", sz);
      }
    }
  }

  return result;
}

static __command struct command_d c_emac = {
  .command = "emac",
  .description = "manage ethernet MAC address",
  .func = cmd_emac,
};

#endif
