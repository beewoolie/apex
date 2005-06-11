/* zlib-heap.c
     $Id$

   written by Marc Singer
   10 Jun 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

   Heap helper for zlib.  It wants to have a memory allocator, but we
   don't have one.  We fake it with these routines.

   o Heap

     A very crude heap is made available to the zlib routines.  It
     allocates, but doesn't deallocate.  As we don't care much about
     using memory, this is OK.  However, it is quite dumb.  The zlib
     stuff here should be replaced with the inflate code from the
     kernel which uses static buffers.

*/


#include <config.h>
#include <apex.h>
#include <linux/kernel.h>
#include <linux/string.h>
#include <zlib.h>
#include <zlib-heap.h>

/* *** FIXME: this heap isn't really going to cut it, it is? */
static unsigned char heap [128*1024]; /* Fake heap */
static size_t heap_allocated;	/* Bytes allocated on the heap */

voidpf zlib_heap_alloc (voidpf opaque, uInt items, uInt size)
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

void zlib_heap_free (voidpf opaque, voidpf address, uInt nbytes)
{
//  printf ("%s: %p %d\n", __FUNCTION__, address, nbytes);
}

void zlib_heap_reset (void)
{
  heap_allocated = 0;
}
