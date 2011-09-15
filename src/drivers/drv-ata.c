/* drv-ata.c

   written by Marc Singer
   13 Sep 2011

   Copyright (C) 2005 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   Forked from CompactFlash driver.

   o Byte-Swapping

     Nothing has been done to be careful about byte swapping and
     endianness.  I believe that adding this should be simple enough
     given access to a big-endian machine.  The hard part is that the
     CF standard has some built-in ordering.

   o Parameter Alignment

     The 3 byte padding in the parameter structure is added because it
     keeps the enclosing structure from misaligning.  This deserves
     more exploration.

   o This driver can be complicated on platforms where access to the
     CF device is challenged by the hardware design.

     o The LPD boards tend to require an IO_BARRIER after writes to
       latch the written data.  The need for an IO_BARRIER on read is
       less clear.

     o The Sharp LH79524 has a feature where the address lines to 16
       bit accessed devices are shifted one, granting a larger address
       space, but also changing the alignment of address lines to the
       CF slot on the LPD SDK board.

     o Early LPD SDK boards drive the low address line which may
       confuse some CF cards.  The LPD behavior is not compliant.  It
       appears that SanDisk CF cards are tolerant of the behavior and
       will perform in spite of A0 being driven.  (IIRC)

   o 16 bit addressing.

     This driver presently only allows for 16 bit access to the CF
     slot.  Other bus widths will be implemented when the hardware is
     available.

   o Timeout on status read

     It is imperative that a timeout be implemented in the status
     read.  As the code stands, it will hang if the CF card
     identifies, but the interface fails to show a ready status.

   o Attributes and & 0xff

     The attribute memory is only byte-valid on even addresses.  We
     mask the value with 0xff to prevent problems on some cards that
     return the same value in the high and low bytes of the 16 bit
     read.

   o IO organization

     IDE interfaces may have one of two organizations.  We're starting
     with a simply sparse organization.  Each register is a byte
     (except for data) and each register is read & written 16 bits
     wide and only the low byte is significant.

   o Signature

     On reset, the device will load a value into the registers that
     identifies whether or not it supports packet commands.  According
     to the ATA documentation, this signature is in the registers
     SECTOR_COUNT, LBA_LOW, LBA_MID, LBA_HIGH, and DEVICE.  Packet
     cabable devices will show a signature of 0x1, 0x1, 0x14, 0xeb.
     Non-capable devices show a signature of 0x1, 0x1, 0x0, 0x0.

00000000: 0040 3ca5 c837 0010  0000 0000 003f 00ee  @..<7... ....?...
00000010: c9b0 0000 2020 2020  2044 4a5a 3037 3134  ....     D ZJ7041
00000020: 3130 3230 3035 3138  0000 0000 0000 5353  01025081 ......SS
00000030: 4420 352e 3230 5361  6e44 6973 6b20 7053   D.502aS Dnsi kSp
00000040: 5344 2d50 3220 3847  4220 2020 2020 2020  DSP- 2G8  B      
00000050: 2020 2020 2020 2020  2020 2020 2020 8001                 ..
00000060: 0000 2300 4000 0200  0000 0007 3ca5 0010  ...#.@.. .....<..
00000070: 003f c9b0 00ee 0000  c9b0 00ee 0000 0007  ?....... ........
00000080: 0003 0078 0078 0078  0078 0000 0000 0000  ..x.x.x. x.......
00000090: 0000 0000 0000 0000  0000 0000 0000 0040  ........ ......@.
000000a0: 01f0 0028 746b 7401  4102 7429 b401 4102  ..(.kt.t .A)t...A
000000b0: 007f 0000 0000 0000  fffe 6001 0000 0000  ........ ...`....
000000c0: 0000 0000 0000 0000  c9b0 00ee 0000 0000  ........ ........
000000d0: 0000 0000 0000 0000  5001 b442 fd28 2546  ........ .PB.(.F%
000000e0: 0000 0000 0000 0000  0000 0000 0000 4000  ........ .......@
000000f0: 4000 0000 0000 0000  0000 0000 0000 0000  .@...... ........
00000100: 0001 0000 0000 0000  0000 0000 0000 0000  ........ ........
00000110: 0000 0000 0000 0000  0000 0000 0000 0000  ........ ........
00000120: 0000 0000 0000 0000  0000 0000 0000 0000  ........ ........
00000130: 0000 0000 0000 0000  0000 0000 0000 0000  ........ ........
00000140: 0000 0000 0000 0000  0000 0000 0000 0000  ........ ........
00000150: 0004 0000 0000 0000  0000 0000 0000 0000  ........ ........
00000160: 0000 0000 0000 0000  0000 0000 0000 0000  ........ ........
00000170: 0000 0000 0000 0000  0000 0000 0000 0000  ........ ........
00000180: 0000 0000 0000 0000  0000 0000 0000 0000  ........ ........
00000190: 0000 0000 0000 0000  0000 0000 0000 0000  ........ ........
000001a0: 0000 0000 0000 0000  0000 0000 0000 0000  ........ ........
000001b0: 0000 0001 0000 0000  0000 0000 0001 0000  ........ ........
000001c0: 0000 0000 0000 0000  0000 0000 0000 0000  ........ ........
000001d0: 0000 0000 0000 0000  0000 0000 0000 0000  ........ ........
000001e0: 0000 0000 0000 0000  0000 0000 0000 0000  ........ ........
000001f0: 0000 0000 0000 0000  0000 0000 0000 7ea5  ........ .......~


*/

