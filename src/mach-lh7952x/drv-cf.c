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

   o FAT

     Some of the work here is to support FAT.  This will be moved to a
     driver on its own before it is fully functional.

   o Parameter Alignment

     The 3 byte padding in the parameter structure is added because it
     keeps the enclosing structure from misaligning.  This deserves
     more exploration.

*/

#include <config.h>
#include <apex.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <spinner.h>

#include "hardware.h"
#include "compactflash.h"

//#define TALK

#if defined TALK
# define PRINTF(v...)	printf (v)
#else
# define PRINTF(v...)	do {} while (0)
#endif

#define ENTRY(l) PRINTF ("%s\n", __FUNCTION__)


#if CF_WIDTH == 16
#define REG __REG16
#endif

#define FAT_READONLY	(1<<0)
#define FAT_HIDDEN	(1<<1)
#define FAT_SYSTEM	(1<<2)
#define FAT_VOLUME	(1<<3)
#define FAT_DIRECTORY	(1<<4)
#define FAT_ARCHIVE	(1<<5)

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

struct parameter {
  unsigned char jump[3];
  unsigned char oemname[8];
  unsigned short bytes_per_sector;
  unsigned char sectors_per_cluster;
  unsigned short reserved_sectors;
  unsigned char fats;
  unsigned short root_entries;
  unsigned short small_sectors;
  unsigned char media;
  unsigned short sectors_per_fat;
  unsigned short sectors_per_track;
  unsigned short heads;
  unsigned long hidden_sectors;
  unsigned long large_sectors;
  unsigned char logical_drive;
  unsigned char reserved;
  unsigned char signature;	/* Must be 0x29 */
  unsigned long serial;
  unsigned char volume[11];
  unsigned char type[8];
  char dummy[2];
} __attribute__ ((packed));

struct directory {
  unsigned char file[8];
  unsigned char extension[3];
  unsigned char attribute;
  unsigned char type;
  unsigned char checksum;
  unsigned char name2[8];
  unsigned short time;
  unsigned short date;
  unsigned short cluster;
  unsigned long length;
} __attribute__ ((packed));

struct cf_info {
  int type;
  char szFirmware[9];		/* Card firmware revision */
  char szName[41];		/* Name of card */
  int cylinders;
  int heads;
  int sectors_per_track;
  int total_sectors;

  //  char bootsector[SECTOR_SIZE];	/* *** Superfluous */
  //  struct directory root[32];

  //  struct partition partition[4];
  //  struct parameter parameter;	/* Parameter info for the partition */

  char rgb[SECTOR_SIZE];	/* Sector buffer */
  int sector;			/* Buffered sector */
};

static struct cf_info cf_d; 

static inline unsigned short read_short (void* pv)
{
  unsigned char* pb = (unsigned char*) pv;
  return  ((unsigned short) pb[0])
       + (((unsigned short) pb[1]) << 8);
}

static inline unsigned long read_long (void* pv)
{
  unsigned char* pb = (unsigned char*) pv;
  return  ((unsigned long) pb[0])
       + (((unsigned long) pb[1]) <<  8)
       + (((unsigned long) pb[2]) << 16)
       + (((unsigned long) pb[3]) << 24);
}

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

