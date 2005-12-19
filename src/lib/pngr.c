/* png.c
     $Id$

   written by Marc Singer
   3 Mar 2005

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

   PNG decoder implemented for regions.  This module allows for
   decompression of PNG images stored in any region. 

   The code is very simple.  It only allows decompression of one image
   at a time.  The caller cannot know this and so must pass a context
   pointer.

   o Creating Images for the Binary Image

     PNG images may be read from within the loader binary.  This is an
     good option for users who want to display one and only one image.

     The way to insert a PNG into the binary is to declare it as an
     array of bytes.  The raw data may be converted to a convenient
     stream of hexadecimal values using hexdump.

       hexdump -v -e '1/1 "0x%02x, " "\n" ' < image.png  > image.h

     The resulting file may be included like so:

       unsigned char rgbImage[] = {
       #include "image.h"
      };

      There will (should) be a way to include the image in the
      executable without requiring an array inclusion.

   o CRC

     We don't bother checking the CRCs.  As this is supposed to run
     from RAM, it is probably better to perform a CRC check on the
     whole firmware image instead of adding overhead of calculating it
     on the image chunks.

   o Robustness

     Many variations on PNG files are not handled.  It should be OK
     for eight and 16 bit components.  Others can be added if images
     for them are found.

   o Chunk reading

     The read code needs to be careful about reading the correct
     number of bytes.  As we are reading from a stream, we must make
     sure that all of the previous chunk has been read before calling
     next_chunk.  Alternatively, we could implement a meta-read
     function that makes sure to check the ranging.  Perhaps, we'll do
     this eventually

   o Decode and the rgbDecode buffer

     We want to be able to decompress chunks in pieces with a
     reasonably sized zlib input buffer.  Keep in mind that we may be
     reading from a filesystem and we cannot simply mmap the data.
     Moreover, we don't really know how large a chunk can be.  The
     specification makes no such declaration.  

     As a stop-gap measure, we're going to declare a static buffer.
     This isn't terribly onerous, but we would like to be able to
     guarantee correct behavior without depending on compiled-in size
     limitations.

*/

#include <config.h>
#include <driver.h>
#include <apex.h>
#include <linux/string.h>
#include <linux/kernel.h>

#include <zlib.h>
#include <zlib-heap.h>
#include <png.h>

struct chunk {
  u32 length;
  u32 id;
};

struct png {
  struct descriptor_d* d;

//  const unsigned char* pb;
//  size_t cb;

  struct chunk c;		/* Current chunk */
  ssize_t ib;			/* Index within the chunk */

  struct png_header hdr;
  int bpp;
  int cbRow;

  char* pbThis;
  char* pbPrev;

  z_stream z;			/* Decompressor context */
};

static struct png png;
static char __xbss(png) rgbDecode[32*1024];	/* zlib decode buffer */

static inline int paeth_predictor (int a, int b, int c)
{
  int p = a + b - c;
  int pa = p - a;
  int pb = p - b;
  int pc = p - c;

  if (pa < 0)
    pa = -pa;
  if (pb < 0)
    pb = -pb;
  if (pc < 0)
    pc = -pc;

  if (pa <= pb && pa <= pc)
    return a;
  if (pb <= pc)
    return b;
  return c;
}

static ssize_t pngr_read (struct png* png, void* pv, size_t cb)
{
  ssize_t cbRead;
  
  if (cb > png->c.length - png->ib)
    cb = png->c.length - png->ib;

  printf ("%s: pngr_read %d to %p\n", __FUNCTION__, cb, pv);
  cbRead = png->d->driver->read (png->d, pv, cb);
  printf ("%s: pngr_read complete\n", __FUNCTION__);
  png->ib += cbRead;

  return cbRead;
}


static long read_long (const void* pv)
{
  const unsigned char* pb = (const char*) pv;
  return (pb[0] << 24) 
    + (pb[1] << 16) 
    + (pb[2] << 8)
    +  pb[3];
}

