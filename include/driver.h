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
#define DRIVER_PRESENT	(1<<8)

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
  void (*info)(void);		/* User information request */
};

#define __driver  __attribute__((used,section(".driver")))
#define __driver_0 __attribute__((used,section(".driver_0")))
#define __driver_1 __attribute__((used,section(".driver_1")))
#define __driver_2 __attribute__((used,section(".driver_2")))
#define __driver_3 __attribute__((used,section(".driver_2")))

/* ----- Globals */

/* ----- Prototypes */



#endif  /* __DRIVER_H__ */