static int cf_identify (void)
{
  ENTRY (0);

  cf_d.type = REG (CF_PHYS);

#if 0
  {
    int index = 0;
    while (index < 1024) {
      unsigned short s = REG (CF_PHYS + index);
      if (s == 0xff)
	break;
      PRINTF ("%03x: 0x%x", index, s);
      index += 2*CF_ADDR_MULT;
      s = REG (CF_PHYS + index);
      PRINTF (" 0x%x (%d)\n", s, s);
      index += 2*CF_ADDR_MULT;
      {
	int i;
	unsigned char rgb[128];
	for (i = 0; i < s; ++i)
	  rgb[i] = REG (CF_PHYS + index + i*2*CF_ADDR_MULT);
	dump (rgb, s, 0);
      }
      index += s*2*CF_ADDR_MULT;
    }
  }
#endif

  if (cf_d.type != typeMemory)
    return 1;

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

    if (rgs[0] != 0x848a) {
      cf_d.type = -1;
      return 1;
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

#if 0
  /* *** FIXME: this cannot be done because it will cause infinite
     *** recursion.  */
  {
    struct descriptor_d d;
    int result;
    if (   !(result = parse_descriptor (DRIVER_NAME ":+1024s", &d))
	&& !(result = open_descriptor (&d))) {
      d.driver->read (&d, cf_d.bootsector, SECTOR_SIZE);
      d.driver->seek (&d, SECTOR_SIZE - 66, SEEK_SET);
      d.driver->read (&d, cf_d.partition, 16*4);
      d.driver->seek (&d, SECTOR_SIZE*32, SEEK_SET); /* Hard coded */
      d.driver->read (&d, &cf_d.parameter, sizeof (struct parameter));
      d.driver->seek (&d, SECTOR_SIZE*(32
				       + read_short (&cf_d.parameter
						     .reserved_sectors)
				       + cf_d.parameter.fats
				       *read_short (&cf_d.parameter
						    .sectors_per_fat)), 
		      SEEK_SET);
      d.driver->read (&d, cf_d.root, sizeof (cf_d.root));
      close_descriptor (&d);
//      dump ((void*) cf_d.bootsector, 512, 0);
//      dump ((void*) &cf_d.parameter, 25, 0);
    }


  }
#endif

  return 0;
}

static int cf_open (struct descriptor_d* d)
{
  ENTRY (0);

  if (cf_identify ())
    return -1;
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

//extern struct driver_d* fs_driver;
//struct driver_d lh79524_cf_driver;

static void cf_init (void)
{
  //  if (fs_driver == NULL)
  //    fs_driver = &lh79524_cf_driver;
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
#if 0
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
    printf ("          bps %d spc %d res %d fats %d re %d sec %d\n", 
	    read_short (&cf_d.parameter.bytes_per_sector),
	    cf_d.parameter.sectors_per_cluster,
	    read_short (&cf_d.parameter.reserved_sectors),
	    cf_d.parameter.fats,
	    read_short (&cf_d.parameter.root_entries), 
	    read_short  (&cf_d.parameter.small_sectors));
    printf ("          med 0x%x spf %d spt %d heads %d hidden %ld sec %ld\n",
	    cf_d.parameter.media,
	    read_short (&cf_d.parameter.sectors_per_fat),
	    read_short (&cf_d.parameter.sectors_per_track),
	    read_short (&cf_d.parameter.heads),
	    read_long (&cf_d.parameter.hidden_sectors), 
	    read_long  (&cf_d.parameter.large_sectors));
    printf ("          log 0x%02x sig 0x%x serial %08lx vol %11.11s"
	    " type %8.8s\n",
	    read_short (&cf_d.parameter.logical_drive),
	    cf_d.parameter.signature,
	    read_long (&cf_d.parameter.serial),	    
	    cf_d.parameter.volume,
	    cf_d.parameter.type);

    for (i = 0; i < SECTOR_SIZE/sizeof (struct directory); ++i) {
      if (cf_d.root[i].attribute == 0xf) {
	int j;
	printf ("%12.12s0x%x ", "", cf_d.root[i].file[0]);
	for (j = 0; j < 14; ++j) {
	  char ch = 0;
	  switch (j) {
	  case 0 ... 5:   ch = ((char*) &cf_d.root[i])[ 1 -  0 + j*2]; break;
	  case 6 ... 11:  ch = ((char*) &cf_d.root[i])[14 - 12 + j*2]; break;
	  case 12 ... 13: ch = ((char*) &cf_d.root[i])[28 - 24 + j*2]; break;
	  }
	  if (!ch)
	    break;
	  printf ("%c", ch);
	}
	printf ("\n");
      }
      else if (cf_d.root[i].cluster == 0)
	continue;
      else
	printf ("%12.12s%-11.11s 0x%x #%d %ld\n",
		"",
		cf_d.root[i].file, 
		cf_d.root[i].attribute,
		cf_d.root[i].cluster, 
		cf_d.root[i].length); 
    }
  }
#endif
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
