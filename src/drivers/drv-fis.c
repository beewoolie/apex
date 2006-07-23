/* drv-fis.c

   written by Marc Singer
   19 Jul 2006

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

   FIS is a partition driver.  It parses flash image descriptors in an
   underlying region as files of a sort.  It is similar to the
   filesystem drivers in this way.

   IXP42X in LE Mode
   -----------------

     This Xscale processor does some shenanigans with byte & half-word
     swapping when it is in little-endian mode.  A flash part which
     was configured for a big-endian system doesn't read correctly
     when the CPU is in little-endian mode.  For the most part, this
     is handled by the NOR flash driver.

     The FIS driver needs to cope in that the data in the partition
     table will be ordered for big-endian consumption.  The strings
     will be OK because of the NOR flash driver's efforts.  However
     the start and length values will be BE and must be swapped for
     LE.

     This conversion is done by reading all of the partition entries,
     looking for the partition table's own entry.  When it is found,
     the driver checks the length of the block for a match with the
     NOR flash erase block size.  If there is a match when the data is
     swapped, it is assumed that the system is reading a BE partition
     table.

     For simplicity and general correctness, this is done every time
     this driver activates.  The table could be cached, but it is
     possible that the underlying block driver region is changed
     between invocations.  So, the easy solution is to read through
     the table twice: once to determine the endian-ness and a second
     time looking for the data we care about.

   Open Call Pass Through
   ----------------------

   The open call converts the fis driver descriptor into a descriptor
   that points to the partition using the source driver.  Due to the
   fact that that partition table is physically addressed, the FIS
   driver has to subtract the base address of the underlying driver
   when passing this through.  Thus, the FIS driver has no
   read/seek/write methods.

*/

#include <config.h>
#include <apex.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <spinner.h>
#include <asm/reg.h>
#include <error.h>
#include <environment.h>
#include <lookup.h>

struct fis_descriptor {
  char name[16];		/* Image name, null terminated */
  unsigned long start;		/* Physical memory address of image */
  unsigned long lma;		/* Load Memory Address for image */
  unsigned long length;		/* Byte length of partition */
  unsigned long entry;		/* Image entry point */
  unsigned long data_length;	/* Byte length of image */
  unsigned char _pad[256 - 16 - 7*sizeof(unsigned long)];
  unsigned long cksum_desc;	/* Checksum over descriptor */
  unsigned long cksum_image;	/* Checksum over image data */
};

static int fis_directory_swap;	/* Set for a byte swapped directory */

#if defined (CONFIG_ENV)
static __env struct env_d e_fis_drv = {
  .key = "fis-drv",
  .default_value = CONFIG_DRIVER_FIS_BLOCKDEVICE,
  .description = "Block device region for FIS partition driver",
};
#endif

static inline unsigned long swab32(unsigned long l)
{
  return (  ((l & 0x000000ffUL) << 24)
	  | ((l & 0x0000ff00UL) << 8)
	  | ((l & 0x00ff0000UL) >> 8)
	  | ((l & 0xff000000UL) >> 24));
}

static inline const char* block_driver (void)
{
  return lookup_alias_or_env ("fis-drv", CONFIG_DRIVER_FIS_BLOCKDEVICE);
}

static int end_of_table (struct fis_descriptor* descriptor)
{
  return descriptor->start == 0xffffffff;
}

static void prescan_directory (struct descriptor_d* d)
{
  unsigned long start = 0;
  unsigned long size = 0;
  unsigned long eraseblocksize = 0;

  fis_directory_swap = 0;

  descriptor_query (d, QUERY_START, &start);
  descriptor_query (d, QUERY_SIZE, &size);

  printf ("%s: start %lx size %ld\n", __FUNCTION__, start, size);
  if (size == 0)
    return;			/* Unable to perform queries */

  d->driver->seek (d, 0, SEEK_SET);

  while (1) {
    struct fis_descriptor descriptor;
    if (d->driver->read (d, &descriptor, sizeof (descriptor))
	!= sizeof (descriptor))
      break;
    if (end_of_table (&descriptor))
      break;
    if (strnicmp (descriptor.name, "fis directory", 16) == 0) {
      descriptor_query (d, QUERY_ERASEBLOCKSIZE, &eraseblocksize);
      printf ("%s: length %lx  eb %lx  eb_swapped %lx\n",
	      __FUNCTION__, descriptor.length, eraseblocksize,
	      swab32 (eraseblocksize));
      if (descriptor.length == swab32 (eraseblocksize))
	fis_directory_swap = 1;
      break;
    }
  }
  d->driver->seek (d, 0, SEEK_SET);
}

