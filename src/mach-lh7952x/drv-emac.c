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
   Altima AC101L PHY, if present.


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
   4) EMAC rxqptr, rxstatus, buffer initializatoin
   5) EMAC txqptr, txstatus, (buffer initialization not really necessary)
   6) EMAC netconfig
   7) EMAC netctrl
   8) Read EMAC irqstatus for LINK_INTR


  PHY Mamagement
  --------------

  It is possible to poll for the PHY management operation to complete.
  There is an interrupt bit as well as a status bit.

  Auto MDI/MDI-X
  --------------

  The altima PHY supports automatic detection of the MDI/MDI-X mode of
  the twisted-pair.  According to Broadcom, this feature requires
  appropriate magnetics.  As of the Rev J SDK board, LPD doesn't
  provide the proper circuit to automatically detect the TP mode.
  This, the feature is disabled.

  CONFIG_CMD_EMAC_LH79524
  -----------------------

  It is possible to disable the EMAC commands altogether.  If this is
  done, the MAC address cannot be set from the APEX command line nor
  can the MAC address be written to EEPROM.  However, the device will
  still be initialized and the MAC address read from EEPROM if it is
  already there.

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

#define USE_DISABLE_AUTOMDI_MDIX /* Disable auto-mdi/mdix detection */
#define USE_PHY_RESET		/* Reset the PHY after detection  */
//#define USE_DISABLE_100MB

//#define USE_DIAG		/* Enables diagnostic code */

//#define TALK 0

#if !defined (CONFIG_CMD_EMAC_LH79524)
# undef USE_DIAG
#endif

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
static unsigned long phy_id;	/* ID read from PHY */

#if defined (USE_DIAG)

/* There are two sets of buffers, one for receive and one for transmit.
   The first half are rx and the second half are tx. */

#define C_BUFFER	2	/* Number of buffers for each rx/tx */
#define CB_BUFFER	1536
  /* Leave the descriptors in normal BSS so they get zeroed */
static long // __attribute__((section(".ethernet.bss"))) 
     rgl_rx_descriptor[C_BUFFER*2];
static long // __attribute__((section(".ethernet.bss"))) 
     rgl_tx_descriptor[C_BUFFER*2];
static char __attribute__((section(".ethernet.bss")))
     rgbRxBuffer[CB_BUFFER*C_BUFFER];
static char __attribute__((section(".ethernet.bss")))
     rgbTxBuffer[CB_BUFFER*C_BUFFER];

struct ethernet_header {
  unsigned char dest[6];
  unsigned char src[6];
  unsigned short protocol;
};

static void msleep (int ms)
{
  unsigned long time = timer_read ();
	
  do {
  } while (timer_delta (time, timer_read ()) < ms);
}

#endif

#if defined (USE_DIAG)
static void emac_setup (void)
{
  int i;
  for (i = 0; i < C_BUFFER; ++i) {
    /* RX */
    rgl_rx_descriptor[i*2]     = ((unsigned long)(rgbRxBuffer + i*CB_BUFFER) 
				  & ~3)
      | ((i == C_BUFFER - 1) ? (1<<1) : 0);
    rgl_rx_descriptor[i*2 + 1] = 0;
    /* TX */
    //    rgl_tx_descriptor[i*2]     = (unsigned long)(rgbTxBuffer + i*CB_BUFFER);
    //    rgl_tx_descriptor[i*2 + 1] = (1<<31)|(1<<15)
    //      | ((i == C_BUFFER - 1) ? (1<<30) : 0);
  }
  PRINTF ("emac: setup rgl_rx_descriptor %p\n", rgl_rx_descriptor);
  EMAC_RXBQP = (unsigned long) rgl_rx_descriptor;
  EMAC_RXSTATUS &= 
    ~(  EMAC_RXSTATUS_RXCOVERRUN | EMAC_RXSTATUS_FRMREC 
      | EMAC_RXSTATUS_BUFNOTAVAIL);
  PRINTF ("emac: setup rgl_tx_descriptor %p\n", rgl_tx_descriptor);
  EMAC_TXBQP = (unsigned long) rgl_tx_descriptor;
  EMAC_TXSTATUS &=
    ~(EMAC_TXSTATUS_TXUNDER | EMAC_TXSTATUS_TXCOMPLETE | EMAC_TXSTATUS_BUFEX);
	 
  MASK_AND_SET (EMAC_NETCONFIG, 3<<10, EMAC_NETCONFIG_DIV32);
  EMAC_NETCONFIG |= 0
    //    | EMAC_NETCONFIG_FULLDUPLEX
    //    | EMAC_NETCONFIG_100MB
    | EMAC_NETCONFIG_CPYFRM
    ;
//  EMAC_NETCONFIG |= EMAC_NETCONFIG_RECBYTE;
  EMAC_NETCTL |= 0
    | EMAC_NETCTL_RXEN
    | EMAC_NETCTL_TXEN
    | EMAC_NETCTL_CLRSTAT
    ;
}
#else
#define emac_setup() do {} while (0)
#endif

