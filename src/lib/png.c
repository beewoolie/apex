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

/* *** FIXME: this heap isn't going to cut it */
static unsigned char heap [128*1024]; /* Fake heap */
static size_t heap_allocated;	/* Bytes allocated on the heap */

struct png {
  const unsigned char* pb;
  size_t cb;

  ssize_t ib;
  struct chunk c;		/* Current chunk */

  struct png_header hdr;
  int bpp;
  int cbRow;

  char* pbThis;
  char* pbPrev;

  z_stream z;			/* Decompressor context */
};

static struct png png;

static voidpf heap_alloc (voidpf opaque, uInt items, uInt size)
{
  int cb = (items*size + 3) & ~3;
  voidpf result = heap + heap_allocated;

  if (heap_allocated + cb > sizeof (heap)) {
    printf ("%s: heap overflow %d %d\n", __FUNCTION__, heap_allocated, cb);
    return Z_NULL;
  }

  heap_allocated += cb;
//  printf ("%s: %p <= %d %d = %d (%d)\n", __FUNCTION__, 
//	  result, items, size, cb, heap_allocated);
  return result;
}

static void heap_free (voidpf opaque, voidpf address, uInt nbytes)
{
//  printf ("%s: %p %d\n", __FUNCTION__, address, nbytes);
}

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

static long read_long (const unsigned char* pb)
{
  return (pb[0] << 24) 
    + (pb[1] << 16) 
    + (pb[2] << 8)
    +  pb[3];
}

static int next_chunk (struct png* png)
{
//  printf ("%s:%d\n", __FUNCTION__, png->ib);

  if (png->ib)
    png->ib += 8 + png->c.length + 4;
  else
    png->ib = 8;		/* Skip MAGIC */
//  printf ("%s:  %d %d\n", __FUNCTION__, png->ib, png->cb);
  if (png->ib >= png->cb)
    return -1;

  png->c.length = read_long (png->pb + png->ib);
  memcpy (&png->c.id, &png->pb[png->ib + 4], 4);
  return 0;
}

void* open_png (const void* pv, size_t cb)
{
  heap_allocated = 0;		/* Completely clobber heap */

//  printf ("%s: %p %d\n", __FUNCTION__, pv, cb); 

  if (cb < 8)
    return NULL;

  /* Check for MAGIC */
  {
    const char* pb = pv;
    if (   pb[0] != 137
	|| pb[1] != 80
	|| pb[2] != 78
	|| pb[3] != 71
	|| pb[4] != 13
	|| pb[5] != 10
	|| pb[6] != 26
	|| pb[7] != 10)
      return NULL; 
  }

//  printf ("%s: magic OK\n", __FUNCTION__); 

  png.pb = pv;
  png.cb = cb;
  png.ib = 0;


//  printf ("%s: reading first chunk\n", __FUNCTION__); 

  if (next_chunk (&png) || memcmp (&png.c.id, "IHDR", 4))
    return NULL;

//  printf ("%s: pulling header\n", __FUNCTION__); 

  png.hdr.width       = read_long (png.pb + png.ib + 8);
  png.hdr.height      = read_long (png.pb + png.ib + 8 + 4);
  png.hdr.bit_depth   = png.pb[png.ib + 8 + 8];
  png.hdr.color_type  = png.pb[png.ib + 8 + 9];
  png.hdr.compression = png.pb[png.ib + 8 + 10];
  png.hdr.filter      = png.pb[png.ib + 8 + 11];
  png.hdr.interleave  = png.pb[png.ib + 8 + 12];

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
    return NULL;

  png.cbRow = 1 + png.hdr.width*png.bpp;
//  printf ("%s: cbRow %d  bpp %d\n", __FUNCTION__, png.cbRow, png.bpp);

  return &png;
}

int read_png_ihdr (void* pv, struct png_header* hdr)
{
  memcpy (hdr, &((struct png*) pv)->hdr, sizeof (struct png_header));
  return 0;
}

/* read_png_idat

   is a bastard of a function.  It should allow reading of an
   arbitrary amount of data from the image.  Instead, because we use
   the heap functions
*/

static ssize_t read_png_idat (void* pv, unsigned char* rgb, size_t cb)
{
  struct png* png = pv;
  int result;

	/* Initialization */
  if (memcmp (&png->c.id, "IDAT", 4)) {
//    printf ("%s: init\n", __FUNCTION__); 

//    printf ("%s: inflateInit\n", __FUNCTION__); 
    memset (&png->z, 0, sizeof (png->z));
    png->z.zalloc = heap_alloc;
    png->z.zfree = heap_free;
    if (inflateInit (&png->z) != Z_OK)
      return -1;
  }

	/* Find next IDAT chunk */
  if (png->z.avail_in == 0) {
//    printf ("%s: search for idat\n", __FUNCTION__); 
    while (next_chunk (png) == 0) {
//      printf ("%s: looking %d %4.4s\n", __FUNCTION__, 
//	      png->c.length, (char*) &png->c.id);

      if (memcmp (&png->c.id, "IEND", 4) == 0)
	return -1;
      if (memcmp (&png->c.id, "IDAT", 4) == 0)
	break;
    }

    png->z.next_in = (char*) (png->pb + png->ib + 8);
    png->z.avail_in = png->c.length;
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

const unsigned char* read_png_row (void* pv)
{
  struct png* png = pv;
  int i;
  int bpp = png->bpp;
  int cbRow = png->cbRow;
  char* pb;
  char* pbPrev;
  char filter;

  if (png->pbThis == NULL) {
    png->pbThis = heap_alloc (0, 1, png->cbRow + bpp - 1);
    png->pbPrev = heap_alloc (0, 1, png->cbRow + bpp - 1);
    memset (png->pbThis, 0, png->cbRow + bpp - 1); /* Note: immediate swap */
  }

  {
    char* pbSwap = png->pbThis;
    png->pbThis = png->pbPrev;
    png->pbPrev = pbSwap;
  }

  pb     = png->pbThis + bpp - 1;
  pbPrev = png->pbPrev + bpp - 1;

//  printf ("%s:\n", __FUNCTION__); 

  if (read_png_idat (pv, pb, cbRow) != cbRow) {
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
      pb[i] = ((int) pb[i - bpp] + (int)pbPrev[i])/2 & 0xff;
    break;
  case 4:			/* Paeth filter */
    for (i = 1; i < cbRow; ++i)
      pb[i] = (pb[i] + paeth_predictor (pb    [i - bpp], 
					pbPrev[i      ], 
					pbPrev[i - bpp])) & 0xff;
    break;
  }

  return pb + 1;
}

void close_png (void* pv)
{
  struct png* png = pv;

  inflateEnd (&png->z);

  png->pbThis = NULL;		/* In lieu of free */
  png->pb = 0;
  png->cb = 0;
}