#include <config.h>
#include <apex.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <spinner.h>
#include <asm/reg.h>
#include <error.h>
#include <command.h>

#include <mach/drv-ata.h>

//#define TALK
//#define TALK_ATTRIB

#if defined TALK
# define PRINTF(v...)	printf (v)
#else
# define PRINTF(v...)	do {} while (0)
#endif

#define ENTRY(l) PRINTF ("%s\n", __FUNCTION__)

#if ATA_WIDTH != 16
# error Unable to build CF driver with specified ATA_WIDTH
#endif

#if ATA_WIDTH == 16
# define REG __REG16
#endif

#define DRIVER_NAME	"ata-generic"

#define SECTOR_SIZE	512

#define ATA_CMD_IDENTIFY	0xec
#define ATA_CMD_IDLE            0xe1
#define ATA_CMD_READSECTOR	0x20


//#define ATA_CMD_READ    0x20    /* Read Sectors (with retries)  */
//#define ATA_CMD_READN   0x21    /* Read Sectors ( no  retries)  */
//#define ATA_CMD_WRITE   0x30    /* Write Sectores (with retries)*/
//#define ATA_CMD_WRITEN  0x31    /* Write Sectors  ( no  retries)*/
//#define ATA_CMD_VRFY    0x40    /* Read Verify  (with retries)  */
//#define ATA_CMD_VRFYN   0x41    /* Read verify  ( no  retries)  */
//#define ATA_CMD_SEEK    0x70    /* Seek                         */
//#define ATA_CMD_DIAG    0x90    /* Execute Device Diagnostic    */
//#define ATA_CMD_INIT    0x91    /* Initialize Device Parameters */
//#define ATA_CMD_RD_MULT 0xC4    /* Read Multiple                */
//#define ATA_CMD_WR_MULT 0xC5    /* Write Multiple               */
//#define ATA_CMD_SETMULT 0xC6    /* Set Multiple Mode            */
//#define ATA_CMD_RD_DMA  0xC8    /* Read DMA (with retries)      */
//#define ATA_CMD_RD_DMAN 0xC9    /* Read DMS ( no  retries)      */
//#define ATA_CMD_WR_DMA  0xCA    /* Write DMA (with retries)     */
//#define ATA_CMD_WR_DMAN 0xCB    /* Write DMA ( no  retires)     */
//#define ATA_CMD_IDENT   0xEC    /* Identify Device              */
//#define ATA_CMD_SETF    0xEF    /* Set Features                 */
//#define ATA_CMD_CHK_PWR 0xE5    /* Check Power Mode             */


