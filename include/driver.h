/* driver.h
     $Id$

   written by Marc Singer
   1 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__DRIVER_H__)
#    define   __DRIVER_H__

/* ----- Includes */

#include <linux/types.h>

/* ----- Types */

#define DRIVER_SERIAL	(1<<1)
#define DRIVER_CONSOLE	(1<<2)
#define DRIVER_MEMORY	(1<<3)

struct driver_d {
  const char* name;
  const char* description;
  unsigned long flags;
  void* priv;			/* Driver's private data */
  int (*probe)(void);
  unsigned long (*open)(const char*);
  ssize_t (*read)(unsigned long fd, void* pv, size_t cb);
  ssize_t (*write)(unsigned long fd, const void* pv, size_t cb);
  size_t (*seek)(unsigned long fd, ssize_t cb, int whence);
};

#define __driver_init __attribute__((used,section(".driver_init")))

/* ----- Globals */

/* ----- Prototypes */



#endif  /* __DRIVER_H__ */
