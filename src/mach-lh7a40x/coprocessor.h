/* coprocessor.h
     $Id$

   written by Marc Singer
   8 Apr 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__COPROCESSOR_H__)
#    define   __COPROCESSOR_H__

#define COPROCESSOR_WAIT\
 ({ unsigned long v; \
    __asm volatile ("mrc p15, 0, %0, c2, c0, 0\n\t" \
		    "mov %0, %0\n\t" \
		    "sub pc, pc, #4" : "=r" (v)); })

/* There is no requirement for stalling the CPU after a CP instruction. */
//#define COPROCESSOR_WAIT

#endif  /* __COPROCESSOR_H__ */
