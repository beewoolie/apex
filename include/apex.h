/* apex.h
     $Id$

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__INIT_H__)
#    define   __INIT_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

extern void init_drivers (void);
extern int __attribute__ ((format (printf, 1, 2)))
     printf (const char * fmt, ...);
extern int puts (const char * fmt);
extern int snprintf(char * buf, size_t size, const char * fmt, ...);


#endif  /* __INIT_H__ */
