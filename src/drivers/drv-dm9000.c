/* drv-dm9000.c

   written by Marc Singer
   26 May 2006

   Copyright (C) 2006 Marc Singer

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

   Optimization
   ------------

   This code is somewhat wasteful.  We should be able to recover some
   space in the loader through use herein.

*/

#include <apex.h>
#include <config.h>
#include <driver.h>
#include <service.h>
#include <command.h>
#include <error.h>
#include <linux/string.h>
#include <asm/reg.h>
#include <linux/kernel.h>

#include <dm9000.h>

#include <mach/drv-dm9000.h>

#define TALK 3

#if defined (TALK)
#define PRINTF(f...)		printf (f)
#define ENTRY(l)		printf ("%s\n", __FUNCTION__)
#define DBG(l,f...)		do { if (TALK >= l) printf (f); } while (0)
#else
#define PRINTF(f...)		do {} while (0)
#define ENTRY(l)		do {} while (0)
#define DBG(l,f...)		do {} while (0)
#endif

#if !defined (C_DM)
# define C_DM	(1)
#endif

struct dm9000 {
  int present;
  unsigned short vendor;
  unsigned short product;
  unsigned short chip;
  u16 rgs_eeprom[64/2];		/* Data copied from the eeprom */
};

struct dm9000 dm9000[C_DM];

static void write_reg (int index, u16 value)
{
  DM_INDEX = index;
  DM_DATA = value;
}

static u16 read_reg (int index)
{
  DM_INDEX = index;
  return DM_DATA;
}

static u16 read_eeprom (int index)
{
  u16 v;

  write_reg (DM9000_EPAR, index);
  write_reg (DM9000_EPCR, EPCR_ERPRR);
  mdelay (10);	/* according to the datasheet 200us should be enough,
		   but it doesn't work */
  write_reg (DM9000_EPCR, 0x0);

  v = read_reg (DM9000_EPDRL) & 0xff;
  v |= (read_reg (DM9000_EPDRH) & 0xff) << 8;
  return v;
}

static void write_eeprom (int index, u16 value)
{
  write_reg (DM9000_EPAR, index);
  write_reg (DM9000_EPDRH, (value >> 8) & 0xff);
  write_reg (DM9000_EPDRL,  value       & 0xff);
  write_reg (DM9000_EPCR, EPCR_WEP | EPCR_ERPRW);
  mdelay (10);
  write_reg (DM9000_EPCR, 0);
}

static void dm9000_read_eeprom (void)
{
  int i;
  for (i = 0; i < sizeof (dm9000[0].rgs_eeprom)/sizeof (u16); ++i)
    dm9000[0].rgs_eeprom[i] = read_eeprom (i);
}


#if defined (CONFIG_CMD_ETH_DM9000)

  /* *** FIXME: this shouldn't be in the command conditional */
#if defined (CONFIG_ETHERNET)
extern char host_mac_address[];
#else
static char host_mac_address[6];
#endif

static int cmd_eth (int argc, const char** argv)
{
  int result = 0;

  if (!dm9000[0].present)
    ERROR_RETURN (ERROR_FAILURE, "dm9000 device not present");

  if (argc == 1) {
    printf ("dm9000: vendor 0x%x  product 0x%x  chip 0x%x\n",
	    dm9000[0].vendor, dm9000[0].product, dm9000[0].chip);
  }
  else {
	/* Set mac address */
    if (strcmp (argv[1], "mac") == 0) {
      char __aligned rgb[6];
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

      memcpy (host_mac_address, rgb, 6); /* For networking layer */

#if 0
      EMAC_SPECAD1BOT
	= (rgb[3]<<24)|(rgb[2]<<16)|(rgb[1]<<8)|(rgb[0]<<0);
      EMAC_SPECAD1TOP
	= (rgb[5]<<8)|(rgb[4]<<0);
      printf ("emac: mac address\n");
      dump (rgb, 6, 0);
#endif
    }

    if (strcmp (argv[1], "save") == 0) {
      u16 rgs[3];
      rgs[0] = host_mac_address[0] | (host_mac_address[1] << 8);
      rgs[1] = host_mac_address[2] | (host_mac_address[3] << 8);
      rgs[2] = host_mac_address[4] | (host_mac_address[5] << 8);
      write_eeprom (0, rgs[0]);
      write_eeprom (1, rgs[1]);
      write_eeprom (2, rgs[2]);
    }

    if (strcmp (argv[1], "re") == 0) {
      dm9000_read_eeprom ();
      dump ((void*) dm9000[0].rgs_eeprom, sizeof (dm9000[0].rgs_eeprom), 0);
    }
  }

  return result;
}

