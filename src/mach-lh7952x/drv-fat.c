/* drv-fat.c
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

   FAT Filesystem Driver 

   o bootsector

     It might be better to read the boot sector than to read the
     partition table in case the user has a floppy drive.  Could do
     this, but we don't have the hardware, so why fuss with it now?

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

#if defined TALK
# define PRINTF(v...)	printf (v)
#else
# define PRINTF(v...)	do {} while (0)
#endif

#define ENTRY(l) PRINTF ("%s\n", __FUNCTION__)


#define FAT_READONLY	(1<<0)
#define FAT_HIDDEN	(1<<1)
#define FAT_SYSTEM	(1<<2)
#define FAT_VOLUME	(1<<3)
#define FAT_DIRECTORY	(1<<4)
#define FAT_ARCHIVE	(1<<5)

#define DRIVER_NAME	"fatfs"

#define SECTOR_SIZE	512

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

struct fat_info {
//  char bootsector[SECTOR_SIZE];	/* *** Superfluous */
  struct directory root[32];

  int fOK;			/* True when the block device recognizabled */
  struct partition partition[4];
  struct parameter parameter;	/* Parameter info for the partition */

  struct descriptor_d d;	/* Descriptor for underlying driver */
};

static struct fat_info fat;

//struct driver_d* fs_driver;	/* *** FIXME: underlying driver link hack */
const char szBlockDriver[] = "cf"; /* Underlying block driver */

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
       + (((unsigned long) pb[1]) <<  8);
       + (((unsigned long) pb[2]) << 16);
       + (((unsigned long) pb[3]) << 24);
}

static void fat_init (void)
{
  ENTRY (0);
}

static void fat_report (void);

/* fat_identify

   performs an initial read on the fat device to get partition
   information.

*/

static int fat_identify (void)
{
  int result;
  char sz[128];
  struct descriptor_d d;

  snprintf (sz, sizeof (sz), "%s:+1s", szBlockDriver);
  if (   (result = parse_descriptor (sz, &d))
      || (result = open_descriptor (&d))) 
    return result;


  /* Check for signature */
  {
    unsigned char rgb[2];

    d.driver->seek (&d, SECTOR_SIZE - 2, SEEK_SET);
    if (d.driver->read (&d, &rgb, 2) != 2
	|| rgb[0] != 0x55
	|| rgb[1] != 0xaa) {
      close_descriptor (&d);
      return -1;
    }
  }

  d.driver->seek (&d, SECTOR_SIZE - 66, SEEK_SET);
  d.driver->read (&d, fat.partition, 16*4);

#if 0
  d.driver->seek (&d, SECTOR_SIZE*32, SEEK_SET); /* Hard coded */
  d.driver->read (&d, &fat.parameter, sizeof (struct parameter));
  d.driver->seek (&d, SECTOR_SIZE*(32
				   + read_short (&fat.parameter
						 .reserved_sectors)
				   + fat.parameter.fats
				   *read_short (&fat.parameter
						.sectors_per_fat)), 
		  SEEK_SET);
  d.driver->read (&d, fat.root, sizeof (fat.root));
#endif

  close_descriptor (&d);

  return 0;
}

static int fat_open (struct descriptor_d* d)
{
  int result;
  char sz[128];

  ENTRY (0);

  fat.fOK = 0;
  if ((result = fat_identify ()))
    return result;
  fat.fOK = 1;

  /* ***  Based on the descriptor, do something interesting  */

#if 0
  d.driver->seek (&d, SECTOR_SIZE*32, SEEK_SET); /* Hard coded */
  d.driver->read (&d, &fat.parameter, sizeof (struct parameter));
  d.driver->seek (&d, SECTOR_SIZE*(32
				   + read_short (&fat.parameter
						 .reserved_sectors)
				   + fat.parameter.fats
				   *read_short (&fat.parameter
						.sectors_per_fat)), 
		  SEEK_SET);
  d.driver->read (&d, fat.root, sizeof (fat.root));
#endif



  printf ("descript %d %d\n", d->c, d->iRoot);
  {
    int i;
    for (i = 0; i < d->c; ++i)
      printf ("  %d: (%s)\n", i, d->pb[i]);
  }

		/* Partition table */
  if (d->c == 0) {
    snprintf (sz, sizeof (sz), "%s:%d+%d", 
	      szBlockDriver, 
	      SECTOR_SIZE - 66, 16*4);
    d->length = 16*4;
  }
  else
    snprintf (sz, sizeof (sz), "%s:%lds+%lds", 
	      szBlockDriver, 
	      fat.partition[0].start, fat.partition[0].length);

  if (   (result = parse_descriptor (sz, &fat.d))
      || (result = open_descriptor (&fat.d))) 
    return -1;

#if 0

  fat.d.driver->seek (&fat.d, SECTOR_SIZE - 66, SEEK_SET);
  fat.d.driver->read (&fat.d, fat.partition, 16*4);
  fat.d.driver->seek (&fat.d, SECTOR_SIZE*32, SEEK_SET); /* Hard coded */
  fat.d.driver->read (&fat.d, &fat.parameter, sizeof (struct parameter));
  fat.d.driver->seek (&fat.d, SECTOR_SIZE*(32
					   + read_short (&fat.parameter
							 .reserved_sectors)
					   + fat.parameter.fats
					   *read_short (&fat.parameter
							.sectors_per_fat)), 
		      SEEK_SET);
  fat.d.driver->read (&fat.d, fat.root, sizeof (fat.root));

  fat_report ();

  /* open file */
#endif

  return 0;
}

