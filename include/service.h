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

#define __service     __attribute__((used,section(".service")))
#define __service_(v) __attribute__((used,section(".service" ## v)))

/* ----- Globals */

/* ----- Prototypes */

#endif  /* __SERVICE_H___ */