static __command struct command_d c_eth = {
  .command = "eth",
  .func = cmd_eth,
  COMMAND_DESCRIPTION ("dm9000 diagnostics")
  COMMAND_HELP(
"eth [SUBCOMMAND [PARAMETER]]\n"
"  Commands for the Ethernet MAC and PHY devices.\n"
"  Without a SUBCOMMAND, it displays info about the chip\n"
"    This information is for debugging the hardware.\n"
//"  clear - reset the EMAC.\n"
//"  anen  - restart auto negotiation.\n"
//"  send  - send a test packet.\n"
//"  loop  - enable loopback mode.\n"
//"  force - force power-up and restart auto-negotiation.\n"
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

#endif

static void dm9000_init (void)
{
  int i;

  for (i = 0; i < C_DM; ++i) {

    dm9000[i].vendor  = read_reg (DM9000_VIDL) | (read_reg (DM9000_VIDH) << 8);
    dm9000[i].product = read_reg (DM9000_PIDL) | (read_reg (DM9000_PIDH) << 8);
    dm9000[i].chip    = read_reg (DM9000_CHIPR);

    if (dm9000[i].vendor != 0xa46 || dm9000[i].product != 0x9000)
      return;

    dm9000[i].present = 1;

    dm9000[i].rgs_eeprom[0] = read_eeprom (0);
    dm9000[i].rgs_eeprom[1] = read_eeprom (1);
    dm9000[i].rgs_eeprom[2] = read_eeprom (2);

    if (i == 0) {
      host_mac_address[0] =  dm9000[i].rgs_eeprom[0]       & 0xff;
      host_mac_address[1] = (dm9000[i].rgs_eeprom[0] >> 8) & 0xff;
      host_mac_address[2] =  dm9000[i].rgs_eeprom[1]       & 0xff;
      host_mac_address[3] = (dm9000[i].rgs_eeprom[1] >> 8) & 0xff;
      host_mac_address[4] =  dm9000[i].rgs_eeprom[2]       & 0xff;
      host_mac_address[5] = (dm9000[i].rgs_eeprom[2] >> 8) & 0xff;
    }
  }
}


#if !defined (CONFIG_SMALL)
static void dm9000_report (void)
{
  int i;

  for (i = 0; i < C_DM; ++i) {
    if (dm9000[i].present) {
      printf ("  dm9000: phy_addr %d  phy_id 0x%lx"
	      "  mac_addr %02x:%02x:%02x:%02x:%02x:%02x\n",
	      -1, (unsigned long)-1,
	      (dm9000[i].rgs_eeprom[0]) & 0xff,
	      (dm9000[i].rgs_eeprom[0] >> 8) & 0xff,
	      (dm9000[i].rgs_eeprom[1]) & 0xff,
	      (dm9000[i].rgs_eeprom[1] >> 8) & 0xff,
	      (dm9000[i].rgs_eeprom[2]) & 0xff,
	      (dm9000[i].rgs_eeprom[2] >> 8) & 0xff);
    }
  }
}
#endif


static __service_6 struct service_d dm9000_service = {
  .init = dm9000_init,
  //  .release = dm9000_release,
#if !defined (CONFIG_SMALL)
  .report = dm9000_report,
#endif
};
