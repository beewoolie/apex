/* drv-cf.c
     $Id$

   written by Marc Singer
   7 Feb 2005

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

   Generic CompactFlash IO driver

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

*/

#include <config.h>
#include <apex.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <spinner.h>
#include <asm/reg.h>
#include <error.h>

#include <mach/drv-cf.h>

//#define TALK
//#define TALK_ATTRIB

#if defined TALK
# define PRINTF(v...)	printf (v)
#else
# define PRINTF(v...)	do {} while (0)
#endif

#define ENTRY(l) PRINTF ("%s\n", __FUNCTION__)

#if CF_WIDTH != 16
# error Unable to build CF driver with specified CF_WIDTH
#endif

#if CF_WIDTH == 16
# define REG __REG16
#endif

#define DRIVER_NAME	"cf-generic"

#define SECTOR_SIZE	512

#define IDE_IDENTIFY	0xec
#define IDE_IDLE	0xe1
#define IDE_READSECTOR	0x20

#define IDE_SECTORCOUNT	0x02
#define IDE_CYLINDER	0x04
#define IDE_SELECT	0x06
#define IDE_COMMAND	0x07
#define IDE_STATUS	0x07
#define IDE_DATA	0x08

  /* IO_BARRIER_READ necessary on some platforms where the chip select
     lines don't transition sufficiently.  It is necessary on reads as
     well as writes, however without a cache the code herein works
     without a read barrier because the instruction fetches intervene.
  */

#if defined (CF_IOBARRIER_PHYS)
# define IOBARRIER_READ	(*(volatile unsigned char*) CF_IOBARRIER_PHYS)
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
  typeMultifunction = 0,
  typeMemory = 1,
  typeSerial = 2,
  typeParallel = 3,
  typeDisk = 4,
  typeVideo = 5,
  typeNetwork = 6,
  typeAIMS = 7,
  typeSCSI = 8,
  typeSecurity = 9,
};

struct cf_info {
  int type;
  char szFirmware[9];		/* Card firmware revision */
  char szName[41];		/* Name of card */
  int cylinders;
  int heads;
  int sectors_per_track;
  int total_sectors;

  char rgb[SECTOR_SIZE];	/* Sector buffer */
  int sector;			/* Buffered sector */
};

static struct cf_info cf_d; 
u8 drive_select;

static unsigned char read8 (int reg)
{
  unsigned short v;
//  IOBARRIER_READ;
  v = REG (CF_PHYS | CF_REG | (reg & ~1)*CF_ADDR_MULT);
  IOBARRIER_READ;
  return (reg & 1) ? (v >> 8) : (v & 0xff);
}

static void write8 (int reg, unsigned char value)
{
  unsigned short v;
  v = REG (CF_PHYS | CF_REG | (reg & ~1)*CF_ADDR_MULT);
//  printf ("write8 0x%x -> 0x%x  ", 
//	  CF_PHYS | CF_REG | (reg & ~1)*CF_ADDR_MULT, v);
  IOBARRIER_READ;
  v = (reg & 1) ? ((v & 0x00ff) | (value << 8)) : ((v & 0xff00) | value);
//  printf (" 0x%x\n", v);
  DELAY;
  REG (CF_PHYS | CF_REG | (reg & ~1)*CF_ADDR_MULT) = v;
  IOBARRIER_READ;
}

static unsigned short read16 (int reg)
{
  unsigned short value;
//  IOBARRIER_READ;
  value = REG (CF_PHYS | CF_REG | (reg & ~1)*CF_ADDR_MULT);
  IOBARRIER_READ;
  return value;
}

static void write16 (int reg, unsigned short value)
{
//  IOBARRIER_READ;
//  printf ("write16: 0x%x <- 0x%x\n", 
//	  CF_PHYS | CF_REG | (reg & ~1)*CF_ADDR_MULT, value);
  REG (CF_PHYS | CF_REG | (reg & ~1)*CF_ADDR_MULT) = value;
  IOBARRIER_READ;
}


static void ready_wait (void)
{
  /* *** FIXME: need a timeout */
  while (read8 (IDE_STATUS) & (1<<7))
    ;
}

static void select (int drive, int head)
{
  drive_select = (0xa0) 
#if defined USE_LBA
    | (1<<6)		/* Enable LBA mode */
#endif
    | (drive ? (1<<4) : 0);

  write16 (IDE_SELECT, drive_select | (IDE_IDLE << 8)); 

  ready_wait ();
}

static void seek (unsigned sector)
{
  unsigned head;
  unsigned cylinder;

  PRINTF ("[%d", sector);

#if defined (USE_LBA)
  head      = (sector >> 24) & 0xf;
  cylinder  = (sector >> 8) & 0xffff;
  sector   &= 0xff;
#else  
  head     = sector/(cf_d.cylinders*cf_d.sectors_per_track);
  cylinder = (sector%(cf_d.cylinders*cf_d.sectors_per_track))
    /sectors_per_track;
  sector  %= sectors_per_track;
#endif

  select (0, head);
  write16 (IDE_SECTORCOUNT, 1 | (sector << 8));
  write16 (IDE_CYLINDER, cylinder);

  ready_wait ();

  PRINTF (" %d %d %d]\n", head, cylinder, sector);
}

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