static const char* map_region (struct descriptor_d* d,
			       struct fis_descriptor* descriptor)
{
  static char sz[64];
  unsigned long start = 0;
  descriptor_query (d, QUERY_START, &start);

  snprintf (sz, sizeof (sz), "%s:0x%08lx+0x%08lx",
	    d->driver_name,
	    descriptor->start - start, descriptor->length);
  return sz;
}

static int fis_open (struct descriptor_d* d)
{
  int result;
  struct descriptor_d fis_d;

  if (d->c != 1 || d->iRoot != 1)
    ERROR_RETURN (ERROR_BADPARTITION, "path must include partition");

  if (   (result = parse_descriptor (block_driver (), &fis_d))
      || (result = open_descriptor (&fis_d)))
    return result;

  prescan_directory (&fis_d);

  while (1) {
    struct fis_descriptor descriptor;
    if (fis_d.driver->read (&fis_d, &descriptor, sizeof (descriptor))
	!= sizeof (descriptor)) {
      close_descriptor (&fis_d);
      ERROR_RETURN (ERROR_IOFAILURE, "premature end of fis-drv");
    }
    if (end_of_table (&descriptor))
      break;
    if (strnicmp (d->pb[0], descriptor.name, sizeof (descriptor.name)))
      continue;

    if (d->start >= descriptor.length) {
      close_descriptor (&fis_d);
      ERROR_RETURN (ERROR_OPEN, "region exceeds partition size");
    }

    if (fis_directory_swap) {
      descriptor.start = swab32 (descriptor.start);
      descriptor.length = swab32 (descriptor.length);
    }

    descriptor.start += d->start;
    descriptor.length -= d->start;
    if (d->length && d->length < descriptor.length)
      descriptor.length = d->length;
    close_helper (d);

    /* Construct a descriptor for the FIS partition */
    {
      const char* sz = map_region (&fis_d, &descriptor);
      printf ("%s: %s\n", __FUNCTION__, sz);
      if (   (result = parse_descriptor (sz, d))
	  || (result = open_descriptor (d)))
	return result;
    }
    return 0;
  }

  close_descriptor (&fis_d);
  ERROR_RETURN (ERROR_BADPARTITION, "partition not found");
}

#if !defined (CONFIG_SMALL)
static void fis_report (void)
{
  int result;
  struct descriptor_d d;

  if (   (result = parse_descriptor (block_driver (), &d))
      || (result = open_descriptor (&d)))
    return;

  prescan_directory (&d);

  printf ("  fis:\n");

  while (1) {
    struct fis_descriptor descriptor;
    if (d.driver->read (&d, &descriptor, sizeof (descriptor))
	!= sizeof (descriptor))
      break;
    if (end_of_table (&descriptor))
      break;

    if (fis_directory_swap) {
      descriptor.start = swab32 (descriptor.start);
      descriptor.length = swab32 (descriptor.length);
    }

    printf ("          %s %s\n",
	    map_region (&d, &descriptor), descriptor.name);
  }

  close_descriptor (&d);
}
#endif

static __driver_6 struct driver_d fis_driver = {
  .name		= "fis-part",
  .description	= "FIS partition driver",
  .flags	= DRIVER_DESCRIP_FS,
  .open		= fis_open,
  .close	= close_helper,
};

static __service_6 struct service_d fis_service = {
#if !defined (CONFIG_SMALL)
  .report = fis_report,
#endif
};
