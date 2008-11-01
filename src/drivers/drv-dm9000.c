/* drv-dm9000.c

   written by Marc Singer
   26 May 2006

   Copyright (C) 2006 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   o Optimization. This code is somewhat wasteful.  We should be able
     to recover some space in the loader through use herein.

   o Multiple dm9000's.  If multiple DM9000 support is enabled in the
     boot loader and only one is stuffed on the target and it is the
     second of the two, you may need to use the -1 switch when setting
     the MAC address even though APEX doesn't really care.

   o Failed probe.  In some cases, it may be desirable to report that
     a probe failed and show the data read from the dm9000 when the
     chip isn't recognized.  Presently, this requires a recompile.

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

//#define TALK 3

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

#if DM_WIDTH != 16
# error "DM9000 driver only supports 16 bit bus"
#endif

#if !defined DM_IO_DELAY
# define DM_IO_DELAY
#endif

#define DRIVER_NAME		 "eth-dm9000"

struct dm9000 {
  int present;
  unsigned short vendor;
  unsigned short product;
  unsigned short chip;
  u16 rgs_eeprom[64/sizeof (u16)]; /* Data copied from the eeprom */
  volatile u16* index;
  volatile u16* data;
  const char* name;
};

static struct dm9000 dm9000[C_DM];
#if C_DM > 1
static int g_dm9000_default;	/* Default dm9000 for commands  */
#else
# define g_dm9000_default 0
#endif
static int g_dm9000_present;

static void write_reg (int dm, int index, u16 value)
{
  DBG (3, "%s: [%d] %x <- %x\n", __FUNCTION__, dm, index, value);
  *dm9000[dm].index = index;
  DM_IO_DELAY;
  *dm9000[dm].data  = value;
}

static u16 read_reg (int dm, int index)
{
  int value;
  *dm9000[dm].index = index;
  DM_IO_DELAY;
  value =  *dm9000[dm].data;
  DBG (3, "%s: [%d] %x -> %x\n", __FUNCTION__, dm, index, value);
  return value;
}

static u16 read_eeprom (int dm, int index)
{
  u16 v;

  write_reg (dm, DM9000_EPAR, index);
  write_reg (dm, DM9000_EPCR, EPCR_ERPRR);
  mdelay (10);	/* according to the datasheet 200us should be enough,
		   but it doesn't work */
  write_reg (dm, DM9000_EPCR, 0x0);

  v = read_reg (dm, DM9000_EPDRL) & 0xff;
  v |= (read_reg (dm, DM9000_EPDRH) & 0xff) << 8;
  return v;
}

static void write_eeprom (int dm, int index, u16 value)
{
  write_reg (dm, DM9000_EPAR, index);
  write_reg (dm, DM9000_EPDRH, (value >> 8) & 0xff);
  write_reg (dm, DM9000_EPDRL,  value       & 0xff);
  write_reg (dm, DM9000_EPCR, EPCR_WEP | EPCR_ERPRW);
  mdelay (10);
  write_reg (dm, DM9000_EPCR, 0);
}

static void dm9000_read_eeprom (int dm)
{
  int i;
  for (i = 0; i < sizeof (dm9000[dm].rgs_eeprom)/sizeof (u16); ++i)
    dm9000[dm].rgs_eeprom[i] = read_eeprom (dm, i);
}


#if defined (CONFIG_CMD_ETH_DM9000)

