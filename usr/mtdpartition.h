/* mtdpartition.h

   written by Marc Singer
   14 Jan 2007

   Copyright (C) 2007 Marc Singer

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

#if !defined (__MTDPARTITION_H__)
#    define   __MTDPARTITION_H__

/* ----- Includes */

#include <string.h>

/* ----- Types */

struct MTDPartition {
  char* device;
  int size;
  int erasesize;
  char* name;
  int base;			/* Offset from the start of flash */

  static const int g_cPartitionMax = 32;
  static const int g_cbMax = 4096;

  static MTDPartition g_rg[g_cPartitionMax];
  static char g_rgb[g_cbMax];
  static void init (void);

  bool is (void) const {
    return device != NULL; }
  void zero (void) { bzero (this, sizeof (*this)); }
  MTDPartition () { zero (); }
  const char* dev_block (void) const {
    static char szDevice[80];
    snprintf (szDevice, sizeof (szDevice), "/dev/mtdblock%s", device+3);
    return szDevice; }
  const char* dev_char (void) const {
    static char szDevice[80];
    snprintf (szDevice, sizeof (szDevice), "/dev/mtd%s", device + 3);
    return szDevice; }

  static const MTDPartition find (unsigned long addr);
  static const MTDPartition find (const char*);
  static const MTDPartition first (void);

};




/* ----- Globals */

/* ----- Prototypes */



#endif  /* __MTDPARTITION_H__ */