#define STATUS_BUSY  (1<<7)
#define STATUS_DRDY  (1<<6)
#define STATUS_FAULT (1<<5)
#define STATUS_SEEK  (1<<4)
#define STATUS_DRQ   (1<<3)
#define STATUS_CORR  (1<<2)
#define STATUS_INDEX (1<<1)
#define STATUS_ERR   (1<<0)

//#define IDE_SECTORCOUNT	0x02
//#define IDE_CYLINDER	0x04
//#define IDE_SELECT	0x06
//#define IDE_COMMAND	0x07
//#define IDE_STATUS	0x07
//#define IDE_DATA	0x08
//#define IDE_CONTROL	0x0e

#define ATA_DATA(x)           ATA_PHYS + ATA_ALT_BASE + ((x)*ATA_ADDR_MULT)
#define ATA_REG(x)            ATA_PHYS + ATA_REG_BASE + ((x)*ATA_ADDR_MULT)
//#define ATA_ALT(x)            ATA_PHYS + ATA_ALT + ((x)*ATA_ADDR_MULT)

#define ATA_REG_DATA          0
#define ATA_REG_ERROR         1 /* read */
#define ATA_REG_FEATURES      1 /* write */
#define ATA_REG_SECTOR_COUNT  2
#define ATA_REG_SECTOR_NUMBER 3
#define ATA_REG_CYLINDER_LOW  4
#define ATA_REG_CYLINDER_HIGH 5
#define ATA_REG_SELECT        6
#define ATA_REG_STATUS        7 /* read */
#define ATA_REG_COMMAND       7 /* write */
#define ATA_REG_ALT_STATUS    8 /* read */
#define ATA_REG_DEVICE_CTRL   8 /* write */
//#define ATA_REG_DATA_EVEN     8
//#define ATA_REG_DATA_ODD      9
//#define ATA_ALT_DEVCTL        6
#define ATA_REG_LBA_LOW       ATA_REG_SECTOR_NUMBER
#define ATA_REG_LBA_MID       ATA_REG_CYLINDER_LOW
#define ATA_REG_LBA_HIGH      ATA_REG_CYLINDER_HIGH
#define ATA_LBA_SELECT        ATA_REG_DEVCTL

#define ATTRIB_OPTION		0x00
#define ATTRIB_OPTION_LEVELREQ	(1<<6)
#define ATTRIB_CONFIGSTATUS	0x02
#define ATTRIB_PIN		0x04
#define ATTRIB_PIN_READY	(1<<1)


#define MS_READY_WAIT_TIMEOUT	(1*1000)

  /* IO_BARRIER_READ necessary on some platforms where the chip select
     lines don't transition sufficiently.  It is necessary on reads as
     well as writes, however without a cache the code herein works
     without a read barrier because the instruction fetches intervene.
  */

#if defined (ATA_IOBARRIER_PHYS)
# define IOBARRIER_READ	(*(volatile unsigned char*) ATA_IOBARRIER_PHYS)
#else
# define IOBARRIER_READ
#endif

#define USE_LBA

  /* This sleep was added for the sake of the reading a particular
     card on the 79520.  It isn't clear why it would be necessary as
     the 79524 has no problems and other cards are OK.  Better to
     perform the sleep and waste loads of time figuring out precise
     timings...for now.  Unfortunately, this delay make the IO
     unbearably slow.  Oh well. */
//#define DELAY usleep(1)
#define DELAY

enum {
  typeUndefined    = 0,
  typeIDE          = 1,
  typeATA          = 2,
  typeCompactFlash = 3,
};

enum {
  cftypeMultifunction = 0,
  cftypeMemory = 1,
  cftypeSerial = 2,
  cftypeParallel = 3,
  cftypeDisk = 4,
  cftypeVideo = 5,
  cftypeNetwork = 6,
  cftypeAIMS = 7,
  cftypeSCSI = 8,
  cftypeSecurity = 9,
};

struct ata_info {
  int type;                     /* true when CompactFlash device */
  int cf_type;
  uint32_t signature;
  uint16_t configuration;       /* Specific config from identity record */
  uint16_t capabilities;
  char szFirmware[9];		/* Card firmware revision */
  char szName[41];		/* Name of card */
  int cylinders;
  int heads;
  int sectors_per_track;
  int total_sectors;
  int speed;			/* Timing value from CF info */

