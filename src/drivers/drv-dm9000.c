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

#define DM_PHYS		(0x10000000)
#define DM_INDEX	__REG16 (DM_PHYS)
#define DM_DATA		__REG16 (DM_PHYS + 4)

#if defined (CONFIG_CMD_ETH_DM9000)

static int cmd_eth (int argc, const char** argv)
{
  int result = 0;

  if (argc == 1) {
    unsigned short vendor = 0;
    unsigned short product = 0;
    unsigned short chip = 0;

    DM_INDEX = DM9000_VIDL;
    vendor |= DM_DATA;
    DM_INDEX = DM9000_VIDH;
    vendor |= (DM_DATA << 8);

    DM_INDEX = DM9000_PIDL;
    product |= DM_DATA;
    DM_INDEX = DM9000_PIDH;
    product |= (DM_DATA << 8);

    DM_INDEX = DM9000_CHIPR;
    chip |= DM_DATA;

    printf ("dm9000: vendor 0x%x  product 0x%x  chip 0x%x\n",
	    vendor, product, chip);
  }

#if 0

  if (argc == 1) {>
    {
      unsigned i;
      struct regs {
	int reg;
	char label[5];
      };
      static struct regs __rodata rgRegs[] = {
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
    select_bank (0);
    PRINT_REG;
    select_bank (1);
    PRINT_REG;
    select_bank (2);
    PRINT_REG;
    select_bank (3);
    PRINT_REG;
  }
  else {
    if (strcmp (argv[1], "en") == 0) {
      smc91x_phy_configure ();
    }
    if (strcmp (argv[1], "an") == 0) {
      int v;
      smc91x_phy_write (phy_address, PHY_ANEG_ADV,
			0
			| (1<<8)
			| (1<<7)
			| (1<<6)
			| (1<<5)
			| (1<<0));
      smc91x_phy_read (phy_address, PHY_ANEG_ADV);
      v = smc91x_phy_read (phy_address, PHY_CONTROL);
      printf ("restarting anen\n");
      v |= PHY_CONTROL_ANEN_ENABLE | PHY_CONTROL_RESTART_ANEN;
      printf ("Writing PHY_CONTROL 0x%x\n", v);
      smc91x_phy_write (phy_address, PHY_CONTROL, v);
      v = smc91x_phy_read (phy_address, PHY_CONTROL);
      printf ("Read back PHY_CONTROL 0x%x\n", v);
    }
  }
#endif

  return result;
}

static __command struct command_d c_eth = {
  .command = "eth",
  .description = "dm9000 diagnostics",
  .func = cmd_eth,
  COMMAND_HELP(
"eth [SUBCOMMAND [PARAMETER]]\n"
"  Commands for the Ethernet MAC and PHY devices.\n"
"  Without a SUBCOMMAND, it displays diagnostics about the EMAC\n"
"    and PHY devices.  This information is for debugging the hardware.\n"
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
