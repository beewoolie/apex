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

   FAT Filesystem Driver.  This driver should be platform independent.
   The only hook to the rest of the system is the name of the driver
   that handles the underlying I/O.

   o bootsector

     It might be better to read the boot sector than to read the
     partition table in case the user has a floppy drive.  Could do
     this, but we don't have the hardware, so why fuss with it now?

   o seek

     will require that we catch the seeking call so that seeking from
     an arbitrary point in the file can be accomplished.  Really,
     we're not going to do it, so we might make it illegal...or at
     least challenge it.

     Seeking from the end of the file will be very expensive if the
     target isn't in the last cluster.  Either we need to keep a list
     of clusters as long as the offset/cluster_size, or we have to
     fail to do the right thing.

   o FAT caching

     one FAT sector is cached to make it somewhat efficient to read
     through a file.  

   o Filename handling

     It is cheaper, overall to put the user's filename in the same
     form as the directory entries that it is to fixup each entry as
     it is read.  However, we don't really care how long it takes to
     find a file.

   o SECTOR_SIZE and parameter.bytes_per_sector

     We cannot handle a sector size that isn't 512 bytes.  So, for the
     time being, the parameter entry for the size of a sector is
     ignored.

   o FAT formats

     Only FAT16 is supported.  Others may be, but only when we have a
     sample to test.

   o FILE I/O

     Information about the current file is cached in the fat info
     structure.  Thus precluding operations on two files.  Be warned.

*/

#include <config.h>
#include <apex.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <linux/ctype.h>
#include <spinner.h>
#include <linux/kernel.h>
#include <error.h>

//#define TALK

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
//  struct directory root[32];

  int fOK;			/* True when the block device recognizabled */
  struct partition partition[4];
  struct parameter parameter;	/* Parameter info for the partition */
  size_t index_cluster_2;	/* Partition index of first cluster */
  size_t bytes_per_cluster;	/* Precomputed cluster size */

  struct descriptor_d d;	/* Descriptor for underlying driver */

  /* *** FIXME: directory clusters not yet implemented */
//unsigned cluster_dir;		/* Current directory cluster being read  */

  char fat[SECTOR_SIZE];	/* Cached FAT sector  */
  int sector_fat;		/* Sector number of the cached FAT */

  struct directory file;	/* Directory entry for the current file */
  unsigned cluster_file; 	/* Current file cluster being read */
  size_t index_cluster_file;	/* Index at base of the cluster */
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

#if 0
static void fat_init (void)
{
  ENTRY (0);
}
#endif

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

  close_descriptor (&d);

  return 0;
}


/* fat_next_cluster 
   
   performs a read of the FAT to determine the next cluster in the
   chain.

   *** FIXME: this implementation only works for FAT16.

*/

static unsigned fat_next_cluster (unsigned cluster)
{
  int sector = fat.parameter.reserved_sectors + cluster/(SECTOR_SIZE/2);

  if (sector != fat.sector_fat) {
    size_t index = fat.d.index;	/* *** FIXME: This is a cheat */
    fat.d.driver->seek (&fat.d, SECTOR_SIZE*sector, SEEK_SET);
    fat.d.driver->read (&fat.d, &fat.fat, SECTOR_SIZE);
    fat.sector_fat = sector;
    fat.d.index = index;	/* *** FIXME: This is a cheat */
  }
  return read_short (fat.fat + (cluster%(SECTOR_SIZE/2))*2);
}


/* fat_find

   is a good part of the meat of this driver.  It takes a descriptor,
   fat.d having been opened on the partition, and the parameter block
   read from the partition and it find the directory entry of the
   indicated file.  It returns an error code.

   The root directory is simple, a contiguous sequence of directory
   entries.  Subdirectories are themselves files, so the traversal is
   a little more complex.

   Storing of the file information is a side-effect of this call.  The
   returned cluster number is, really, just informative.  

*/

static int fat_find (struct descriptor_d* d)
{
  int i;

	/* Start reading the root directory */
  fat.d.driver->seek (&fat.d, 
		      (fat.parameter.sectors_per_fat*fat.parameter.fats
		       + fat.parameter.reserved_sectors)*SECTOR_SIZE, 
		      SEEK_SET);
  for (i = 0; i < fat.parameter.root_entries; ++i) {
    char sz[12];
    int cbRead = fat.d.driver->read (&fat.d, &fat.file, sizeof (fat.file));
    if (cbRead != sizeof (fat.file))
      break;
    if (fat.file.attribute == 0xf) {	/* vfat entry */
      continue;
    }
    if (fat.file.cluster == 0
	|| fat.file.file[0] == 0
	|| fat.file.file[0] == 0xe5)
      continue;
    {
      int j, k = 0;
      for (j = 0; j < 11; ++j) {
	if (fat.file.file[j] != ' ') {
	  if (j == 8)
	    sz[k++] = '.';
	  sz[k++] = tolower (fat.file.file[j]);
	}
      }
      sz[k] = 0;
    }
#if 0
    {
      unsigned cluster_next = fat_next_cluster (fat.file.cluster);
      PRINTF ("  (%12.12s) @ %5d  %8ld bytes  %d (0x%x)\n", 
	      sz, fat.file.cluster, fat.file.length, 
	      cluster_next, cluster_next);
    }
#endif
    if (strcmp (d->pb[d->iRoot], sz) == 0)
      return 0;
  }

  return ERROR_FILENOTFOUND;
}

