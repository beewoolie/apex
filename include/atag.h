/* atag.h
     $Id$

   written by Marc Singer
   6 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__ATAG_H__)
#    define   __ATAG_H__

/* ----- Includes */

#include <linux/types.h>
#include <asm-arm/setup.h>

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

struct atag_d {
  struct tag* (*append_atag) (struct tag*);
};


#define __atag_0 __attribute__((used,section(".atag_0")))
#define __atag_1 __attribute__((used,section(".atag_1")))
#define __atag_2 __attribute__((used,section(".atag_2")))
#define __atag_3 __attribute__((used,section(".atag_2")))
#define __atag_4 __attribute__((used,section(".atag_2")))
#define __atag_5 __attribute__((used,section(".atag_2")))
#define __atag_6 __attribute__((used,section(".atag_2")))
#define __atag_7 __attribute__((used,section(".atag_2")))

#endif  /* __ATAG_H__ */
