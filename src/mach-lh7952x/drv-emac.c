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

#if defined (TALK)
#define PRINTF(f...)		printf (f)
#else
#define PRINTF(f...)		do {} while (0)
#endif

static void enable_cs (void)
{
  PRINTF ("enable_cs 0x%x", __REG16 (CPLD_SPI));
  __REG16 (CPLD_SPI) |= CPLD_SPI_CS_MAC;
  usleep (T_CSS);
  PRINTF (" -> 0x%x\r\n", __REG16 (CPLD_SPI));
}

static void disable_cs (void)
{
  PRINTF ("disable_cs 0x%x", __REG16 (CPLD_SPI));
  __REG16 (CPLD_SPI) &= ~CPLD_SPI_CS_MAC;
  PRINTF (" -> 0x%x\r\n", __REG16 (CPLD_SPI));
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
  __REG (RCPC_PHYS | RCPC_CTRL) |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS + RCPC_AHBCLKCTRL) &= ~(1<<2);
  __REG (RCPC_PHYS | RCPC_CTRL) &= ~RCPC_CTRL_UNLOCK;
  __REG (IOCON_PHYS + IOCON_MUXCTL1) &= ~((3<<8)|(3<<6)|(3<<4));
  __REG (IOCON_PHYS + IOCON_MUXCTL1) |=   (1<<8)|(1<<6)|(1<<4);
  __REG (IOCON_PHYS + IOCON_MUXCTL23)
    &= ~((3<<14)|(3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<2)|(3<<0));
  __REG (IOCON_PHYS + IOCON_MUXCTL23)
    |=   (1<<14)|(1<<12)|(1<<10)|(1<<8)|(1<<6)|(1<<2)|(1<<0);
  __REG (IOCON_PHYS + IOCON_MUXCTL24)
    &= ~((3<<12)|(3<<10)|(3<<8)|(3<<6)|(3<<2)|(3<<0));
  __REG (IOCON_PHYS + IOCON_MUXCTL24)
    |=   (1<<12)|(1<<10)|(1<<8)|(1<<6)|(1<<2)|(1<<0);
  __REG (EMAC_PHYS + EMAC_SPECAD1BOT) 
    = (rgb[2]<<24)|(rgb[3]<<16)|(rgb[4]<<8)|(rgb[5]<<0);
  __REG (EMAC_PHYS + EMAC_SPECAD1TOP) 
    = (rgb[0]<<8)|(rgb[1]<<0);
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
