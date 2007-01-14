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

/* ----- Types */

#include <string.h>

struct MTDPartition {
  char* device;
  int size;
  int erasesize;
  char* name;

  static MTDPartition* g_rg;
  static void init (void);

  void zero (void) { bzero (this, sizeof (*this)); }
  MTDPartition () { zero (); }

  static const MTDPartition& find (unsigned long addr);

};


/* ----- Globals */

/* ----- Prototypes */



#endif  /* __MTDPARTITION_H__ */