void dm9000_dump (char* rgb)
{
  printf ("NCR %s %s",
          (rgb[DM9000_NCR] & NCR_FDX) ? "FDX" : "HDX",
          (rgb[DM9000_NCR] & NCR_WAKEEN) ? "WAKE" : "!wake"
          );
  printf ("NSR %s %s %s %s %s %s",
          (rgb[DM9000_NSR] & NSR_RXOV) ? "RXOV" : "!rxov",
          (rgb[DM9000_NSR] & NSR_TX1END) ? "TX1END" : "",
          (rgb[DM9000_NSR] & NSR_TX2END) ? "TX2END" : "",
          (rgb[DM9000_NSR] & NSR_WAKEST) ? "WAKEST" : "",
          (rgb[DM9000_NSR] & NSR_LINKST) ? "LINK" : "!link",
          (rgb[DM9000_NSR] & NSR_SPEED) ? "100Mbps" : "10Mbps"
          );
  printf ("ISR %s %s %s %s %s %s\n",
          (rgb[DM9000_ISR] & ISR_PRS) ? "RCV" : "!rcv",
          (rgb[DM9000_ISR] & ISR_PTS) ? "TXC" : "!txc",
          (rgb[DM9000_ISR] & ISR_ROS) ? "ROV" : "!rov",
          (rgb[DM9000_ISR] & ISR_ROOS) ? "ROOC" : "!rooc",
          (rgb[DM9000_ISR] & ISR_UDRUN) ? "TUN" : "!tun",
          (rgb[DM9000_ISR] & ISR_LINKCHG) ? "LINK" : "!link"
          );
  printf ("IMR %s %s %s %s %s %s\n",
          (rgb[DM9000_IMR] & IMR_PRM) ? "RCV" : "!rcv",
          (rgb[DM9000_IMR] & IMR_PTM) ? "TXC" : "!txc",
          (rgb[DM9000_IMR] & IMR_ROM) ? "ROV" : "!rov",
          (rgb[DM9000_IMR] & IMR_ROOM) ? "ROOC" : "!rooc",
          (rgb[DM9000_IMR] & IMR_UDRUNM) ? "TUN" : "!tun",
          (rgb[DM9000_IMR] & IMR_LINKCHGM) ? "LINK" : "!link"
          );
}


  /* *** FIXME: this shouldn't be in the command conditional */
#if defined (CONFIG_ETHERNET)
extern char host_mac_address[];
#else
static char host_mac_address[6];
#endif

static int cmd_eth (int argc, const char** argv)
{
  int result = 0;
  int dm = g_dm9000_default;

  if (argc > 1 && argv[1][0] == '-') {
    switch (argv[1][1]) {
    default:
    case '0':
      break;
#if C_DM > 0
    case '1':
      dm = 1;
      break;
#endif
    }
    --argc;
    ++argv;
  }

  if (!dm9000[dm].present)
    ERROR_RETURN (ERROR_FAILURE, "dm9000 device not present");

  if (argc == 1) {
    printf ("dm9000[%d]: vendor 0x%x  product 0x%x  chip 0x%x\n",
	    dm, dm9000[dm].vendor, dm9000[dm].product, dm9000[dm].chip);
    printf ("  0x%02x 0x%02x 0x%02x 0x%02x 0x%04x\n",
            read_reg (dm, DM9000_VIDH), read_reg (dm, DM9000_VIDL),
            read_reg (dm, DM9000_PIDH), read_reg (dm, DM9000_PIDL),
            read_reg (dm, DM9000_CHIPR));
  }
  else {
	/* Set mac address */
    if (PARTIAL_MATCH (argv[1], "m", "ac") == 0) {
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
    }

    if (PARTIAL_MATCH (argv[1], "s", "ave") == 0) {
      u16 rgs[3];
      rgs[0] = host_mac_address[0] | (host_mac_address[1] << 8);
      rgs[1] = host_mac_address[2] | (host_mac_address[3] << 8);
      rgs[2] = host_mac_address[4] | (host_mac_address[5] << 8);
      write_eeprom (dm, 0, rgs[0]);
      write_eeprom (dm, 1, rgs[1]);
      write_eeprom (dm, 2, rgs[2]);
      dm9000_read_eeprom (dm);
    }

    if (PARTIAL_MATCH (argv[1], "rea", "d-eeprom") == 0) {
      dm9000_read_eeprom (dm);
      dump ((void*) dm9000[dm].rgs_eeprom, sizeof (dm9000[dm].rgs_eeprom), 0);
    }

    if (PARTIAL_MATCH (argv[1], "reg", "") == 0) {
      if (argc == 2) {
        int reg;
        char rgb[256];
        for (reg = 0; reg < sizeof (rgb); ++reg)
          rgb[reg] = (unsigned char) read_reg (dm, reg);
        dumpw (rgb, sizeof (rgb), 0, 0);
        dm9000_dump (rgb);
      }
      else {
        int reg = simple_strtoul (argv[2], NULL, 0);
        unsigned char value = read_reg (dm, reg);
        printf ("reg 0x%x -> 0x%x (%d)\n", reg, value, value);
      }
    }

    if (PARTIAL_MATCH (argv[1], "w", "rite") == 0) {
      int reg = simple_strtoul (argv[2], NULL, 0);
      unsigned char value = simple_strtoul (argv[3], NULL, 0);
      write_reg (dm, reg, value);
      printf ("reg 0x%x <- 0x%x (%d)\n", reg, value, value);
    }
  }

  return result;
}

