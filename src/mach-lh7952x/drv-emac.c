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

   Support for initialization of the embedded lh79524 Ethernet MAC.


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

/* *** FIXME: these timing values are substantially larger than the
   *** chip requires. We may implement an nsleep () function. */
#define T_SKH	1		/* Clock time high (us) */
#define T_SKL	1		/* Clock time low (us) */
#define T_CS	1		/* Minimum chip select low time (us)  */
#define T_CSS	1		/* Minimum chip select setup time (us)  */
#define T_DIS	1		/* Data setup time (us) */

//#define TALK

//#define NOISY 

#if defined (NOISY)
#define PRINTF(f...)		printf (f)
#else
#define PRINTF(f...)		do {} while (0)
#endif

#define PHY_CONTROL	0
#define PHY_STATUS	1
#define PHY_ID1		2
#define PHY_ID2		3

#define PHY_CONTROL_RESET		(1<<15)
#define PHY_CONTROL_RESTART_ANEN	(1<<12)
#define PHY_STATUS_ANEN_COMPLETE	(1<<5)
#define PHY_STATUS_LINK			(1<<2)

static int phy_address;

#define C_BUFFER	2
#define CB_BUFFER	1536
static long __attribute__((section("ethernet.bss"))) 
     rlDescriptor[C_BUFFER*2];
static char __attribute__((section("ethernet.bss")))
     rgbBuffer[CB_BUFFER*C_BUFFER];	/* Memory for buffers */

#define P_START		(1<<9)
#define P_WRITE		(1<<7)
#define P_READ		(2<<7)
#define P_ERASE		(3<<7)
#define P_EWDS		(0<<7)
#define P_WRAL		(0<<7)
#define P_ERAL		(0<<7)
#define P_EWEN		(0<<7)
#define P_A_EWDS	(0<<5)
#define P_A_WRAL	(1<<5)
#define P_A_ERAL	(2<<5)
#define P_A_EWEN	(3<<5)


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
    rlDescriptor[i*2] = ((unsigned long)(rgbBuffer + i*CB_BUFFER) & ~3)
      | ((i == C_BUFFER - 1) ? (1<<1) : 0);
    rlDescriptor[i*2 + 1] = 0;
  }
  printf ("rlDescriptor %p\r\n", rlDescriptor);
  __REG (EMAC_PHYS + EMAC_RXBQP) = (unsigned long) rlDescriptor;
  __REG (EMAC_PHYS + EMAC_NETCTL) |= EMAC_NETCTL_RXEN; /* Enable RX */
  __REG (EMAC_PHYS + EMAC_NETCTL) |= EMAC_NETCTL_TXEN; /* Enable TX */
}

static int emac_phy_read (int phy_address, int phy_register)
{
  PRINTF ("emac_phy_read %d %d\r\n", phy_address, phy_register);

  __REG (EMAC_PHYS + EMAC_NETCTL) |= EMAC_NETCTL_MANGEEN;
  __REG (EMAC_PHYS + EMAC_PHYMAINT)
    = (1<<30)|(2<<28)
    |((phy_address & 0x1f)<<23)
    |((phy_register & 0x1f)<<18)
    |(2<<16);

  usleep (2000*1000*1000/HCLK); /* wait 2000 HCLK cycles  */
  while ((__REG (EMAC_PHYS + EMAC_NETSTATUS) & (1<<2)) == 0)
    ;

  __REG (EMAC_PHYS + EMAC_NETCTL) &= ~EMAC_NETCTL_MANGEEN;

  PRINTF ("emac_phy_read => 0x%lx\r\n", __REG (EMAC_PHYS + EMAC_PHYMAINT));

  return __REG (EMAC_PHYS + EMAC_PHYMAINT) & 0xffff;
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
  int v = 0;
  printf ("reset phy\r\n");
  emac_phy_write (phy_address, 0,
		  PHY_CONTROL_RESET
		  | emac_phy_read (phy_address, 0));
  while (emac_phy_read (phy_address, 0) & PHY_CONTROL_RESET)
    ++v;
#if 0
  printf ("  resetart anen %d\r\n", v);
  emac_phy_write (phy_address, 0,
		  PHY_CONTROL_RESTART_ANEN
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
      printf ("0x%x  0x%x\r\n", phy_address & 0x1f, (id1 << 16) | id2);
      break;
    }
  }

  emac_phy_reset (phy_address);
}