static int cf_identify (void)
{
  ENTRY (0);

  drive_select = 0xe0;

  cf_d.type = REG (CF_PHYS);
  IOBARRIER_READ;

#if defined (TALK_ATTRIB)
  {
    int index = 0;
    while (index < 1024) {
      unsigned short s = REG (CF_PHYS + index);
      IOBARRIER_READ;
      if (s == 0xff)
	break;
      PRINTF ("%03x: 0x%x", index, s);
      index += 2*CF_ADDR_MULT;
      s = REG (CF_PHYS + index);
      IOBARRIER_READ;
      PRINTF (" 0x%x (%d)\n", s, s);
      index += 2*CF_ADDR_MULT;
      {
	int i;
	unsigned char rgb[128];
	for (i = 0; i < s; ++i) {
	  rgb[i] = REG (CF_PHYS + index + i*2*CF_ADDR_MULT);
	  IOBARRIER_READ;
	}
	dump (rgb, s, 0);
      }
      index += s*2*CF_ADDR_MULT;
    }
  }
#endif

  if (cf_d.type != typeMemory)
    ERROR_RETURN (ERROR_UNSUPPORTED, "unsupported CF device");

#if defined (TALK)
  talk_registers ();
#endif

  select (0, 0);
  ready_wait ();

//  write8 (IDE_COMMAND, IDE_IDENTIFY);
  write16 (IDE_SELECT, (IDE_IDENTIFY << 8) | drive_select);
  ready_wait ();

  {
    unsigned short rgs[128];
    int i;
    for (i = 0; i < 128; ++i)
      rgs[i] = read16 (IDE_DATA);
//      rgs[i] = read16 (0);

#if defined (TALK)
    dump ((void*) rgs, 256, 0);
#endif

    if (rgs[0] != 0x848a) {
      cf_d.type = -1;
      ERROR_RETURN (ERROR_IOFAILURE, "unexpected data read from CF card");
    }

    cf_d.cylinders         = rgs[1];
    cf_d.heads		   = rgs[3];
    cf_d.sectors_per_track = rgs[6];
    cf_d.total_sectors	   = (rgs[7] << 16) + rgs[8];

    for (i = 23; i < 27; ++i) {
      cf_d.szFirmware[(i - 23)*2]     =  rgs[i] >> 8;
      cf_d.szFirmware[(i - 23)*2 + 1] = (rgs[i] & 0xff);
    }
    for (i = 27; i < 47; ++i) {
      cf_d.szName[(i - 27)*2]     =  rgs[i] >> 8;
      cf_d.szName[(i - 27)*2 + 1] = (rgs[i] & 0xff);
    }
  }

  cf_d.sector = -1;

  return 0;
}

static int cf_open (struct descriptor_d* d)
{
  int result;
  ENTRY (0);

  if ((result = cf_identify ()))
    return result;

  /* perform bounds check */

  return 0;
}

static ssize_t cf_read (struct descriptor_d* d, void* pv, size_t cb)
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
    if (sector != cf_d.sector) {
      int i;

      PRINTF ("cf reading %d\n", sector);

      seek (sector);
      write8 (IDE_COMMAND, IDE_READSECTOR);
      ready_wait ();
      cf_d.sector = sector;

      for (i = 0; i < SECTOR_SIZE/2; ++i) 
	*(unsigned short*) &cf_d.rgb[i*2] = read16 (IDE_DATA);
    }
    
    memcpy (pv, cf_d.rgb + (index & (SECTOR_SIZE - 1)), available);

    d->index += available;
    cb -= available;
    cbRead += available;
    pv += available;
  }

  return cbRead;
}

#if !defined (CONFIG_SMALL)

static void cf_report (void)
{
  ENTRY (0);

  if (cf_identify ())
    return;

  printf ("  cf:     %d.%02dMiB, %s %s\n", 
	  (cf_d.total_sectors/2)/1024,
	  (((cf_d.total_sectors/2)%1024)*100)/1024,
	  cf_d.szFirmware, cf_d.szName);
}

#endif

static __driver_3 struct driver_d cf_driver = {
  .name = DRIVER_NAME,
  .description = "CompactFlash flash driver",
  .flags = DRIVER_WRITEPROGRESS(6),
  .open = cf_open,
  .close = close_helper,
  .read = cf_read,
//  .write = cf_write,
//  .erase = cf_erase,
  .seek = seek_helper,
};

static __service_6 struct service_d lpd79524_cf_service = {
//  .init = cf_init,
#if !defined (CONFIG_SMALL)
  .report = cf_report,
#endif
};
