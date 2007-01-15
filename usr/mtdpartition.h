/* mtdpartition.h

   written by Marc Singer
   14 Jan 2007

   Copyright (C) 2007 Marc Singer

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

  bool is (void) {
    return device != NULL; }
  void zero (void) { bzero (this, sizeof (*this)); }
  MTDPartition () { zero (); }

  static const MTDPartition find (unsigned long addr);

};




/* ----- Globals */

/* ----- Prototypes */



#endif  /* __MTDPARTITION_H__ */