static int fat_open (struct descriptor_d* d)
{
  int result;
  char sz[128];

  ENTRY (0);

  fat.fOK = 0;
  fat.sector_fat = 0;

  if ((result = fat_identify ()))
    return result;
  fat.fOK = 1;

#if 0
  PRINTF ("descript %d %d\n", d->c, d->iRoot);
  {
    int i;
    for (i = 0; i < d->c; ++i)
      PRINTF ("  %d: (%s)\n", i, d->pb[i]);
  }
#endif

		/* Read just the partition table */
  if (d->c == 0) {
    snprintf (sz, sizeof (sz), "%s:%d+%d", 
	      szBlockDriver, 
	      SECTOR_SIZE - 66, 16*4);
    d->length = 16*4;
  }
  else {
    int partition = 0;
    if (d->iRoot > 0 && d->c)
      partition = simple_strtoul (d->pb[0], NULL, 10) - 1;
    if (partition < 0 || partition > 3 || fat.partition[partition].length == 0)
      ERROR_RETURN (ERROR_BADPARTITION, "invalid partition"); 

    snprintf (sz, sizeof (sz), "%s:%lds+%lds", 
	      szBlockDriver, 
	      fat.partition[0].start, fat.partition[0].length);
  }

	/* Open descriptor for the partition */
  if (   (result = parse_descriptor (sz, &fat.d))
      || (result = open_descriptor (&fat.d))) 
    return -1;

	/* Read parameter block */
  fat.d.driver->seek (&fat.d, 0, SEEK_SET);
  fat.d.driver->read (&fat.d, &fat.parameter, sizeof (struct parameter));
  fat.index_cluster_2
    = (fat.parameter.reserved_sectors
       + fat.parameter.fats*fat.parameter.sectors_per_fat)
    *SECTOR_SIZE
    + fat.parameter.root_entries*32;
  fat.bytes_per_cluster = fat.parameter.sectors_per_cluster*SECTOR_SIZE;

	/* Open file by finding directory entry and thus, the first cluster */
  result = fat_find (d);
  fat.cluster_file = fat.file.cluster;
  fat.index_cluster_file = 0;

  if (d->start > fat.file.length)
    d->start = fat.file.length;
  if (d->start + d->length > fat.file.length)
    d->length = fat.file.length - d->start;

  /* *** Do we want to set the default length to be the whole file?  */

  return result;
}

static void fat_close (struct descriptor_d* d)
{
  ENTRY (0);

  close_descriptor (&fat.d);
  close_helper (d);
}

static ssize_t fat_read (struct descriptor_d* d, void* pv, size_t cb)
{
  ssize_t cbRead = 0;

  ENTRY (0);

  while (cb) {
    size_t available = cb;
    size_t index = d->index + d->start;
    if (index < fat.index_cluster_file) {
      /* Rewinding not supported   */
      return -1;
    }
    
    /* Find next cluster */
    while (index >= fat.index_cluster_file + fat.bytes_per_cluster) {
      fat.cluster_file = fat_next_cluster (fat.cluster_file);
      if (fat.cluster_file == 0xffff)
	return -1;		/* An unrecoverable error */
      fat.index_cluster_file += fat.parameter.sectors_per_cluster*SECTOR_SIZE;
    }

    /* Bound within the cluster */
    if (index + available > fat.index_cluster_file + fat.bytes_per_cluster)
      available = fat.index_cluster_file + fat.bytes_per_cluster - index;

    /* Bound within exact extent of file */
    if (index + available > fat.file.length)
      available = fat.file.length - index;

#if 0
    PRINTF (" fat.idx_clus_2 %d (%x) "
	    " fat.clus_file %d "
	    " fat.by_p_clus %d\n"
	    " idx %d (%x) "
	    " fat.idx_clus_f %d (%x)\n",
	    fat.index_cluster_2, fat.index_cluster_2, 
	    fat.cluster_file,
	    fat.bytes_per_cluster,
	    index, index,
	    fat.index_cluster_file, fat.index_cluster_file);
#endif

    fat.d.driver->seek (&fat.d, 
			fat.index_cluster_2 
			+ (fat.cluster_file - 2)*fat.bytes_per_cluster
			+ index - fat.index_cluster_file,
			SEEK_SET);

    {
      ssize_t cbThis = fat.d.driver->read (&fat.d, pv, available);
      if (cbThis <= 0)
	break;
      d->index += cbThis;
      cb -= cbThis;
      cbRead += cbThis;
    }
  }
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

#if 0
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
//  .init = fat_init,
#if !defined (CONFIG_SMALL)
  .report = fat_report,
#endif
};