static int emac_phy_read (int phy_address, int phy_register)
{
  unsigned short result;

  PRINTF3 ("emac_phy_read %d %d\n", phy_address, phy_register);

  EMAC_NETCTL |= EMAC_NETCTL_MANGEEN;
  EMAC_PHYMAINT
    = (1<<30)|(2<<28)
    | ((phy_address  & 0x1f)<<23)
    | ((phy_register & 0x1f)<<18)
    | (2<<16);

  while ((EMAC_NETSTATUS & (1<<2)) == 0)
    ;

  result = EMAC_PHYMAINT & 0xffff;

  EMAC_NETCTL &= ~EMAC_NETCTL_MANGEEN;

  PRINTF3 ("emac_phy_read => 0x%x\n", result);

  return result;
}

static void emac_phy_write (int phy_address, int phy_register, int phy_data)
{
  EMAC_NETCTL |= EMAC_NETCTL_MANGEEN;
  EMAC_PHYMAINT
    = (1<<30)|(1<<28)
    |((phy_address & 0x1f)<<23)
    |((phy_register & 0x1f)<<18)
    |(2<<16)
    |(phy_data & 0xffff);

  while ((EMAC_NETSTATUS & (1<<2)) == 0)
    ;

  EMAC_NETCTL &= ~EMAC_NETCTL_MANGEEN;
}

#if defined (USE_PHY_RESET)
static void emac_phy_reset (int phy_address)
{

  PRINTF ("emac: phy_reset\n");
  emac_phy_write (phy_address, 0,
		  PHY_CONTROL_RESET
		  | emac_phy_read (phy_address, 0));
  while (emac_phy_read (phy_address, 0) & PHY_CONTROL_RESET)
    ;
}
#else
# define emac_phy_reset(v) do { } while (0)
#endif

static void emac_phy_configure (int phy_address)
{
  PRINTF ("emac: phy_configure\n");

#if defined USE_DISABLE_AUTOMDI_MDIX
  if (phy_id == 0x00225521){
    unsigned long l = emac_phy_read (phy_address, 23);
    emac_phy_write (phy_address, 23,
		    (l & ~((1<<7)|(1<<6)))|(1<<7)); /* Force MDI. */
  }
#endif

#if defined USE_DISABLE_100MB
  {
    unsigned long l = emac_phy_read (phy_address, 0);
    emac_phy_write (phy_address, 0, l & ~(1<<13));
  }
#endif
}


/* emac_phy_detect

   scans for a valid PHY attached to the EMAC.

*/

