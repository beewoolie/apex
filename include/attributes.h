/* attributes.h
     $Id$

   written by Marc Singer
   10 Feb 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__ATTRIBUTES_H__)
#    define   __ATTRIBUTES_H__

/* ----- Includes */

#include <apex/compiler.h>

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */


#define __weak __attribute__((weak))
#define __naked __attribute__((naked))
# define __used __attribute_used__
#define __section(s) __attribute__((section(#s)))
#define __irq_handler __attribute__((interrupt ("IRQ")))
#define __fiq_handler __attribute__((interrupt ("FIQ")))
#define __swi_handler __attribute__((interrupt ("SWI")))
#define __abort_handler __attribute__((interrupt ("ABORT")))
#define __undef_handler __attribute__((interrupt ("UNDEF")))

#endif  /* __ATTRIBUTES_H__ */
