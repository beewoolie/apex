/* zlib-heap.h

   written by Marc Singer
   10 Jun 2005

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
