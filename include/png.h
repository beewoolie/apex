/* png.h
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

*/

#if !defined (__PNG_H__)
#    define   __PNG_H__

/* ----- Includes */

/* ----- Types */

struct png_header {
  int width;
  int height;
  int bit_depth;
  int color_type;
  int compression;
  int filter;
  int interleave;
};

/* ----- Globals */

/* ----- Prototypes */

void*		     open_png (const void* pv, size_t cb);
int		     read_png_ihdr (void* pv, struct png_header* hdr);
const unsigned char* read_png_row (void* pv);  
void		     close_png (void* pv);

#endif  /* __PNG_H__ */