  /* *** FIXME: buffer should be in .xbss section */
  char rgb[SECTOR_SIZE];	/* Sector buffer */
  int sector;			/* Buffered sector */
};

static struct ata_info ata_d;
u8 drive_select;

static inline void clear_info (void)
{
  memset (&ata_d, 0, sizeof (ata_d));
  ata_d.sector = -1;
  ata_d.speed  = -1;
}

static uint8_t read8 (int reg)
{
  uint16_t v = REG (ATA_PHYS | ATA_REG_BASE | reg*ATA_ADDR_MULT);
//  printf ("read reg %08x -> %x\n",
//          ATA_PHYS | ATA_REG_BASE | reg*ATA_ADDR_MULT,
//          v & 0xff);
  return v & 0xff;
}

static void write8 (int reg, uint8_t v)
{
//  printf ("write8 0x%x <- 0x%x\n",
//          ATA_PHYS | ATA_REG_BASE | reg*ATA_ADDR_MULT, v);
  REG (ATA_PHYS | ATA_REG_BASE | reg*ATA_ADDR_MULT) = v;
}


#if 0
static unsigned char reada8 (int reg)
{
  unsigned short v;
  v = REG (ATA_PHYS | ATA_ATTR_BASE | (reg & ~1)*ATA_ADDR_MULT);
  IOBARRIER_READ;
  return (reg & 1) ? (v >> 8) : (v & 0xff);
}
#endif

#if 0
static void writea8 (int reg, unsigned char value)
{
  unsigned short v;
  v = REG (ATA_PHYS | ATA_ATTRIB | (reg & ~1)*ATA_ADDR_MULT);
  IOBARRIER_READ;
  v = (reg & 1) ? ((v & 0x00ff) | (value << 8)) : ((v & 0xff00) | value);
  DELAY;
  REG (ATA_PHYS | ATA_ATTRIB | (reg & ~1)*ATA_ADDR_MULT) = v;
  IOBARRIER_READ;
}
#endif

static uint16_t read16 (int reg)
{
  uint16_t value;
  value = REG (ATA_PHYS | ATA_REG_BASE | reg*ATA_ADDR_MULT);
  return value;
}

#if 0
static void write16 (int reg, unsigned short value)
{
  printf ("write16: 0x%x <- 0x%x\n",
	  ATA_PHYS | ATA_REG_BASE | reg*ATA_ADDR_MULT, value);
  REG (ATA_PHYS | ATA_REG_BASE | reg*ATA_ADDR_MULT) = value;
}
#endif


static uint8_t status_wait (void)
{
  time_t start = timer_read ();
  uint8_t v;
  while ((v = read8 (ATA_REG_STATUS)) & STATUS_BUSY)
    if (timer_delta (start, timer_read ()) >= MS_READY_WAIT_TIMEOUT)
      break;
  return v;
}

#if 0
static void ready_wait (void)
{
  unsigned long time = timer_read ();

  while ((reada8 (ATTRIB_PIN) & ATTRIB_PIN_READY) == 0
         && timer_delta (time, timer_read ()) < MS_READY_WAIT_TIMEOUT)
    ;

//  while (read8 (IDE_STATUS) & (1<<7))
//    ;
}
#endif

#if defined (TALK)
static void talk_registers (void)
{
  printf ("ide: sec %04x cyl %04x stat %04x data %04x\n",
	  read16 (IDE_SECTORCOUNT),
	  read16 (IDE_CYLINDER),
	  read16 (IDE_SELECT),
	  read16 (IDE_DATA));
}
#endif

const char* describe_capabilities (uint32_t cap)
{
  static char sz[64];
  snprintf (sz, sizeof (sz), "%s%s%s%s%s",
            (cap & (1<<8)) ? " DMA" : "",
            (cap & (1<<9)) ? " LBA" : "",
            (cap & (1<<10)) ? " IORdyDis" : "",
            (cap & (1<<11)) ? " IORdy" : "",
            (cap & (1<<13)) ? " STDBY_SUP" : " STDBY_DEV");
  return sz;
}

static inline bool can_lba (void)
{
  return ata_d.capabilities & (1<<9);
}

