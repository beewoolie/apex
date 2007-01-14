/* coprocessor.h

   written by Marc Singer
   8 Apr 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__COPROCESSOR_H__)
#    define   __COPROCESSOR_H__

/* This coprocessor wait is require on the XScale when it is necessary
   to guarantee that the coprocessor has finished before continuing.
   AFAIK, this is not required on other ARM architectures. */

#define COPROCESSOR_WAIT\
 ({ unsigned long v; \
    __asm volatile ("mrc p15, 0, %0, c2, c0, 0\n\t" \
		    "mov %0, %0\n\t" \
		    "sub pc, pc, #4" : "=r" (v)); })

#endif  /* __COPROCESSOR_H__ */
