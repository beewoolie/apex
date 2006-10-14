/* iobarrier.h

   written by Marc Singer
   13 Oct 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__IOBARRIER_H__)
#    define   __IOBARRIER_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#if defined (CONFIG_MACH_COMPANION)
# define IOBARRIER_PHYS	0x70000000
#else
# define IOBARRIER_PHYS	0x10000000
#endif

#endif  /* __IOBARRIER_H__ */