static void select (int drive)
{
  drive_select = (0xa0)
    | (can_lba () ? (1<<6) : 0)
    | (drive ? (1<<4) : 0);

  write8 (ATA_REG_SELECT, drive_select);
  status_wait ();
}

static void seek (unsigned sector)
{
  unsigned head;
  unsigned cylinder;

  PRINTF ("[s %d", sector);

  if (can_lba ()) {
    head      = (sector >> 24) & 0xf;
    cylinder  = (sector >> 8) & 0xffff;
    sector   &= 0xff;
  }
  else {
    head     = sector/(ata_d.cylinders*ata_d.sectors_per_track);
    cylinder = (sector%(ata_d.cylinders*ata_d.sectors_per_track))
      /ata_d.sectors_per_track;
    sector  %= ata_d.sectors_per_track;
  }

  select (0);
  write8 (ATA_REG_SECTOR_COUNT, 1);
  write8 (ATA_REG_SECTOR_NUMBER, sector);
  write8 (ATA_REG_CYLINDER_LOW,   cylinder       & 0xff);
  write8 (ATA_REG_CYLINDER_HIGH, (cylinder >> 8) & 0xff);

  status_wait ();

  PRINTF (" %d %d %d]\n", head, cylinder, sector);

#if defined (TALK)
  talk_registers ();
#endif
}

static void ata_init (void)
{
  ATA_CONTROL      = 0x80;
  ATA_CONTROL      = 0xc0;

  ATA_TIME_OFF     = 3;
  ATA_TIME_ON      = 3;
  ATA_TIME_1       = 2;
  ATA_TIME_2W      = 5;
  ATA_TIME_2R      = 5;
  ATA_TIME_AX      = 6;
  ATA_TIME_PIO_RDX = 1;
  ATA_TIME_4       = 1;
  ATA_TIME_9       = 1;

  ATA_CONTROL      = 0x41;
  ATA_FIFO_ALARM   = 20;
}

