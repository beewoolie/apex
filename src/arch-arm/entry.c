/* entry.c

   written by Marc Singer
   28 October 2004

   Default entry for APEX loader on ARM.  Most of the symbols herein
   are weak and may be overridden by target specific implementations.
   Refer to the documentation for details.

*/

#if defined (HAVE_CONFIG_H)
# include <config.h>
#endif

extern unsigned long APEX_LMA_START;
extern unsigned long APEX_LMA_END;
extern unsigned long APEX_VMA_START;
extern unsigned long APEX_STACK_START;
extern unsigned long APEX_BSS_START;
extern unsigned long APEX_BSS_END;

#define __weak __attribute__((weak))
#define __naked __attribute__((naked))
#define __section(s) __attribute__((section(#s)))

extern void __weak reset (void);
extern void __weak exception_error (void);
extern void initialize_bootstrap (void);
extern void initialize (void);
extern void __weak relocate_apex (void);
extern void __weak setup_c (void);
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
  initialize_bootstrap ();
  relocate_apex ();
  initialize_target ();
  setup_c ();
  init ();			/* Start the loader, doesn't return */
}


/* exception_error

   where bad exceptions go to fail.

*/

void __naked __section (.bootstrap) exception_error (void)
{
  while (1)
    ;
}


/* relocate_apex

   performs a simple memory move of the whole loader, presumed to be
   from NOR flash into SDRAM.  The controlling symbols come from the
   loader link map.

*/

void __naked __section (.bootstrap) relocate_apex (void)
{
  __asm (
      "0: ldmia %0!, {r3-r10}\n\t"
	 "stmia %1!, {r3-r10}\n\t"
	 "cmp %0, %2\n\t"
	 "ble 0b\n\t"
	 "add pc, lr, %3\n\t"
	 : 
      : "r" (&APEX_LMA_START), "r" (&APEX_VMA_START), "r" (&APEX_LMA_END),
	"r" (&APEX_VMA_START - &APEX_LMA_START)
      : "r3", "r4", "r5", "r6", "r7", "r8", "r9", "r10" 
	 );
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
  __asm (
	 "0: stmia %0!, {%2}\n\t"
	 "   cmp %0, %1\n\t"
	 "   ble 0b\n\t"
	 : : "r" (&APEX_BSS_START), "r" (&APEX_BSS_END), "r" (0));

  __asm ("mov pc, lr");
}
