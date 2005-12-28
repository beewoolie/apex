/* exception_vectors.c
     $Id$

   written by Marc Singer
   15 Nov 2004

   Copyright (C) 2004 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.

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
   by the ATAGS structures.  It must also make sure not to modify
   registers before pushing onto the exception stack.

*/

#include <config.h>
#include <apex.h>
#include <asm/bootstrap.h>
#include <asm/system.h>
#include <service.h>
#include "hardware.h"
#include <asm/interrupts.h>
#include <debug_ll.h>

extern void reset (void);
extern void irq_handler (void);

#define UART_DR		__REG(UART + 0x00)

irq_return_t (*interrupt_handlers[32])(int irq);

void __naked __section (.vector.0) exception_vectors (void)
{
  __asm ("b v_reset\n\t"		/* reset */
	 "b v_undef_error\n\t"	/* undefined instruction */
	 "b v_exception_error\n\t"	/* software interrupt (SWI) */
	 "b v_prefetch_error\n\t"	/* prefetch abort */
	 "b v_abort_error\n\t"	/* data abort */
	 "b v_exception_error\n\t"	/* (reserved) */
	 "b v_irq_handler\n\t"		/* irq (interrupt) */
//	 "b v_exception_error\n\t"	/* irq (interrupt) */
	 "b v_exception_error\n\t"	/* fiq (fast interrupt) */
	 );
}

void __naked __section (.vector.1) v_irq_handler (void)
{
  __asm volatile ("ldr pc, =irq_handler"); /* Trampoline */
}

void __naked __section (.vector.1) v_reset (void)
{
  PUTC_LL ('x');
  PUTC_LL ('R');

  while (1)
    ;

	/* Disable MMU */
  /* *** FIXME: we only need to disable the mmu if we've enabled it.  */
  { 
    unsigned long l;
    __asm volatile ("mrc p15, 0, %0, c1, c0, 0\n\t"
		    "bic r0, %0, #1\n\t"
		    "mcr p15, 0, %0, c1, c0, 0" : "=&r" (l));
  }


	/* Reenter the loader */
  __asm volatile ("mov pc, %0" :: "r" (reset));
}

void __naked __section (.vector.1) v_undef_error (void)
{
  PUTC_LL ('x');
  PUTC_LL ('U');

  while (1)
    ;
}

void __naked __section (.vector.1) v_prefetch_error (void)
{
  PUTC_LL ('x');
  PUTC_LL ('P');

  while (1)
    ;
}

void __naked __section (.vector.1) v_abort_error (void)
{
  unsigned long far;
  unsigned long fsr;

  PUTC_LL ('x');
  PUTC_LL ('A');

  __asm volatile ("mrc p15, 0, %0, c5, c0" : "=r" (fsr));
  __asm volatile ("mrc p15, 0, %0, c6, c0" : "=r" (far));

  PUTHEX_LL (fsr);
  PUTC_LL (':');
  PUTHEX_LL (far);

  while (1)
    ;
}

void __naked __section (.vector.1) v_exception_error (void)
{
  UART_DR = 'x';
  UART_DR = 'E';

	/* *** FIXME: this might be a good place to reset the system */
  while (1)
    ;
}

/* *** FIXME: this handler should not be in the bootstrap as this is
   *** the portion that must fit in the first block of the loader.  In
   *** fact, it should probably be in text. */

void __irq_handler irq_handler (void)
{
#define C_LR	(4096)
  static unsigned long irq_lr[C_LR];
//  static unsigned long irq_lr; 
  static unsigned long irq_c;
  unsigned long v;
  int irq;

  __asm volatile ("mov %0, lr, lsr #16\n\t"
		  "cmp %0, %1\n\t"
		  "0: bne 0" : "=&r" (v) : "r" (0x2020));


//  __asm volatile ("ldr %0, [sp, #-1]" : "=r" (irq_lr));
  __asm volatile ("str lr, [%0]" :: "r" (irq_lr + (irq_c%C_LR)));
  ++irq_c;
  barrier ();

  v = VIC_IRQSTATUS;

  VIC_VECTADDR;		/* Tell priority hardware we're processing */

  /* *** FIXME: Now, I should be able to reenable interrupts so that
     the higher priority interrupts can be handled. */

  for (irq = 0; irq < 32; ++irq) {
    if (v & (1<<irq)) {
      if (  !interrupt_handlers[irq]
	  || interrupt_handlers[irq](irq) != IRQ_HANDLED) {
	VIC_INTENCLEAR = (1<<irq);
      }
      break;
    }    
  }

  VIC_VECTADDR = 0;
}

static void lh7952x_exception_init (void)
{
  RCPC_CTRL  |=  RCPC_CTRL_UNLOCK;
  RCPC_REMAP  =  0x1;	/* SDRAM at 0x0 */
  RCPC_CTRL  &= ~RCPC_CTRL_UNLOCK;

	/* Clear V for exception vectors at 0x0 */
  { 
    unsigned long l;
    __asm volatile ("mrs %0, cpsr\n\t"
		    "bic %0, %0, #(1<<13)\n\t"
		    "msr cpsr, %0" : "=&r" (l));
  }
	/* Install vectors.
	   I didn't use memcpy because the target address is 0--memcpy
	   fails to copy to 0x0. */
  {
    unsigned long v = 0;
    unsigned long t;
    void* p = &APEX_VMA_VECTOR_START;
    __asm volatile (
		 "0: ldmia %1!, {%2}\n\t"
		    "stmia %0!, {%2}\n\t"
		    "cmp %1, %3\n\t"
		    "bne 0b\n\t"
		    : "+r" (v), "+r" (p), "=&r" (t)
		    : "r" (&APEX_VMA_VECTOR_END)
		    : "cc"
		  );		  
  }

  VIC_INTENCLEAR = ~0;
  VIC_SOFTINT_CLEAR = ~0;
  VIC_DEFVECTADDR = (unsigned long) irq_handler;

  VIC_INTSELECT = 0;
  VIC_VECTADDR;
  VIC_VECTADDR = 0;

		/* Initialize interrupt stack */
  {
    unsigned long cpsr_save;
    unsigned long cpsr;
    __asm volatile ("mrs %0, cpsr\n\t"
		    "bic %1, %0, #0b11111\n\t"
		    "orr %1, %1, #0b10010\n\t"
		    "msr cpsr_c, %1\n\t"
		    "mov sp, %2\n\t"
		    "msr cpsr_c, %0\n\t"
		    : "=&r" (cpsr_save), "=&r" (cpsr)
		    : "r" (&APEX_VMA_IRQSTACK_START)
		    );
  }

  local_irq_enable ();
}

static void lh7952x_exception_release (void)
{
  local_irq_disable ();
  VIC_INTENABLE = 0;
}

static void lh7952x_exception_report (void)
{
  printf ("  except: select 0x%lx  enable 0x%lx  status 0x%lx  raw 0x%lx"
	  "  itop 0x%lx\n", 
	  VIC_INTSELECT, VIC_INTENABLE, VIC_IRQSTATUS, VIC_RAWINTSR, VIC_ITOP);
  printf ("          vectors %p %p, %d bytes\n", 
	  &APEX_VMA_VECTOR_START, &APEX_VMA_VECTOR_END, 
	  &APEX_VMA_VECTOR_END - &APEX_VMA_VECTOR_START);
}

static __service_1 struct service_d lh7952x_exception_service = {
  .init    = lh7952x_exception_init,
  .release = lh7952x_exception_release,
  .report  = lh7952x_exception_report,
};
