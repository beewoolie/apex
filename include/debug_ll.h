/* debug_ll.h
     $Id$

   written by Marc Singer
   12 Feb 2005

   Copyright (C) 2005 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__INCLUDE_DEBUG_LL_H__)
#    define   __INCLUDE_DEBUG_LL_H__

/* ----- Includes */

#include <config.h>

#if defined (CONFIG_DEBUG_LL)
# include <mach/debug_ll.h>

# define PUTHEX_LL(value) ({ unsigned long v = (unsigned long) value; \
			     int i; unsigned char ch; \
			     for (i = 8; i--; ) {\
			     ch = ((v >> (i*4)) & 0xf);\
			     ch += (ch >= 10) ? 'a' - 10 : '0';\
			     PUTC_LL (ch); }})

#else
# define PUTC_LL(c) do {} while (0)
# define PUTHEX_LL(v) do {} while (0)
#endif

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */



#endif  /* __INCLUDE_DEBUG_LL_H__ */
