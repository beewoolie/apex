/* ethernet.h
     $Id$

   written by Marc Singer
   7 Jul 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__ETHERNET_H__)
#    define   __ETHERNET_H__

/* ----- Includes */

#include <driver.h>

/* ----- Types */

#define FRAME_LENGTH_MAX	1536

struct ethernet_frame {
  size_t cb;
  int state;
  char rgb[FRAME_LENGTH_MAX];
};

/* ----- Globals */

/* ----- Prototypes */

struct ethernet_frame* ethernet_frame_allocate (void);
void ethernet_frame_release (struct ethernet_frame*);
void ethernet_receive (struct descriptor_d*, struct ethernet_frame*);

void udp_setup (struct ethernet_frame*, const char*, u16, u16, size_t);


#endif  /* __ETHERNET_H__ */