static __command struct command_d c_eth = {
  .command = "eth",
  .func = cmd_eth,
  COMMAND_DESCRIPTION ("dm9000 diagnostics")
  COMMAND_HELP(
"eth [-#] [SUBCOMMAND [PARAMETER]]\n"
"  Commands for the Ethernet MAC and PHY devices.\n"
"  Without a SUBCOMMAND, it displays info about the chip\n"
"    This information is for debugging the hardware.\n"
"  -#          - select interface number # (0..N-1)\n"
//"  clear - reset the EMAC.\n"
//"  anen  - restart auto negotiation.\n"
//"  send  - send a test packet.\n"
//"  loop  - enable loopback mode.\n"
//"  force - force power-up and restart auto-negotiation.\n"
"  mac         - set the MAC address to PARAMETER.\n"
"    PARAMETER has the form XX:XX:XX:XX:XX:XX where each X is a\n"
"    hexadecimal digit.  Be aware that MAC addresses must be unique for\n"
"    proper operation of the network.  This command may be added to the\n"
"    startup commands to set the MAC address at boot-time.\n"
"  save        - saves the MAC address to the mac: EEPROM device.\n"
"    A saved MAC address will be used to automatically configure the MAC\n"
"    at startup.  For this feature to work, there must be a mac: driver.\n"
"  read-eeprom - reads and dumps the contents of the EEPROM.\n"
"  reg [IDX]   - show dm9000 register IDX or all registers\n"
"  write IDX VAL - write dm9000 register IDX with value VAL\n"
"  e.g.  eth mac 01:23:45:67:89:ab         # Never use this MAC address\n"
"        eth save\n"
  )
};

#endif

static void dm9000_init (void)
{
  int dm;

#if C_DM > 1
  g_dm9000_default = -1;
#endif

  for (dm = 0; dm < C_DM; ++dm) {

    DBG (2, "%s: %d\n", __FUNCTION__, dm);

    switch (dm) {
    case 0:
      dm9000[dm].index = &__REG16 (DM_PHYS_INDEX);
      dm9000[dm].data  = &__REG16 (DM_PHYS_DATA);
# if defined (DM_NAME)
      dm9000[dm].name = DM_NAME;
# endif
      break;
#if C_DM > 1
    case 1:
      dm9000[dm].index = &__REG16 (DM1_PHYS_INDEX);
      dm9000[dm].data  = &__REG16 (DM1_PHYS_DATA);
# if defined (DM1_NAME)
      dm9000[dm].name = DM1_NAME;
# endif
      break;
#endif
    default:
      continue;
    }

#if C_DM > 1
    if (g_dm9000_default == -1)	/* Default interface for commands */
      g_dm9000_default = dm;
#endif

    DBG (2, "%s: [%d] index %p  data %p\n", __FUNCTION__, dm,
	 dm9000[dm].index, dm9000[dm].data);

    dm9000[dm].vendor  = read_reg (dm, DM9000_VIDL)
      | (read_reg (dm, DM9000_VIDH) << 8);
    dm9000[dm].product = read_reg (dm, DM9000_PIDL)
      | (read_reg (dm, DM9000_PIDH) << 8);
    dm9000[dm].chip    = read_reg (dm, DM9000_CHIPR);

    DBG (2, "%s: [%d] vendor %x  product %x  chip %x\n", __FUNCTION__, dm,
	 dm9000[dm].vendor, dm9000[dm].product, dm9000[dm].chip);

    if (dm9000[dm].vendor != 0xa46 || dm9000[dm].product != 0x9000)
      continue;

    dm9000[dm].present = 1;
    g_dm9000_present = 1;

    dm9000[dm].rgs_eeprom[0] = read_eeprom (dm, 0);
    dm9000[dm].rgs_eeprom[1] = read_eeprom (dm, 1);
    dm9000[dm].rgs_eeprom[2] = read_eeprom (dm, 2);

	/* Only the first interface can participate in the network stack */
    if (dm == 0) {
      host_mac_address[0] =  dm9000[dm].rgs_eeprom[0]       & 0xff;
      host_mac_address[1] = (dm9000[dm].rgs_eeprom[0] >> 8) & 0xff;
      host_mac_address[2] =  dm9000[dm].rgs_eeprom[1]       & 0xff;
      host_mac_address[3] = (dm9000[dm].rgs_eeprom[1] >> 8) & 0xff;
      host_mac_address[4] =  dm9000[dm].rgs_eeprom[2]       & 0xff;
      host_mac_address[5] = (dm9000[dm].rgs_eeprom[2] >> 8) & 0xff;
    }
  }
}


