/* region-checksum.c

   written by Marc Singer
   22 Aug 2008

   Copyright (C) 2008 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/


#include <config.h>
#include <apex.h>
#include <driver.h>
#include <error.h>
#include <spinner.h>
#include "region-checksum.h"

extern unsigned long compute_crc32 (unsigned long crc, const void *pv, int cb);
//extern uint32_t compute_crc32_length (uint32_t crc, size_t cb);

/** Compute the CRC32 checksum for a region.  The flags includes a bit
    to add each byte of the length, in LSB order, as is used by the
    POSIX cksum command. */

int region_checksum (struct descriptor_d* d, unsigned flags,
                     unsigned long* crc_result)
{
  ssize_t available = d->length - d->index;
  int index = 0;
  unsigned long crc = 0;
  while (index < available) {
    char __aligned rgb[512];
    int cb = d->driver->read (d, rgb, sizeof (rgb));
    if (cb < 0)
      return ERROR_IOFAILURE;
    if (flags & regionChecksumSpinner)
      SPINNER_STEP;
    crc = compute_crc32 (crc, rgb, cb);
    index += cb;
  }

#if 0
  compute_crc32_length (crc, index);
#else
  /* Add the length to the computation */
  {
    unsigned char b;
    unsigned long v;
    for (v = index; v; v >>= 8) {
      b = v & 0xff;
      crc = compute_crc32 (crc, &b, 1);
    }
  }
#endif

  *crc_result = crc;
  return 0;
}
