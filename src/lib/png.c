/* png.c
     $Id$

   written by Marc Singer
   3 Mar 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

   PNG decoder.  This module allows for inclusion of compressed splash
   screens.

   The code is very simple.  It only allows decompression of one image
   at a time.  The caller doesn't know this and must pass the context
   pointer.

   The easy way to insert a PNG into the binary is to declare it as an
   array of bytes.  The raw data may be converted to a convenient list
   of hexadecimal values using hexdump.

     hexdump -v -e '1/1 "0x%02x, " "\n" ' < image.png  > image.h

   The resulting file may be included like so:

     unsigned char rgbImage[] = {
     #include "image.h"
     };

*/

#include <config.h>
#include <apex.h>
#include <linux/string.h>
#include <linux/kernel.h>

#include <zlib.h>
#include <png.h>

struct chunk {
  u32 length;
  u32 id;
};

struct png {
  const unsigned char* pb;
  size_t cb;

  ssize_t ib;
  struct chunk c;		/* Current chunk */

  struct png_header hdr;

  z_stream z;			/* Decompressor context */
};

static struct png png;

static long read_long (const unsigned char* pb)
{
  return (pb[0] << 24) 
    + (pb[1] << 16) 
    + (pb[2] << 8)
    +  pb[3];
}

static int next_chunk (struct png* png)
{
  if (png->ib)
    png->ib += 8 + png->c.length + 4;
  if (png->ib >= png->cb)
    return -1;

  png->c.length = read_long (png->pb + png->ib);
  memcpy (&png->c.id, &png->pb[png->ib + 4], 4);
  return 0;
}

void* open_png (const void* pv, size_t cb)
{
  if (png.cb < 8)
    return NULL;

  /* Check for MAGIC */
  {
    const char* pb = pv;
    if (pb[0] != 137
	|| pb[1] != 80
	|| pb[2] != 78
	|| pb[3] != 71
	|| pb[4] != 13
	|| pb[5] != 10
	|| pb[6] != 26
	|| pb[7] != 10)
      return NULL; 
  }

  png.pb = pv;
  png.cb = cb;
  png.ib = 0;


  if (next_chunk (&png) || memcmp (&png.c.id, "IHDR", 4))
    return NULL;

  png.hdr.width       = read_long (png.pb + png.ib + 8);
  png.hdr.height      = read_long (png.pb + png.ib + 8 + 4);
  png.hdr.bit_depth   = png.pb[png.ib + 8 + 8];
  png.hdr.color_type  = png.pb[png.ib + 8 + 9];
  png.hdr.compression = png.pb[png.ib + 8 + 10];
  png.hdr.filter      = png.pb[png.ib + 8 + 11];
  png.hdr.interleave  = png.pb[png.ib + 8 + 12];

  return &png;
}

int read_png_ihdr (void* pv, struct png_header* hdr)
{
  memcpy (hdr, &((struct png*) pv)->hdr, sizeof (struct png_header));
  return 0;
}

ssize_t read_png_idat (void* pv, unsigned char* rgb, size_t cb)
{
  struct png* png = pv;

	/* Initialization */
  if (memcmp (&png->c.id, "IDAT", 4)) {
    memset (&png->z, 0, sizeof (png->z));
    if (inflateInit (&png->z) != Z_OK)
      return -1;
  }

	/* Find next IDAT chunk */
  if (png->z.avail_in == 0) {

    while (next_chunk (png) == 0) {
      if (memcmp (&png->c.id, "IEND", 4))
	return -1;
      if (memcmp (&png->c.id, "IDAT", 4))
	break;
    }

    png->z.next_in = (char*) (png->pb + png->ib + 8);
    png->z.avail_in = png->c.length;
  }

  png->z.next_out = rgb;
  png->z.avail_out = cb;
  if (inflate (&png->z, 0) == Z_OK)
    return cb - png->z.avail_out;

  return -1;
}

void close_png (void* pv)
{
  struct png* png = pv;

  inflateEnd (&png->z);

  png->pb = 0;
  png->cb = 0;
}