static void fat_close (struct descriptor_d* d)
{
  ENTRY (0);

  close_descriptor (&fat.d);
  close_helper (d);
}

static ssize_t fat_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cbRead;

  ENTRY (0);

  /* We may want to limit the range of the read */

  fat.d.driver->seek (&fat.d, d->index - d->start, SEEK_SET);
  cbRead = fat.d.driver->read (&fat.d, pv, cb);
  d->index += cbRead;
  return cbRead;
}

#if !defined (CONFIG_SMALL)

static void fat_report (void)
{
  int i;

  if (!fat.fOK) {
    if (fat_identify ())
      return;
    fat.fOK = 1;
  }

//  if (   fat.bootsector[SECTOR_SIZE - 2] == 0x55 
//      && fat.bootsector[SECTOR_SIZE - 1] == 0xaa) {
  printf ("  fat:\n"); 
    for (i = 0; i < 4; ++i)
      if (fat.partition[i].type)
	printf ("          partition %d: %c %02x 0x%08lx 0x%08lx\n", i, 
		fat.partition[i].boot ? '*' : ' ',
		fat.partition[i].type,
		fat.partition[i].start,
		fat.partition[i].length);

#if 0
    printf ("          bps %d spc %d res %d fats %d re %d sec %d\n", 
	    read_short (&fat.parameter.bytes_per_sector),
	    fat.parameter.sectors_per_cluster,
	    read_short (&fat.parameter.reserved_sectors),
	    fat.parameter.fats,
	    read_short (&fat.parameter.root_entries), 
	    read_short  (&fat.parameter.small_sectors));
    printf ("          med 0x%x spf %d spt %d heads %d hidden %ld sec %ld\n",
	    fat.parameter.media,
	    read_short (&fat.parameter.sectors_per_fat),
	    read_short (&fat.parameter.sectors_per_track),
	    read_short (&fat.parameter.heads),
	    read_long (&fat.parameter.hidden_sectors), 
	    read_long  (&fat.parameter.large_sectors));
    printf ("          log 0x%02x sig 0x%x serial %08lx vol %11.11s"
	    " type %8.8s\n",
	    read_short (&fat.parameter.logical_drive),
	    fat.parameter.signature,
	    read_long (&fat.parameter.serial),	    
	    fat.parameter.volume,
	    fat.parameter.type);

    for (i = 0; i < SECTOR_SIZE/sizeof (struct directory); ++i) {
      if (fat.root[i].attribute == 0xf) {
	int j;
	printf ("%12.12s0x%x ", "", fat.root[i].file[0]);
	for (j = 0; j < 14; ++j) {
	  char ch = 0;
	  switch (j) {
	  case 0 ... 5:   ch = ((char*) &fat.root[i])[ 1 -  0 + j*2]; break;
	  case 6 ... 11:  ch = ((char*) &fat.root[i])[14 - 12 + j*2]; break;
	  case 12 ... 13: ch = ((char*) &fat.root[i])[28 - 24 + j*2]; break;
	  }
	  if (!ch)
	    break;
	  printf ("%c", ch);
	}
	printf ("\n");
      }
      else if (fat.root[i].cluster == 0)
	continue;
      else
	printf ("%12.12s%-11.11s 0x%x #%d %ld\n",
		"",
		fat.root[i].file, 
		fat.root[i].attribute,
		fat.root[i].cluster, 
		fat.root[i].length); 
    }
#endif

    //  }
}

#endif

static __driver_3 struct driver_d fat_driver = {
  .name = DRIVER_NAME,
  .description = "FAT filesystem driver",
  .flags = DRIVER_DESCRIP_FS,
  .open = fat_open,
  .close = fat_close,
  .read = fat_read,
//  .write = cf_write,
//  .erase = cf_erase,
  .seek = seek_helper,
};

static __service_6 struct service_d fat_service = {
  .init = fat_init,
#if !defined (CONFIG_SMALL)
  .report = fat_report,
#endif
};