static int next_chunk (struct png* png)
{
  printf ("%s:%d\n", __FUNCTION__, png->ib);

  //  if (png->ib)
  //    png->ib += 8 + png->c.length + 4;
  //  else
  //    png->ib = 8;		/* Skip MAGIC */
//  printf ("%s:  %d %d\n", __FUNCTION__, png->ib, png->cb);
  //  if (png->ib >= png->cb)
  //    return -1;

	/* Skip remainder of chunk */
  if (png->c.length) {
    printf ("%s: seeking %d\n", __FUNCTION__, png->c.length - png->ib + 4);
    png->d->driver->seek (png->d, png->c.length - png->ib + 4, SEEK_CUR);
  }

  printf ("%s: reading chunk header %d\n",
	  __FUNCTION__, sizeof (struct chunk));
  png->d->driver->read (png->d, &png->c.length, sizeof (struct chunk));
  png->c.length = read_long (&png->c.length);
  png->ib = 0;
  printf ("%s: type %x  l %d\n", __FUNCTION__, png->c.id, png->c.length);
  return 0;
}

void* open_pngr (struct descriptor_d* d)
{
  int cb;
  char rgb[8];

  zlib_heap_reset ();
  png.pbThis = 0;		/* In lieu of free */
  png.pbPrev = 0;		/* In lieu of free */

  printf ("%s\n", __FUNCTION__); 

  if (d->length < 8) {
  close_fail:
    return NULL;
  }

  printf ("pngr_read of magic\n");
  cb = d->driver->read (d, rgb, 8);
  if (cb < 8)
    goto close_fail;

  if (   rgb[0] != 137
      || rgb[1] != 80
      || rgb[2] != 78
      || rgb[3] != 71
      || rgb[4] != 13
      || rgb[5] != 10
      || rgb[6] != 26
      || rgb[7] != 10)
    goto close_fail;

  printf ("%s: magic OK\n", __FUNCTION__); 

  png.d = d;
//  png.ib = 0;
  png.c.length = 0;

  if (next_chunk (&png) || memcmp (&png.c.id, "IHDR", 4))
    goto close_fail;

  printf ("%s: pulling header\n", __FUNCTION__); 

  pngr_read (&png, &png.hdr, sizeof (struct png_header));
  printf ("%s: fixing header\n", __FUNCTION__);
  png.hdr.width       = read_long (&png.hdr.width);
  png.hdr.height      = read_long (&png.hdr.height);

  printf ("%s: header %d %d %d %d\n", __FUNCTION__,
	  png.hdr.width, png.hdr.height, 
	  png.hdr.bit_depth, png.hdr.color_type);

  switch (png.hdr.color_type) {
  case 0:
    png.bpp = png.hdr.bit_depth/8; /* Wrong for 1bpp, 2bpp or 4bpp images */
    break;
  case 2:
    png.bpp = png.hdr.bit_depth*3/8;
    break;
  case 3:
    png.bpp = png.hdr.bit_depth/8;
    break;
  case 4:
    png.bpp = png.hdr.bit_depth*2/8;
    break;
  case 6:
    png.bpp = png.hdr.bit_depth*4/8;
    break;
  }

  if (png.hdr.bit_depth != 8)	/* Weak sanity check */
    goto close_fail;

  png.cbRow = 1 + png.hdr.width*png.bpp;
  printf ("%s: cbRow %d  bpp %d\n", __FUNCTION__, png.cbRow, png.bpp);

  return &png;
}

int read_pngr_ihdr (void* pv, struct png_header* hdr)
{
	/* Read from our cached/fixed-up copy */
  memcpy (hdr, &((struct png*) pv)->hdr, sizeof (struct png_header));
  return 0;
}