#if !defined (CONFIG_SMALL)
static void dm9000_report (void)
{
  int dm;

  if (!g_dm9000_present)
    return;

  printf ("  dm9000: host_mac_addr %02x:%02x:%02x:%02x:%02x:%02x\n",
	  host_mac_address[0], host_mac_address[1],
	  host_mac_address[2], host_mac_address[3],
	  host_mac_address[4], host_mac_address[5]);

  for (dm = 0; dm < C_DM; ++dm) {
    if (dm9000[dm].present) {
      printf ("  dm9000: [%d]"
//	      " phy_addr %d  phy_id 0x%lx"
	      "  mac_addr %02x:%02x:%02x:%02x:%02x:%02x"
	      " (0x%x 0x%x 0x%x) %s\n",
	      dm,
//	      -1, (unsigned long)-1,
	      (dm9000[dm].rgs_eeprom[0]) & 0xff,
	      (dm9000[dm].rgs_eeprom[0] >> 8) & 0xff,
	      (dm9000[dm].rgs_eeprom[1]) & 0xff,
	      (dm9000[dm].rgs_eeprom[1] >> 8) & 0xff,
	      (dm9000[dm].rgs_eeprom[2]) & 0xff,
	      (dm9000[dm].rgs_eeprom[2] >> 8) & 0xff,
	      dm9000[dm].vendor, dm9000[dm].product, dm9000[dm].chip,
	      dm9000[dm].name ? dm9000[dm].name : "");
    }
  }
}
#endif


static int dm9000_open (struct descriptor_d* d)
{
  write_reg (g_dm9000_default, DM9000_RCR, RCR_RXEN); /* Receive enable */
  write_reg (g_dm9000_default, DM9000_IMR, IMR_PAR);  /* Auto increemnet */

  /* FIXME: Make sure we're init'd */

  return 0;
}

/** Read packet from the DM9000 into system memory.  The header for
    the packet is a 0x01, STATUS, LENGTH_LOW, LENGTH_HIGH.

*/

static int dm9000_read (struct descriptor_d* d, void* pv, size_t cb)
{
  uint16_t status = read_reg (g_dm9000_default, DM9000_MRCMDX);
  size_t length;
  static char rgb[2048];
  int i;

  if ((status & 0xff) != 1)
    return 0;

  status        = read_reg (g_dm9000_default, DM9000_MRCMD);
  length = read_reg (g_dm9000_default, DM9000_MRCMD);

  for (i = 0; i < length; i += 2) {
    uint16_t v = read_reg (g_dm9000_default, DM9000_MRCMD);
    rgb[i] = v & 0xff;
    rgb[i + 1] = (v >> 8) & 0xff;
  }

  printf ("Received %d bytes (status 0x%x)\n", length, status);
  dumpw (rgb, length, 0, 0);

  return 0;
}

static int dm9000_write (struct descriptor_d* d, const void* pv, size_t cb)
{
  uint8_t* rgb = (uint8_t*) pv;
  int i;
  for (i = 0; i < cb; i += 2) {
    uint16_t v = rgb[i] | (rgb[i + 1] << 8);
    /* *** FIXME: should have a speedier write that doesn't require
       *** setup every time. */
    write_reg (g_dm9000_default, DM9000_MWCMD, v);
  }

	/* Tell DM9000 the size of the packet */
  write_reg (g_dm9000_default, DM9000_TXPLL, cb & 0xff);
  write_reg (g_dm9000_default, DM9000_TXPLL, (cb >> 8) & 0xff);

	/* Initiate transfer */
  write_reg (g_dm9000_default, DM9000_TCR,
             TCR_TXREQ
             | TCR_CRC_DIS1 | TCR_CRC_DIS2
             | TCR_PAD_DIS1 | TCR_PAD_DIS2);

  return 0;
}

static __driver_4 struct driver_d dm9000_driver = {
  .name = DRIVER_NAME,
  .description = "Davicom DM9000 Ethernet driver",
  .flags = DRIVER_NET,
  .open = dm9000_open,
  .close = close_helper,
  .read = dm9000_read,
  .write = dm9000_write,
};

static __service_6 struct service_d dm9000_service = {
  .init = dm9000_init,
  //  .release = dm9000_release,
#if !defined (CONFIG_SMALL)
  .report = dm9000_report,
#endif
};
