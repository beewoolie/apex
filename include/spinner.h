/* spinner.h
     $Id$

   written by Marc Singer
   8 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__SPINNER_H__)
#    define   __SPINNER_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#if defined (CONFIG_SPINNER)
# define SPINNER_STEP spinner_step()
#else
# define SPINNER_STEP
#endif

void spinner_step (void);

#endif  /* __SPINNER_H__ */
