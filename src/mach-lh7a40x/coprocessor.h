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
 ({ __asm volatile ("sub pc, pc, #4\n\t"\
		    "nop\n\t"\
		    "nop\n\t"\
		    "nop\n\t"\
		    ); })

#endif  /* __COPROCESSOR_H__ */