void enable_cs (void)
{
  __REG16 (CPLD_SPI) |=  CPLD_SPI_CS_MAC;
  usleep (T_CSS);
}

static void disable_cs (void)
{
  __REG16 (CPLD_SPI) &= ~CPLD_SPI_CS_MAC;
  usleep (T_CS);
}

static void pulse_clock (void)
{
  __REG16 (CPLD_SPI) |=  CPLD_SPI_SCLK;
  usleep (T_SKH);
  __REG16 (CPLD_SPI) &= ~CPLD_SPI_SCLK;
  usleep (T_SKL);
}


/* execute_spi_command

   sends a spi command to a device.  It first sends cwrite bits from
   v.  If cread is greater than zero it will read cread bits
   (discarding the leading 0 bit) and return them.  If cread is less
   than zero it will check for completetion status and return 0 on
   success or -1 on timeout.  If cread is zero it does nothing other
   than sending the command.

*/

static int execute_spi_command (int v, int cwrite, int cread)
{
  unsigned long l = 0;

  PRINTF ("0x%08x %d", v, cwrite);

  enable_cs ();

  v <<= CPLD_SPI_TX_SHIFT;	/* Correction for the position of SPI_TX bit */
  while (cwrite--) {
    __REG16 (CPLD_SPI) 
      = (__REG16 (CPLD_SPI) & ~CPLD_SPI_TX)
      | ((v >> cwrite) & CPLD_SPI_TX);
    usleep (T_DIS);
    pulse_clock ();
  }

  if (cread < 0) {
    unsigned long time;
    disable_cs ();
    time = timer_read ();
    enable_cs ();
	
    l = -1;
    do {
      if (__REG16 (CPLD_SPI) & CPLD_SPI_RX) {
	l = 0;
	break;
      }
    } while (timer_delta (time, timer_read ()) < 10*1000);
  }
  else
	/* We pulse the clock before the data to skip the leading zero. */
    while (cread-- > 0) {
      pulse_clock ();
      l = (l<<1) 
	| (((__REG16 (CPLD_SPI) & CPLD_SPI_RX) >> CPLD_SPI_RX_SHIFT) & 0x1);
    }

  disable_cs ();
  PRINTF (" -> %lx\r\n", l);
  return l;
}


/* emac_read_mac

   pulls a MAC address from the MAC EEPROM if there is one.  The
   format of this EEPROM data is described in the comment above.

*/

static int emac_read_mac (char rgb[6])
{
  int i;
  if (   execute_spi_command (P_START|P_READ|0, 10, 8) != 0x94
      || execute_spi_command (P_START|P_READ|1, 10, 8) != 0x01)
    return -1;

  for (i = 0; i < 6; ++i)
    rgb[i] = execute_spi_command (P_START|P_READ|(i + 2), 10, 8);
  return 0;
}