static int ata_identify (void)
{
  int status;

  ENTRY (0);

  ata_init ();

  write8 (ATA_REG_SELECT, 0xe0);

  write8 (ATA_REG_DEVICE_CTRL, 0x4); /* SRST */
  msleep (10);
  write8 (ATA_REG_DEVICE_CTRL, 0);
  clear_info ();

  if ((status = status_wait ()) & STATUS_BUSY) {
    printf ("** timeout (0x%x)\n", status);
    ERROR_RETURN (ERROR_UNSUPPORTED, "timeout");
  }

  ata_d.signature = 0
    | (read8 (ATA_REG_SECTOR_COUNT) << 24)
    | (read8 (ATA_REG_LBA_LOW)      << 16)
    | (read8 (ATA_REG_LBA_MID)      <<  8)
    | (read8 (ATA_REG_LBA_HIGH)     <<  0);

  switch (ata_d.signature) {
  case 0x01010000:
    ata_d.type = typeIDE;
    break;
  case 0x010113eb:
    ata_d.type = typeATA;
    break;
  default:
    printf ("unrecognized device signature 0x%x\n", ata_d.signature);
    ERROR_RETURN (ERROR_UNSUPPORTED, "unsupported device");
  }

  write8 (ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

  if ((status = status_wait ()) & STATUS_BUSY) {
    printf ("** timeout (0x%x)\n", status);
    ERROR_RETURN (ERROR_UNSUPPORTED, "timeout");
  }

  if ((status & STATUS_DRDY) == 0) {
    printf ("** failed to identify\n");
    ERROR_RETURN (ERROR_UNSUPPORTED, "failed to identify");
  }

  {
    uint16_t rgs[256];
    int i;
    for (i = 0; i < sizeof (rgs)/sizeof (*rgs); ++i)
      rgs[i] = read16 (ATA_REG_DATA);

    ata_d.configuration = rgs[2];
    switch (ata_d.configuration) {
    case 0x37c8:                /* SET FEATURES;    incomplete */
      break;
    case 0x73c8:                /* SET FEATURES;    complete */
      break;
    case 0x8c73:                /* No SET FEATURES; incomplete */
      break;
    case 0xc837:                /* No SET FEATURES; complete */
      break;
    case 0x838a:                /* CompactFlash */
      ata_d.type = typeCompactFlash;
      break;
    }

    for (i = 23; i < 27; ++i) {
      ata_d.szFirmware[(i - 23)*2]     =  rgs[i] >> 8;
      ata_d.szFirmware[(i - 23)*2 + 1] = (rgs[i] & 0xff);
    }
    for (i = 27; i < 47; ++i) {
      ata_d.szName[(i - 27)*2]     =  rgs[i] >> 8;
      ata_d.szName[(i - 27)*2 + 1] = (rgs[i] & 0xff);
    }

    ata_d.capabilities = (rgs[50] << 16) | rgs[49];

    switch (ata_d.type) {
    case typeIDE:
    case typeATA:
      ata_d.total_sectors = (rgs[61] << 16) | rgs[60];
      break;

    case typeCompactFlash:
	/* Fetch CF organization */
      if (rgs[53] & (1<<1)) {
        ata_d.cylinders		= rgs[54];
        ata_d.heads		= rgs[55];
        ata_d.sectors_per_track	= rgs[56];
        ata_d.total_sectors	= (rgs[58] << 16) + rgs[57];
      }
      else {
        ata_d.cylinders		= rgs[1];
        ata_d.heads		= rgs[3];
        ata_d.sectors_per_track	= rgs[6];
        ata_d.total_sectors	= (rgs[7] << 16) + rgs[8];
      }

      if (rgs[53] & 1)
        ata_d.speed = rgs[67];
      break;
    }
  }

  return 0;
}

static int ata_open (struct descriptor_d* d)
{
  int result;
  ENTRY (0);

  if ((result = ata_identify ()))
    return result;

  PRINTF ("%s: opened %ld %ld\n", __FUNCTION__, d->start, d->length);

  /* perform bounds check */

  return 0;
}

static ssize_t ata_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cbRead = 0;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  PRINTF ("%s: @ 0x%lx\n", __FUNCTION__, d->start + d->index);

  while (cb) {
    unsigned long index = d->start + d->index;
    int availableMax = SECTOR_SIZE - (index & (SECTOR_SIZE - 1));
    int available = cb;
    int sector = index/SECTOR_SIZE;

    if (available > availableMax)
      available = availableMax;

		/* Read sector into buffer  */
    if (sector != ata_d.sector) {
      int i;

      PRINTF ("ata reading %d\n", sector);

      seek (sector);
      write8 (ATA_REG_COMMAND, ATA_CMD_READSECTOR);
      status_wait ();
      ata_d.sector = sector;

      for (i = 0; i < SECTOR_SIZE/2; ++i)
	*(unsigned short*) &ata_d.rgb[i*2] = read16 (ATA_REG_DATA);
    }

    memcpy (pv, ata_d.rgb + (index & (SECTOR_SIZE - 1)), available);

    d->index += available;
    cb -= available;
    cbRead += available;
    pv += available;
  }

  return cbRead;
}

#if !defined (CONFIG_SMALL)

static void ata_report (void)
{
  ENTRY (0);

  if (ata_identify ())
    return;

  printf ("  ata:    %d.%02dMiB, %s %s",
	  (ata_d.total_sectors/2)/1024,
	  (((ata_d.total_sectors/2)%1024)*100)/1024,
	  ata_d.szFirmware, ata_d.szName);
  if (ata_d.speed != -1)
    printf (" (%d ns)", ata_d.speed);
  printf ("\n");

#if defined (TALK)
  printf ("          cyl %d heads %d spt %d total %d\n",
	  ata_d.cylinders, ata_d.heads, ata_d.sectors_per_track,
	  ata_d.total_sectors);
#endif
}

#endif

static __driver_3 struct driver_d ata_driver = {
  .name        = DRIVER_NAME,
  .description = "ATA driver",
  .flags       = DRIVER_WRITEPROGRESS(6),
  .open        = ata_open,
  .close       = close_helper,
  .read        = ata_read,
  .seek        = seek_helper,
};

