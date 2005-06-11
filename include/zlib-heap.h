/* zlib-heap.h
     $Id$

   written by Marc Singer
   10 Jun 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__ZLIB_HEAP_H__)
#    define   __ZLIB_HEAP_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

extern voidpf zlib_heap_alloc (voidpf opaque, uInt items, uInt size);
extern void zlib_heap_free (voidpf opaque, voidpf address, uInt nbytes);
extern void zlib_heap_reset (void);


#endif  /* __ZLIB_HEAP_H__ */
