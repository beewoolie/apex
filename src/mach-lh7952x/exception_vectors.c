/* exception_vectors.c
     $Id$

   written by Marc Singer
   15 Nov 2004

   Copyright (C) 2004 Marc Singer

   -----------
   DESCRIPTION
   -----------

   These exception vectors are the defaults for the platform.  A
   machine implementation may override them by defining the
   exception_vectors() function.

   NOTES
   -----

   Since we are loaded at some arbitrary address and we don't like
   hard-coding things, we have to do a little work to get the vectors
   to work.

   We map SDRAM at 0 and create vectors there.

   Sections .vector.? are copied to 0x0.  The vectoring code will jump
   back to the loader for anything substantial.  This code *must*
   *must* *must* be smaller than 256 bytes so it doesn't get stomped
   by the ATAGS structures.

*/

#include <config.h>
#include <apex.h>
#include <asm/bootstrap.h>
#include <asm/system.h>
#include <service.h>
#include "hardware.h"

extern void reset (void);
extern void irq_handler (void);

void (*interrupt_handlers[32])(void);

void __naked __section (.vector.0) exception_vectors (void)
{
  __asm ("b v_reset\n\t"		/* reset */
	 "b v_exception_error\n\t"	/* undefined instruction */
	 "b v_exception_error\n\t"	/* software interrupt (SWI) */
	 "b v_exception_error\n\t"	/* prefetch abort */
	 "b v_exception_error\n\t"	/* data abort */
	 "b v_exception_error\n\t"	/* (reserved) */
	 "b v_irq_handler\n\t"		/* irq (interrupt) */
	 "b v_exception_error\n\t"	/* fiq (fast interrupt) */
	 );
}

void __naked __section (.vector.1) v_reset (void)
{
	/* Disable MMU */
  __asm volatile ("mrc p15, 0, r0, c1, c0, 0\n\t"
		  "bic r0, r0, #1\n\t"
		  "mcr p15, 0, r0, c1, c0, 0");


	/* Reenter the loader */
  __asm volatile ("mov pc, %0" :: "r" (reset));
}

void __naked __section (.vector.1) v_exception_error (void)
{
	/* *** FIXME: this might be a good place to reset the system */
  while (1)
    ;
}

void __naked __section (.vector.1) v_irq_handler (void)
{
  __asm volatile ("mov pc, %0" :: "r" (irq_handler));
}

void __irq_handler __section (.bootstrap) irq_handler (void)
{
  int i;
  unsigned long v = __REG (VIC_PHYS + VIC_IRQSTATUS);
  if (v == 0)
    return;

  for (i = 0; i; ++i) {
    if (v & (1<<i)) {
      if (interrupt_handlers[i])
	interrupt_handlers[i]();
      __REG (VIC_PHYS + VIC_VECTADDR) = 0;
      break;
    }    
  }
}

void lh7952x_exception_init (void)
{
  __REG (RCPC_PHYS + RCPC_CTRL)       |= RCPC_CTRL_UNLOCK;
  __REG (RCPC_PHYS + RCPC_REMAP) = 0x1;	/* SDRAM at 0x0 */
  __REG (RCPC_PHYS + RCPC_CTRL)       &= ~RCPC_CTRL_UNLOCK;

	/* Clear V for exception vectors at 0x0 */
  { 
    unsigned long l;
    __asm volatile ("mrs %0, cpsr\n\t"
		    "bic %0, %0, #(1<<13)\n\t"
		    "msr cpsr, %0" : "=r" (l));
  }
	/* Install vectors.
	   I didn't use memcpy because the target address is 0! */
  __asm volatile (
	       "0: ldmia %1!, {r3}\n\t"
		  "stmia %0!, {r3}\n\t"
		  "cmp %1, %2\n\t"
		  "bne 0b\n\t"
		  :
		  : "r" (0), 
		    "r" (&APEX_VMA_VECTOR_START),
		    "r" (&APEX_VMA_VECTOR_END)
		  : "r3"
		  );		  

  printf ("exceptions %p %p\r\n", 
	  &APEX_VMA_VECTOR_START, &APEX_VMA_VECTOR_END);

  __REG (VIC_PHYS + VIC_INTSELECT) = 0;
  __REG (VIC_PHYS + VIC_INTENABLE) = 0;
  __REG (VIC_PHYS + VIC_INTENCLEAR) = ~0;
  __REG (VIC_PHYS + VIC_SOFTINT_CLEAR) = ~0;

		/* Initialize interrupt stack */
  __asm volatile ("mrs r2, cpsr\n\t"
		  "mov r3, r2\n\t"
		  "bic r3, r3, #0b11111\n\t"
		  "orr r3, r3, #0b10010\n\t"
		  "msr cpsr_c, r3\n\t"
		  "mov sp, %0\n\t"
		  "msr cpsr_c, r2\n\t"
		  :
		  : "r" (&APEX_VMA_IRQSTACK_START)
		  : "r2","r3"
		  );

//  local_irq_enable ();
}

void lh7952x_exception_release (void)
{
  __REG (VIC_PHYS + VIC_INTENABLE) = 0;

  local_irq_disable ();
}

static __service_1 struct service_d lh7952x_exception_service = {
  .init    = lh7952x_exception_init,
  .release = lh7952x_exception_release,
};