static ssize_t read_pngr_idat (void* pv, unsigned char* rgb, size_t cb)
{
  struct png* png = pv;
  int result;

	/* Initialization */
  if (memcmp (&png->c.id, "IDAT", 4)) {
//    printf ("%s: init\n", __FUNCTION__); 

//    printf ("%s: inflateInit\n", __FUNCTION__); 
    memset (&png->z, 0, sizeof (png->z));
    png->z.zalloc = zlib_heap_alloc;
    png->z.zfree = zlib_heap_free;
    if (inflateInit (&png->z) != Z_OK)
      return -1;
  }

	/* Find next IDAT chunk */
  if (png->z.avail_in == 0) {
    printf ("%s: search for idat\n", __FUNCTION__); 
    while (next_chunk (png) == 0) {
//      printf ("%s: looking %d %4.4s\n", __FUNCTION__, 
//	      png->c.length, (char*) &png->c.id);

      if (memcmp (&png->c.id, "IEND", 4) == 0)
	return -1;
      if (memcmp (&png->c.id, "IDAT", 4) == 0)
	break;
    }

    /* *** FIXME: we assume that we can read all of the chunk into
       this buffer.  It would be better to code the loop to be able to
       read out pieces of the image. */
    png->z.next_in = rgbDecode;
    png->z.avail_in = pngr_read (png, rgbDecode, sizeof (rgbDecode));
  }

  png->z.next_out = rgb;
  png->z.avail_out = cb;
//  printf ("%s: inflate\n", __FUNCTION__); 
  result = inflate (&png->z, 0);
  if (result < 0)
    return result;
  if (result == Z_STREAM_END && cb - png->z.avail_out == 0)
    return -1;
  return cb - png->z.avail_out;
}

const unsigned char* read_pngr_row (void* pv)
{
  struct png* png = pv;
  int i;
  int bpp = png->bpp;
  int cbRow = png->cbRow;
  char* pb;
  char* pbPrev;
  char filter;

  if (png->pbThis == NULL) {
    png->pbThis = zlib_heap_alloc (0, 1, png->cbRow + bpp - 1);
    png->pbPrev = zlib_heap_alloc (0, 1, png->cbRow + bpp - 1);
    memset (png->pbPrev, 0, png->cbRow + bpp - 1);
  }

  pb     = png->pbThis + bpp - 1;
  pbPrev = png->pbPrev + bpp - 1;

//  printf ("%s:\n", __FUNCTION__); 

  if (read_pngr_idat (pv, pb, cbRow) != cbRow) {
    printf ("%s: read failed\n", __FUNCTION__); 
    return NULL;
  }

  filter = pb[0];
  pb[0] = 0;			/* So adjacent pel is 0; simplifies loops */

#if 0
  {
    static int code;
    static int row;
    if (code != filter) {
      printf ("%s: code %d  row %d\n", __FUNCTION__, filter, row);
      code = filter;
    }
    ++row;
  }
#endif

  switch (filter) {
  case 0:			/* No filtering */
    break;
  case 1:			/* Sub filter */
    for (i = 1; i < cbRow; ++i)
      pb[i] = (pb[i] + pb[i - bpp]) & 0xff;
    break;
  case 2:			/* Up filter */
    for (i = 1; i < cbRow; ++i)
      pb[i] = (pb[i] + pbPrev[i]) & 0xff;
    break;
  case 3:			/* Averaging filter */
    for (i = 1; i < cbRow; ++i)
      pb[i] = (pb[i] + ((int) pb[i - bpp] + (int)pbPrev[i])/2) & 0xff;
    break;
  case 4:			/* Paeth filter */
    for (i = 1; i < cbRow; ++i)
      pb[i] = (pb[i] + paeth_predictor (pb    [i - bpp], 
					pbPrev[i      ], 
					pbPrev[i - bpp])) & 0xff;
    break;
  }

	/* Swap for next invocation */
  {
    char* pbSwap = png->pbThis;
    png->pbThis = png->pbPrev;
    png->pbPrev = pbSwap;
  }

  return pb + 1;
}

void close_pngr (void* pv)
{
  struct png* png = pv;

  inflateEnd (&png->z);

  png->d = 0;

//  png->pb = 0;
//  png->cb = 0;
}