void emac_init (void)
{
#if defined (TALK)
  PRINTF ("  EMAC\r\n");
#endif
  
	/* Hardware setup */
  __REG16 (CPLD_CONTROL) |= CPLD_CONTROL_WRLAN_ENABLE;
  __REG (RCPC_PHYS | RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS + RCPC_AHBCLKCTRL) &= ~(1<<2);
  __REG (RCPC_PHYS | RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;

  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL1),
		(3<<8)|(3<<6)|(3<<4),
		(1<<8)|(1<<6)|(1<<4));		

  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_RESCTL1),
		(3<<8)|(3<<6)|(3<<4),
		(0<<8)|(0<<6)|(0<<4));
  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL23),
		(3<<14)|(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0),
		(1<<14)|(1<<12)|(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0));
  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_RESCTL23),
		(3<<14)|(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0),
		(0<<14)|(0<<12)|(0<<10)|(0<<8)|(0<<6)|(0<<4)|(0<<2)|(0<<0));
  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_MUXCTL24),
		(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0),
		(1<<12)|(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0));
  MASK_AND_SET (__REG (IOCON_PHYS + IOCON_RESCTL24),
		(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0),
		(0<<12)|(0<<10)|(0<<8)|(0<<6)|(0<<4)|(0<<2)|(0<<0));

  {
    unsigned char rgb[6];
    if (!emac_read_mac (rgb)) {
      __REG (EMAC_PHYS + EMAC_SPECAD1BOT) 
	= (rgb[2]<<24)|(rgb[3]<<16)|(rgb[4]<<8)|(rgb[5]<<0);
      __REG (EMAC_PHYS + EMAC_SPECAD1TOP) 
	= (rgb[0]<<8)|(rgb[1]<<0);
    }
  }

  emac_setup ();
  emac_phy_detect ();

  /* Disable advertisement except for 100Base-TX  */
  {
    unsigned long l = emac_phy_read (phy_address, 4);
    emac_phy_write (phy_address, 4, l & ~((1<<8)|(1<<6)|(1<<5)));
  }
}

static __service_6 struct service_d lh7952x_emac_service = {
  .init = emac_init,
};

#if defined (CONFIG_CMD_EMAC)

int cmd_emac (int argc, const char** argv)
{
  if (argc == 1) {
    static const int cbMax = 0x20;
    char rgb[cbMax]; 
    int i;

    for (i = 0; i < cbMax; ++i)
      rgb[i] = (unsigned char) execute_spi_command (P_START|P_READ|i, 10, 8);

    dump (rgb, cbMax, 0);

#if 0
    {
      unsigned long l;
      l = emac_phy_read (phy_address, 0);
      printf ("phy_control 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 1);
      printf ("phy_status 0x%lx %ld %ld\r\n",
	      l,
	      l&PHY_STATUS_ANEN_COMPLETE,
	      l&PHY_STATUS_LINK);
      l = emac_phy_read (phy_address, 4);
      printf ("phy_advertisement 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 5);
      printf ("phy_partner 0x%lx", l);
      if (l & (1<<9))
	printf ("  100Base-T4");
      if (l & (1<<8))
	printf ("  100Base-TX-full");
      if (l & (1<<7))
	printf ("  100Base-TX");
      if (l & (1<<6))
	printf ("  10Base-T-full");
      if (l & (1<<5))
	printf ("  10Base-T");
      printf ("\r\n");
      l = emac_phy_read (phy_address, 6);
      printf ("phy_autoneg_expansion 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 7);
      printf ("phy_autoneg_nextpage 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 16);
      printf ("phy_bt_interrupt_level 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 17);
      printf ("phy_interrupt_control 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 18);
      printf ("phy_diagnostic 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 19);
      printf ("phy_power 0x%lx\r\n", l);
      l = emac_phy_read (phy_address, 20);
      printf ("phy_cable 0x%lx\r\n", l);
    }
#endif
  }
  else {
    unsigned char rgb[9];
    if (argc != 2)
      return ERROR_PARAM;

    rgb[0] = 0x94;		/* Signature */
    rgb[1] = 0x01;		/* OP: MAC address */
    rgb[8] = 0xff;		/* OP: end */

    {
      int i;
      char* pb = (char*) argv[1];
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
      int i;

      execute_spi_command (P_START|P_EWEN|P_A_EWEN, 10, 0);

		/* Erase everything */
//      if (execute_spi_command (P_START|P_ERAL|P_A_ERAL, 10, -1))
//	return ERROR_FAILURE;

      for (i = 0; i < 9; ++i) {
	if (execute_spi_command (P_START|P_ERASE|i, 10, -1))
	  return ERROR_FAILURE;

	if (execute_spi_command (((P_START|P_WRITE|i)<<8)|rgb[i], 18, -1))
	  return ERROR_FAILURE;
      }
      execute_spi_command (P_START|P_EWDS|P_A_EWDS, 10, 0);
    }
  }

  return 0;
}

static __command struct command_d c_emac = {
  .command = "emac",
  .description = "manage ethernet MAC address",
  .func = cmd_emac,
};

#endif
