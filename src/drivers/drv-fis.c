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

*/

#include <config.h>
#include <apex.h>
#include <driver.h>
#include <alias.h>
#include <environment.h>
#include <service.h>
#include <linux/string.h>
#include <spinner.h>
#include <asm/reg.h>
#include <error.h>

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

#define DRIVER_NAME	"fis"

#define _s(v) #v
#define _t(v) _s(v)

#if defined (CONFIG_ENV)
static __env struct env_d e_fis_drv = {
  .key = "fis-drv",
  .default_value = _t(CONFIG_DRIVER_FIS_BLOCKDEVICE),
  .description = "Block device region for FIS driver",
};
#endif

static const char* block_driver (void)
{
  const char* sz;
#if defined (CONFIG_CMD_ALIAS)
  sz = alias_lookup ("fis-drv");
  if (sz)
    return sz;
#endif
#if defined (CONFIG_ENV)
  sz = env_fetch ("fis-drv");
  if (sz)
    return sz;
#endif
  sz = CONFIG_DRIVER_FIS_BLOCKDEVICE;
  return sz;
}

static void fis_report (void)
{
  int result;
  struct descriptor_d d;

  printf (" (fis block driver %s\n", block_driver ());

  if (   (result = parse_descriptor (block_driver (), &d))
      || (result = open_descriptor (&d)))
    return;

  printf ("  fis:\n");

  while (1) {
    struct fis_descriptor descriptor;
    if (d.driver->read (&d, &descriptor, sizeof (descriptor))
	!= sizeof (descriptor))
      break;
    if (descriptor.start == 0xffffffff)
      break;
    printf ("    0x%08lx+0x%08lx %s\n",
	    descriptor.start, descriptor.length, descriptor.name);
  }

  close_descriptor (&d);
}


static __driver_6 struct driver_d fis_driver = {
  .name = DRIVER_NAME,
  .description = "FIS partition driver",
  .flags = DRIVER_DESCRIP_FS,
  //  .open = fis_open,
  //  .close = fis_close,
  //  .read = fis_read,
  //  .seek = seek_helper,
#if defined (CONFIG_CMD_INFO)
  //  .info = fis_info,
#endif
};

static __service_6 struct service_d fis_service = {
//  .init = fis_init,
#if !defined (CONFIG_SMALL)
  .report = fis_report,
#endif
};
