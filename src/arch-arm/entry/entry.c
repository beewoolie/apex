/* entry.c

   written by Marc Singer
   28 October 2004

   Default entry for APEX loader on ARM.  Most of the symbols herein
   are weak and may be overridden by target specific implementations.
   Refer to the documentation for details.

*/

#include <config.h>
#include <asm/bootstrap.h>

extern void reset (void);
extern void exception_error (void);
extern int  initialize_bootstrap (void);
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
  /* This would disable the MMU, but there is little reason to include
     it.  The only way this would be called would be if the CPU jumped
     to the loader while running something that needed the MMU.  In
     the unusual situation where the MMU maps the loader in a place
     where it can execute, this might make things work.  I'd rather
     wait for a legitimate use for this operation.  */
  __asm volatile ("mrc p15, 0, r0, c1, c0, 0\n\t"
		  "bic r0, r0, #1\n\t"
		  "mcr p15, 0, r0, c1, c0, 0");
#endif

  initialize_bootstrap ();	/* Initialization critical to relocate */
  relocate_apex ();
  initialize_target ();		/* Rest of platform initialization */
  setup_c ();			/* Setups before executing C code */

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
  __asm ("mov sp, %0" :: "r" (&APEX_VMA_STACK_START));

	/* Clear BSS */
  if (&APEX_VMA_BSS_START != &APEX_VMA_BSS_END)
    __asm (
	   "0: stmia %0!, {%2}\n\t"
	   "   cmp %0, %1\n\t"
	   "   ble 0b\n\t"
	   : : "r" (&APEX_VMA_BSS_START), "r" (&APEX_VMA_BSS_END), "r" (0));

  __asm ("mov pc, lr");
}

void __div0 (void)
{
  while (1)
    ;
}
