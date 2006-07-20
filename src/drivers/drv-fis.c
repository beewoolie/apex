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

   The open call converts the fis driver descriptor into a memory
   driver descriptor that points to the partition.  It's a simple
   translation.  This driver has no read methods and the flash
   partition data cannot be written through this descriptor.

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

#if defined (CONFIG_ENV)
static __env struct env_d e_fis_drv = {
  .key = "fis-drv",
  .default_value = CONFIG_DRIVER_FIS_BLOCKDEVICE,
  .description = "Block device region for FIS partition driver",
};
#endif

static inline const char* block_driver (void)
{
  return lookup_alias_or_env ("fis-drv", CONFIG_DRIVER_FIS_BLOCKDEVICE);
}

static int end_of_table (struct fis_descriptor* descriptor)
{
  return descriptor->start == 0xffffffff;
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

    descriptor.start += d->start;
    descriptor.length -= d->start;
    if (d->length && d->length < descriptor.length)
      descriptor.length = d->length;
    close_helper (d);

    /* Construct a memory descriptor for the FIS partition */
    parse_descriptor ("mem:", d);
    d->start = descriptor.start;
    d->length = descriptor.length;
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

  printf ("  fis:\n");

  while (1) {
    struct fis_descriptor descriptor;
    if (d.driver->read (&d, &descriptor, sizeof (descriptor))
	!= sizeof (descriptor))
      break;
    if (end_of_table (&descriptor))
      break;
    printf ("          0x%08lx+0x%08lx %s\n",
	    descriptor.start, descriptor.length, descriptor.name);
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