static void emac_phy_detect (void)
{
				/* Scan PHYs, #1 first, #0 last */
  for (phy_address = 1; phy_address < 33; ++phy_address) {
    unsigned int id1;
    unsigned int id2;
    id1 = emac_phy_read (phy_address & 0x1f, PHY_ID1);
    id2 = emac_phy_read (phy_address & 0x1f, PHY_ID2);
//    PRINTF ("%s: %d %d %d\n", __FUNCTION__, phy_address, id1, id2);
    if (   id1 != 0x0000 && id1 != 0xffff && id1 != 0x8000
	&& id2 != 0x0000 && id2 != 0xffff && id2 != 0x8000) {
      phy_id = (id1 << 16) | id2;
      phy_address &= 0x1f;
      PRINTF ("emac: phy_detect 0x%x  0x%lx\n", phy_address, phy_id);
      break;
    }
  }

  emac_phy_reset (phy_address);
  emac_phy_configure (phy_address);
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

  if (   parse_descriptor ("mac:0#8", &d)
      || open_descriptor (&d)) {
    printf ("ethernet mac eeprom unavailable\n");
    return -1;			/* No driver */
  }
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
  PRINTF ("emac: init\n");
  
	/* Hardware setup -- necessary for the PHY to be detected. */
  MASK_AND_SET (IOCON_MUXCTL1,
		(3<<8)|(3<<6)|(3<<4),
		(1<<8)|(1<<6)|(1<<4));		
//  MASK_AND_SET (IOCON_RESCTL1,
//		(3<<8)|(3<<6)|(3<<4),
//		(0<<8)|(0<<6)|(0<<4));
  MASK_AND_SET (IOCON_MUXCTL23,
		(3<<14)|(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0),
		(1<<14)|(1<<12)|(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0));
//  MASK_AND_SET (IOCON_RESCTL23,
//		(3<<14)|(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0),
//		(0<<14)|(0<<12)|(0<<10)|(0<<8)|(0<<6)|(0<<4)|(0<<2)|(0<<0));
  MASK_AND_SET (IOCON_MUXCTL24,
		(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0),
		(1<<12)|(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0));
//  MASK_AND_SET (IOCON_RESCTL24,
//		(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0),
//		(0<<12)|(0<<10)|(0<<8)|(0<<6)|(0<<4)|(0<<2)|(0<<0));

//  PRINTF ("CPLD_CONTROL %x => %x\n", CPLD_CONTROL,
//	  CPLD_CONTROL | CPLD_CONTROL_WRLAN_ENABLE);

  RCPC_CTRL       |=  RCPC_CTRL_UNLOCK;
  RCPC_AHBCLKCTRL &= ~(1<<2);
  RCPC_CTRL	  &= ~RCPC_CTRL_UNLOCK;

  {
    unsigned char rgb[6];
    if (!emac_read_mac (rgb)) {
      EMAC_SPECAD1BOT 
	= (rgb[3]<<24)|(rgb[2]<<16)|(rgb[1]<<8)|(rgb[0]<<0);
      EMAC_SPECAD1TOP 
	= (rgb[5]<<8)|(rgb[4]<<0);
#if defined (TALK)
      PRINTF ("emac: mac address\n"); 
      dump (rgb, 6, 0);
#endif
    }
  }

#if defined (CPLD_CONTROL_WRLAN_ENABLE)
  CPLD_CONTROL |= CPLD_CONTROL_WRLAN_ENABLE;
#endif

  emac_phy_detect ();		/* Detect and perform special setups */

  emac_setup ();		/* Prepare EMAC for IO */
}

#if !defined (CONFIG_SMALL)
static void emac_report (void)
{
  if (phy_id) {
    unsigned long bot = EMAC_SPECAD1BOT;
    unsigned long top = EMAC_SPECAD1TOP;
  
    printf ("  emac:   phy_addr %d  phy_id 0x%lx"
	    "  mac_addr %02x:%02x:%02x:%02x:%02x:%02x\n",
	    phy_address, phy_id,
	    (unsigned char) (bot >>  0),
	    (unsigned char) (bot >>  8),
	    (unsigned char) (bot >> 16),
	    (unsigned char) (bot >> 24),
	    (unsigned char) (top >>  0),
	    (unsigned char) (top >>  8)
	    );
  }
}
#endif

static __service_6 struct service_d lh7952x_emac_service = {
  .init = emac_init,
#if !defined (CONFIG_SMALL)
  .report = emac_report,
#endif
};

#if defined (USE_DIAG)

