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

};




/* ----- Globals */

/* ----- Prototypes */



#endif  /* __MTDPARTITION_H__ */
