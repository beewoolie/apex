/* memtest.h

   written by Marc Singer
   7 Nov 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__MEMTEST_H__)
#    define   __MEMTEST_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

int __naked __section (.bootstrap) memory_test_0 (unsigned long address,
						  unsigned long cb);

#endif  /* __MEMTEST_H__ */