void emac_send_packet (void)
{
  struct ethernet_header* header = (struct ethernet_header*)rgbTxBuffer;
  int length = 128;

  {
    int i;
    for (i = 0; i < 128; ++i)
      rgbTxBuffer[i] = (unsigned char) i;
  }

  header->dest[0] = 0xff;
  header->dest[1] = 0xff;
  header->dest[2] = 0xff;
  header->dest[3] = 0xff;
  header->dest[4] = 0xff;
  header->dest[5] = 0xff;
  memcpy (&header->src, &header->dest, sizeof (header->src));
  header->protocol = htons (0x0806);

  rgl_tx_descriptor[0] = (unsigned long) rgbTxBuffer;
  rgl_tx_descriptor[1] = (1<<30)|(1<<15)|(length<<0);
  EMAC_TXBQP = (unsigned long) rgl_tx_descriptor;

  printf ("  preparing to send %lx %lx\n",   
	  rgl_tx_descriptor[0], 
	  rgl_tx_descriptor[1]);
  EMAC_NETCTL |= EMAC_NETCTL_STARTTX;
}
#endif

#if defined (USE_DIAG)

static void show_tx_flags (unsigned long l)
{
  if (l & (1<<31))
    printf (" used");
  if (l & (1<<30))
    printf (" wrap");
  if (l & (1<<29))
    printf (" retries");
  if (l & (1<<28))
    printf (" under");
  if (l & (1<<27))
    printf (" exhaust");
} 
#endif

#if defined (CONFIG_CMD_EMAC_LH79524)

