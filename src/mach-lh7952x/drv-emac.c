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
  printf ("  reset %d\r\n", v);
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



static void enable_cs (void)
{
  //  PRINTF ("enable_cs 0x%x", __REG16 (CPLD_SPI));
  __REG16 (CPLD_SPI) |= CPLD_SPI_CS_MAC;
  usleep (T_CSS);
  //  PRINTF (" -> 0x%x\r\n", __REG16 (CPLD_SPI));
}

static void disable_cs (void)
{
  //  PRINTF ("disable_cs 0x%x", __REG16 (CPLD_SPI));
  __REG16 (CPLD_SPI) &= ~CPLD_SPI_CS_MAC;
  //  PRINTF (" -> 0x%x\r\n", __REG16 (CPLD_SPI));
}


/* wait_for_write

   returns 0 on success.

*/

static int wait_for_write (void)
{
  unsigned long time = timer_read ();
  usleep (T_CS);
  enable_cs ();
	
  do {
    if (__REG16 (CPLD_SPI) & CPLD_SPI_RX) {
      disable_cs ();
      return 0;
    }
  } while (timer_delta (time, timer_read ()) < 10*1000);

  disable_cs ();
  return -1;
}

static void pulse_clock (void)
{
  __REG16 (CPLD_SPI) |=  CPLD_SPI_SCLK;
  usleep (T_SKH);
  __REG16 (CPLD_SPI) &= ~CPLD_SPI_SCLK;
  usleep (T_SKL);
}

static void write_bits (int v, int c)
{
  while (c--) {
    if (v & (1<<c))
      __REG16 (CPLD_SPI) |=  CPLD_SPI_TX;
    else
      __REG16 (CPLD_SPI) &= ~CPLD_SPI_TX;
    usleep (T_DIS);
    pulse_clock ();
  }
}

static int read_bits (int c)
{
  int v = 0;
  while (c--) {
    pulse_clock ();
    v = (v<<1) 
      | (((__REG16 (CPLD_SPI) & CPLD_SPI_RX) >> CPLD_SPI_RX_SHIFT) & 0x1);
  }
  return v;
}

static void emac_init (void)
{
  /* Staticlly initialized MAC address until we can get the eeprom
     working. */
  static const unsigned char rgb[6] = { 0x00, 0x08, 0xee, 0x00, 0x12, 0xa1 };

#if defined (TALK)
  PRINTF ("  EMAC\r\n");
  dump (rgb, 6, 0);
#endif
  
	/* Hardware setup */
  __REG16 (CPLD_CONTROL) |= CPLD_CONTROL_WRLAN_ENABLE;
  __REG (RCPC_PHYS | RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS + RCPC_AHBCLKCTRL) &= ~(1<<2);
  __REG (RCPC_PHYS | RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;
  __REG (IOCON_PHYS + IOCON_MUXCTL1) &= ~((3<<8)|(3<<6)|(3<<4));
  __REG (IOCON_PHYS + IOCON_MUXCTL1) |=   (1<<8)|(1<<6)|(1<<4);
  __REG (IOCON_PHYS + IOCON_RESCTL1)  = (2<<8)|(2<<6)|(2<<4);
  __REG (IOCON_PHYS + IOCON_MUXCTL23)
    &= ~((3<<14)|(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0));
  __REG (IOCON_PHYS + IOCON_MUXCTL23)
    |=   (1<<14)|(1<<12)|(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0);
  __REG (IOCON_PHYS + IOCON_RESCTL23)  
     =   (2<<14)|(2<<12)|(2<<10)|(2<<8)|(2<<6)|(2<<4)|(2<<2)|(2<<0);
  __REG (IOCON_PHYS + IOCON_MUXCTL24)
    &= ~((3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<4)|(3<<2)|(3<<0));
  __REG (IOCON_PHYS + IOCON_MUXCTL24)
    |=   (1<<12)|(1<<10)|(1<<8)|(1<<6)|(1<<4)|(1<<2)|(1<<0);
  __REG (IOCON_PHYS + IOCON_RESCTL24)
     =   (2<<12)|(2<<10)|(2<<8)|(2<<6)|(2<<4)|(2<<2)|(2<<0);
  __REG (EMAC_PHYS + EMAC_SPECAD1BOT) 
    = (rgb[2]<<24)|(rgb[3]<<16)|(rgb[4]<<8)|(rgb[5]<<0);
  __REG (EMAC_PHYS + EMAC_SPECAD1TOP) 
    = (rgb[0]<<8)|(rgb[1]<<0);

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
    char rgb[128]; 
    int i;

    for (i = 0; i < 128; ++i) {

      enable_cs ();
      write_bits ((1<<9)|(2<<7)|i, 10);			/* READ | address */

      rgb[i] = (read_bits (8) & 0xff);

      disable_cs ();
    }

    dump (rgb, 128, 0);

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
  }
  else {
    unsigned char rgb[6];
    if (argc != 2)
      return ERROR_PARAM;

    {
      int i;
      char* pb = (char*) argv[1];
      for (i = 0; i < 6; ++i) {
	if (!*pb)
	  return ERROR_PARAM;
	rgb[i] = simple_strtoul (pb, &pb, 16);
	if (*pb)
	  ++pb;
      }
      if (*pb)
	return ERROR_PARAM;
    }

    dump (rgb, 6, 0);

#if 1
    {
      int i;
      enable_cs ();
      write_bits (10, (1<<9)|(0<<7)|(3<<5));		/* EWEN */
      disable_cs ();

      for (i = 0; i < 6; ++i) {
	enable_cs ();
	write_bits (10, (1<<9)|(1<<7)|i);		/* WRITE */
	write_bits (8, rgb[i]);
	disable_cs ();
	if (wait_for_write ())
	  return ERROR_FAILURE;
      }
    }
#endif
  }

  return 0;
}

static __command struct command_d c_emac = {
  .command = "emac",
  .description = "manage ethernet mac address",
  .func = cmd_emac,
};

#endif
