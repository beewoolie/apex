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

#define __service_0 __attribute__((used,section(".service_0")))
#define __service_1 __attribute__((used,section(".service_1")))
#define __service_2 __attribute__((used,section(".service_2")))
#define __service_3 __attribute__((used,section(".service_3")))
#define __service_4 __attribute__((used,section(".service_4")))
#define __service_5 __attribute__((used,section(".service_5")))
#define __service_6 __attribute__((used,section(".service_6")))
#define __service_7 __attribute__((used,section(".service_7")))

/* ----- Globals */

/* ----- Prototypes */

extern void release_services (void);

#endif  /* __SERVICE_H___ */