static int cmd_emac (int argc, const char** argv)
{
  int result = 0;

  if (argc == 1) {
#if defined (USE_DIAG)
#if defined (TALK)
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
#else
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
	  emac_phy_write (phy_address, 28, 
			  (emac_phy_read (phy_address, 28) & 0x0fff)
			  | (rgRegs[index].reg & 0xf000));
	printf ("%5s-%-2d %04x  ", 
		rgRegs[index].label, rgRegs[index].reg & 0xff, 
		emac_phy_read (phy_address, rgRegs[index].reg & 0xff));
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
	    emac_phy_write (phy_address, 28, 
			    (emac_phy_read (phy_address, 28) & 0x0fff)
			    | (rgRegs[index].reg & 0xf000));
	  printf ("%04x    ", emac_phy_read (phy_address, 
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
      l = emac_phy_read (phy_address, 0);
      PRINTF ("phy_control 0x%lx\n", l);
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
      printf ("\n");
      //      l = emac_phy_read (phy_address, 4);
      //      PRINTF ("phy_advertisement 0x%lx\n", l);
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
      PRINTF ("\n");
      l = emac_phy_read (phy_address, 6);
      PRINTF ("phy_autoneg_expansion 0x%lx", l);
      if (l & (1<<4))
	PRINTF (" parallel-fault");
      if (l & (1<<1))
	PRINTF (" page-received");
      if (l & (1<<0))
	PRINTF (" partner-ANEN-able");
      PRINTF ("\n");
      //      l = emac_phy_read (phy_address, 7);
      //      PRINTF ("phy_autoneg_nextpage 0x%lx\n", l);
      //      l = emac_phy_read (phy_address, 16);
      //      PRINTF ("phy_bt_interrupt_level 0x%lx\n", l);
      l = emac_phy_read (phy_address, 17);
      PRINTF ("phy_interrupt_control 0x%lx", l);
      if (l & (1<<4))
	PRINTF (" parallel_fault");
      if (l & (1<<2))
	PRINTF (" link_not_ok");
      if (l & (1<<0))
	PRINTF (" anen_complete");
      PRINTF ("\n");
      l = emac_phy_read (phy_address, 18);
      PRINTF ("phy_diagnostic 0x%lx", l);
      if (l & (1<<12))
	PRINTF (" force 100TX");
      if (l & (1<<13))
	PRINTF (" force 10BT");
      PRINTF ("\n");
      //      l = emac_phy_read (phy_address, 19);
      //      PRINTF ("phy_power_loopback 0x%lx\n", l);
      //      l = emac_phy_read (phy_address, 20);
      //      PRINTF ("phy_cable 0x%lx\n", l);
      //      l = emac_phy_read (phy_address, 21);
      //      PRINTF ("phy_rxerr 0x%lx\n", l);
      //      l = emac_phy_read (phy_address, 22);
      //      PRINTF ("phy_power_mgmt 0x%lx\n", l);
      //      l = emac_phy_read (phy_address, 23);
      //      PRINTF ("phy_op_mode 0x%lx\n", l);
      //      l = emac_phy_read (phy_address, 24);
      //      PRINTF ("phy_last_crc 0x%lx\n", l);

      PRINTF ("emac_netctl 0x%lx\n", EMAC_NETCTL);
      PRINTF ("emac_netconfig 0x%lx\n", EMAC_NETCONFIG);
//      PRINTF ("emac_netstatus 0x%lx\n", EMAC_NETSTATUS);
      PRINTF ("emac_txstatus 0x%lx\n", EMAC_TXSTATUS);
      PRINTF ("emac_rxstatus 0x%lx\n", EMAC_RXSTATUS);
      PRINTF ("emac_txbqp 0x%lx\n", EMAC_TXBQP);
      PRINTF ("emac_rxbqp 0x%lx\n", EMAC_RXBQP);
      printf ("emac:tx0 & %p 0 0x%lx  1 0x%lx", 
	      rgl_tx_descriptor, 
	      rgl_tx_descriptor[0], 
	      rgl_tx_descriptor[1]);
      show_tx_flags (rgl_tx_descriptor[1]);
      printf ("\n");
      printf ("emac:tx1 & %p 0 0x%lx  1 0x%lx", 
	      &rgl_tx_descriptor[2], rgl_tx_descriptor[2], 
	      rgl_tx_descriptor[3]);
      show_tx_flags (rgl_tx_descriptor[3]);
      printf ("\n");
      printf ("emac:rx0 & %p 0 0x%lx  1 0x%lx\n", 
	      rgl_rx_descriptor, rgl_rx_descriptor[0], rgl_rx_descriptor[1]);
      printf ("emac:rx1 & %p 0 0x%lx  1 0x%lx\n", 
	      &rgl_rx_descriptor[2], rgl_rx_descriptor[2], 
	      rgl_rx_descriptor[3]);
    }
#endif
#endif
  }
  else {
#if defined (USE_DIAG)
    if (strcmp (argv[1], "clear") == 0) {
      EMAC_TXSTATUS = 0x7f;
      rgl_tx_descriptor[0] = 0;
      rgl_tx_descriptor[1] = 0;
    }
    else if (strcmp (argv[1], "send") == 0) {
      emac_send_packet ();
    }
    else if (strcmp (argv[1], "loop") == 0) {
      emac_phy_reset (phy_address);
      emac_phy_configure (phy_address);
      printf ("emac: loopback and restart anen\n");
      emac_phy_write (phy_address, 0,
		      PHY_CONTROL_RESTART_ANEN
		      | PHY_CONTROL_ANEN_ENABLE 
		      | PHY_CONTROL_LOOPBACK
		      | emac_phy_read (phy_address, 0));
      EMAC_NETCONFIG |= 0
	| EMAC_NETCONFIG_IGNORE
	| EMAC_NETCONFIG_ENFRM
	| EMAC_NETCONFIG_CPYFRM
	//	| EMAC_NETCONFIG_FULLDUPLEX
	;
    }
    else if (strcmp (argv[1], "anen") == 0) {
      emac_phy_reset (phy_address);
      emac_phy_configure (phy_address);
      printf ("emac: restart anen\n");
      emac_phy_write (phy_address, 0,
		      PHY_CONTROL_RESTART_ANEN | PHY_CONTROL_ANEN_ENABLE
		      | emac_phy_read (phy_address, 0));
    }
    else if (strcmp (argv[1], "force") == 0) {
      emac_phy_reset (phy_address);
      emac_phy_configure (phy_address);
      PRINTF ("emac: forcing power-up and restarting anen\n");
      emac_phy_write (phy_address, 22, 0); /* Force power-up */
      emac_phy_write (phy_address, 0, /* restart anen */
		      PHY_CONTROL_RESTART_ANEN | PHY_CONTROL_ANEN_ENABLE
		      | emac_phy_read (phy_address, 0));
    }
#endif

    	/* Set mac address */
    if (strcmp (argv[1], "mac") == 0) {
      unsigned char rgb[6];
      if (argc != 3)
	return ERROR_PARAM;

      {
	int i;
	char* pb = (char*) argv[2];
	for (i = 0; i < 6; ++i) {
	  if (!*pb)
	    ERROR_RETURN (ERROR_PARAM, "mac address too short");;
	  rgb[i] = simple_strtoul (pb, &pb, 16);
	  if (*pb)
	    ++pb;
	}
	if (*pb)
	  ERROR_RETURN (ERROR_PARAM, "mac address too long");;
      }

      EMAC_SPECAD1BOT 
	= (rgb[3]<<24)|(rgb[2]<<16)|(rgb[1]<<8)|(rgb[0]<<0);
      EMAC_SPECAD1TOP 
	= (rgb[5]<<8)|(rgb[4]<<0);
      printf ("emac: mac address\n"); 
      dump (rgb, 6, 0);
    }

    	/* Save mac address */
    if (strcmp (argv[1], "save") == 0) {
      unsigned char rgb[9];
      unsigned long bot = EMAC_SPECAD1BOT;
      unsigned long top = EMAC_SPECAD1TOP;

      rgb[0] = 0x94;		/* Signature */
      rgb[1] = 0x01;		/* OP: MAC address */
      rgb[8] = 0xff;		/* OP: end */

      rgb[2] = bot >>  0;
      rgb[3] = bot >>  8;
      rgb[4] = bot >> 16;
      rgb[5] = bot >> 24;
      rgb[6] = top >>  0;
      rgb[7] = top >>  8;
      
      {
	struct descriptor_d d;
	static const char sz[] = "mac:0#9";
	if (   !parse_descriptor (sz, &d)
	    && !open_descriptor (&d)) {
	  d.driver->erase (&d, 9);
	  d.driver->seek (&d, 0, SEEK_SET);
	  if (d.driver->write (&d, rgb, 9) != 9)
	    result = ERROR_RESULT (ERROR_FAILURE, 
				   "unable to write mac address");;
	  close_descriptor (&d);
	}
	else
	  ERROR_RETURN (ERROR_NODRIVER, 
			"mac eeprom driver unavailable");;
      }
    }
  }

  return result;
}

	/* Work-around for gcc-2.95 */
#if defined (USE_DIAG)
# define _USE_DIAG(s) s
#else
# define _USE_DIAG(s)
#endif

static __command struct command_d c_emac = {
  .command = "emac",
  .description = "manage ethernet MAC address",
  .func = cmd_emac,
  COMMAND_HELP(
"emac [SUBCOMMAND [PARAMETER]]\n"
"  Commands for the Ethernet MAC and PHY devices.\n"
_USE_DIAG(
"  Without a SUBCOMMAND, it displays diagnostics about the EMAC\n"
"    and PHY devices.  This information is for debugging the hardware.\n"
"  clear - reset the EMAC.\n"
"  anen  - restart auto negotiation.\n"
"  send  - send a test packet.\n"
"  loop  - enable loopback mode.\n"
"  force - force power-up and restart auto-negotiation.\n"
)
"  mac   - set the MAC address to PARAMETER.\n"
"    PARAMETER has the form XX:XX:XX:XX:XX:XX where each X is a\n"
"    hexadecimal digit.  Be aware that MAC addresses must be unique for\n"
"    proper operation of the network.  This command may be added to the\n"
"    startup commands to set the MAC address at boot-time.\n"
"  save  - saves the MAC address to the mac: EEPROM device.\n"
"    A saved MAC address will be used to automatically configure the MAC\n"
"    at startup.  For this feature to work, there must be a mac: driver.\n"
"  e.g.  emac mac 01:23:45:67:89:ab         # Never use this MAC address\n"
"        emac save\n"
  )
};

#endif 				/* CONFIG_CMD_EMAC_LH79524 */
