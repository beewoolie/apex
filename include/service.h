/* service.h
     $Id$

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__SERVICE_H___)
#    define   __SERVICE_H___

/* ----- Includes */

/* ----- Types */

struct service_d {
  void (*init) (void);
  void (*release) (void);
};

#define __service  __attribute__((used,section(".service")))
#define __service_0 __attribute__((used,section(".service_0")))
#define __service_1 __attribute__((used,section(".service_1")))
#define __service_2 __attribute__((used,section(".service_2")))
#define __service_3 __attribute__((used,section(".service_3")))

/* ----- Globals */

/* ----- Prototypes */

#endif  /* __SERVICE_H___ */
