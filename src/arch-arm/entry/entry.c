/* entry.c

   written by Marc Singer
   28 October 2004

   Default entry for APEX loader on ARM.  Most of the symbols herein
   are weak and may be overridden by target specific implementations.
   Refer to the documentation for details.

*/

#include <config.h>
#include "bootstrap.h"

extern void reset (void);
extern void exception_error (void);
extern void initialize_bootstrap (void);
extern void initialize_target (void);
extern void initialize (void);
extern void relocate_apex (void);
extern void setup_c (void);
extern void init (void);


/* exception_vectors

   These are the vectored 

*/

void __naked __section(.entry) exception_vectors (void)
{
  __asm ("b reset\n\t"
	 "b exception_error\n\t"
	 "b exception_error\n\t"
	 "b exception_error\n\t"
	 "b exception_error\n\t"
	 "b exception_error\n\t"
	 "b exception_error\n\t"
	 "b exception_error\n\t");
}
/* reset

   implements the reset exception vector.  All symbols but the init ()
   call must not depend on a stack or any RAM whatsoever.

*/

void __naked __section (.bootstrap) reset (void)
{
#if 0
				/* Disable MMU */
  __asm volatile ("mrc p15, 0, r0, c1, c0, 0\n\t"
		  "bic r0, r0, #1\n\t"
		  "mcr p15, 0, r0, c1, c0, 0");
#endif

  initialize_bootstrap ();
  relocate_apex ();
  initialize_target ();
  setup_c ();

	/* Start loader proper which doesn't return */
  __asm volatile ("mov pc, %0" :: "r" (&init)); 
}


/* exception_error

   where bad exceptions go to fail.

*/

void __naked __section (.bootstrap) exception_error (void)
{
  while (1)
    ;
}


/* setup_c

   performs setup necessary to let standard C (APCS) function, a
   stack, a clear BSS, data variables in RAM. 

*/

void __naked __section (.text) setup_c (void)
{
	/* Setup stack, quite trivial */
  __asm ("mov sp, %0" :: "r" (&APEX_STACK_START));

	/* Clear BSS */
  if (&APEX_BSS_START != &APEX_BSS_END)
    __asm (
	   "0: stmia %0!, {%2}\n\t"
	   "   cmp %0, %1\n\t"
	   "   ble 0b\n\t"
	   : : "r" (&APEX_BSS_START), "r" (&APEX_BSS_END), "r" (0));

  __asm ("mov pc, lr");
}

void __div0 (void)
{
  while (1)
    ;
}
