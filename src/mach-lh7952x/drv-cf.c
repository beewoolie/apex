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

   CompactFlash flash IO driver for LPD79524.

   o Byte-Swapping

     Nothing has been done to be careful about byte swapping and
     endianness.  I believe that adding this should be simple enough
     given access to a big-endian machine.  The hard part is that the
     CF standard has some built-in ordering.

*/

#include <config.h>
#include <apex.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <spinner.h>

#include "hardware.h"
#include "compactflash.h"

#define TALK

#if CF_WIDTH == 16
#define REG __REG16
#endif

#define DRIVER_NAME	"cf-lpd79524"

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

#define USE_LBA

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

struct partition {
  unsigned char boot;
  unsigned char begin_chs[3];
  unsigned char type;
  unsigned char end_chs[3];
  unsigned long start;
  unsigned long length;
};

struct cf_info {
  int type;
  char szFirmware[9];		/* Card firmware revision */
  char szName[41];		/* Name of card */
  int cylinders;
  int heads;
  int sectors_per_track;
  int total_sectors;

  char bootsector[SECTOR_SIZE];	/* *** Superfluous */

  struct partition partition[4];

  char rgb[SECTOR_SIZE];	/* Sector buffer */
  int sector;			/* Buffered sector */
};

static struct cf_info cf_d; 

static unsigned char read8 (int reg)
{
  unsigned short v;
  v = REG (CF_PHYS | CF_REG | (reg & ~1)*CF_ADDR_MULT);
  return (reg & 1) ? (v >> 8) : (v & 0xff);
}

static void write8 (int reg, unsigned char value)
{
  unsigned short v;
  v = REG (CF_PHYS | CF_REG | (reg & ~1)*CF_ADDR_MULT);
  v = (reg & 1) ? ((v & 0x00ff) | (value << 8)) : ((v & 0xff00) | value);
  REG (CF_PHYS | CF_REG | (reg & ~1)*CF_ADDR_MULT) = v;
}

static unsigned short read16 (int reg)
{
  unsigned short value;
  value = REG (CF_PHYS | CF_REG | (reg & ~1)*CF_ADDR_MULT);
  return value;
}

static void write16 (int reg, unsigned short value)
{
  REG (CF_PHYS | CF_REG | (reg & ~1)*CF_ADDR_MULT) = value;
}


static void ready_wait (void)
{
  /* *** FIXME: need a timeout */
  while (read8 (IDE_STATUS) & (1<<7))
    ;
}

static void select (int drive, int head)
{
  write16 (IDE_SELECT, (0xa0) 
#if defined USE_LBA
	   | (1<<6)		/* Enable LBA mode */
#endif
	   | (drive ? (1<<4) : 0) 
	   | (IDE_IDLE << 8)); 

  ready_wait ();
}

static void seek (unsigned sector)
{
  unsigned head;
  unsigned cylinder;

#if defined (TALK)
  printf ("[%d", sector);
#endif

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

#if defined (TALK)
  printf (" %d %d %d]\n", head, cylinder, sector);
#endif
}

static void cf_init (void)
{
  cf_d.type = REG (CF_PHYS);

  if (cf_d.type != typeMemory)
    return;

  select (0, 0);
  ready_wait ();

  write8 (IDE_COMMAND, IDE_IDENTIFY);
  ready_wait ();

  {
    unsigned short rgs[128];
    int i;
    for (i = 0; i < 128; ++i)
      rgs[i] = read16 (IDE_DATA);

//    dump ((void*) rgs, 256, 0);

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

  {
    struct descriptor_d d;
    int result;
    if (   !(result = parse_descriptor (DRIVER_NAME ":+1s", &d))
	&& !(result = open_descriptor (&d))) {
      d.driver->read (&d, cf_d.bootsector, SECTOR_SIZE);
      d.driver->seek (&d, SECTOR_SIZE - 66, SEEK_SET);
      d.driver->read (&d, cf_d.partition, 16*4);
      close_descriptor (&d);
//      dump ((void*) cf_d.bootsector, 512, 0);
    }
  }
}

static int cf_open (struct descriptor_d* d)
{
  if (cf_d.type != typeMemory)
    return -1;

  /* perform bounds check */

  return 0;
}

static ssize_t cf_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cbRead = 0;

  if (d->index + cb > d->length)
    cb = d->length - d->index;

  
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
#if defined (TALK)
      printf ("cf reading %d\n", sector);
#endif
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
  printf ("  cf:     %d.%02dMiB, %s %s\n", 
	  (cf_d.total_sectors/2)/1024,
	  (((cf_d.total_sectors/2)%1024)*100)/1024,
	  cf_d.szFirmware, cf_d.szName);
  if (   cf_d.bootsector[SECTOR_SIZE - 2] == 0x55 
      && cf_d.bootsector[SECTOR_SIZE - 1] == 0xaa) {
    int i;
    for (i = 0; i < 4; ++i)
      if (cf_d.partition[i].type)
	printf ("          partition %d: %c %02x 0x%08lx 0x%08lx\n", i, 
		cf_d.partition[i].boot ? '*' : ' ',
		cf_d.partition[i].type,
		cf_d.partition[i].start,
		cf_d.partition[i].length);
  }
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
  .init = cf_init,
#if !defined (CONFIG_SMALL)
  .report = cf_report,
#endif
};