static __service_6 struct service_d ata_service = {
#if !defined (CONFIG_SMALL)
  .name        = DRIVER_NAME,
  .description = "ATA service",
  .report      = ata_report,
#endif
};


static int cmd_ata (int argc, const char** argv)
{
  uint8_t status;

  ata_init ();

  write8 (ATA_REG_SELECT, 0xe0);

  write8 (ATA_REG_DEVICE_CTRL, 0x4); /* SRST */
  msleep (10);
  write8 (ATA_REG_DEVICE_CTRL, 0);
  clear_info ();

  if ((status = status_wait ()) & STATUS_BUSY) {
    printf ("** timeout (0x%x)\n", status);
    return 0;
  }

  ata_d.signature = 0
    | (read8 (ATA_REG_SECTOR_COUNT) << 24)
    | (read8 (ATA_REG_LBA_LOW)      << 16)
    | (read8 (ATA_REG_LBA_MID)      <<  8)
    | (read8 (ATA_REG_LBA_HIGH)     <<  0);

  if (ata_d.signature == 0x01010000)
    ata_d.type = typeIDE;
  else if (ata_d.signature == 0x010113eb)
    ata_d.type = typeATA;
  else {
    printf ("unrecognized device signature 0x%x\n", ata_d.signature);
  }

  write8 (ATA_REG_COMMAND, ATA_CMD_IDENTIFY);

  if ((status = status_wait ()) & STATUS_BUSY) {
    printf ("** timeout (0x%x)\n", status);
    return 0;
  }

  if ((status & STATUS_DRDY) == 0) {
    printf ("** failed to identify\n");
    return 0;
  }

  {
    uint16_t rgs[256];
    int i;
    for (i = 0; i < sizeof (rgs)/sizeof (*rgs); ++i)
      rgs[i] = read16 (ATA_REG_DATA);

    ata_d.configuration = rgs[2];
    switch (ata_d.configuration) {
    case 0x37c8:                /* SET FEATURES;    incomplete */
      break;
    case 0x73c8:                /* SET FEATURES;    complete */
      break;
    case 0x8c73:                /* No SET FEATURES; incomplete */
      break;
    case 0xc837:                /* No SET FEATURES; complete */
      break;
    case 0x838a:                /* CompactFlash */
      ata_d.type = typeCompactFlash;
      break;
    }

    for (i = 23; i < 27; ++i) {
      ata_d.szFirmware[(i - 23)*2]     =  rgs[i] >> 8;
      ata_d.szFirmware[(i - 23)*2 + 1] = (rgs[i] & 0xff);
    }
    for (i = 27; i < 47; ++i) {
      ata_d.szName[(i - 27)*2]     =  rgs[i] >> 8;
      ata_d.szName[(i - 27)*2 + 1] = (rgs[i] & 0xff);
    }

    ata_d.capabilities = (rgs[50] << 16) | rgs[49];

    switch (ata_d.type) {
    case typeIDE:
    case typeATA:
      ata_d.total_sectors = (rgs[61] << 16) | rgs[60];
      break;

    case typeCompactFlash:
	/* Fetch CF organization */
      if (rgs[53] & (1<<1)) {
        ata_d.cylinders		= rgs[54];
        ata_d.heads		= rgs[55];
        ata_d.sectors_per_track	= rgs[56];
        ata_d.total_sectors	= (rgs[58] << 16) + rgs[57];
      }
      else {
        ata_d.cylinders		= rgs[1];
        ata_d.heads		= rgs[3];
        ata_d.sectors_per_track	= rgs[6];
        ata_d.total_sectors	= (rgs[7] << 16) + rgs[8];
      }

      if (rgs[53] & 1) {
        ata_d.speed = rgs[67];
      }
      else ata_d.speed = -1;

      break;
    }

  }

  printf ("conf 0x%04x  cap 0x%08x (%s) total_sect %d\n",
          ata_d.configuration, ata_d.capabilities,
          describe_capabilities (ata_d.capabilities),
          ata_d.total_sectors);

  return 0;
}

static __command struct command_d c_ata = {
  .command = "ata",
  .func = cmd_ata,
  COMMAND_DESCRIPTION ("test ATA")
};
