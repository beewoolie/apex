/* bootstrap.h
     $Id$

   written by Marc Singer
   9 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__BOOTSTRAP_H__)
#    define   __BOOTSTRAP_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

extern char APEX_VMA_COPY_START;
extern char APEX_VMA_COPY_END;
extern char APEX_VMA_STACK_START;
extern char APEX_VMA_BSS_START;
extern char APEX_VMA_BSS_END;

/* ----- Prototypes */

#define __weak __attribute__((weak))
#define __naked __attribute__((naked))
#define __section(s) __attribute__((section(#s)))


#endif  /* __BOOTSTRAP_H__ */
