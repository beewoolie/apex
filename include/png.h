/* png.h
     $Id$

   written by Marc Singer
   3 Mar 2005

   Copyright (C) 2005 Marc Singer

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
