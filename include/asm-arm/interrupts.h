/* interrupts.h
     $Id$

   written by Marc Singer
   21 Jan 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

   Support for interrupt handlers.

*/

#if !defined (__INTERRUPTS_H__)
#    define   __INTERRUPTS_H__

/* ----- Includes */

/* ----- Types */

enum {
  IRQ_NONE = 0,
  IRQ_HANDLED = 1,
};

typedef int irq_return_t;

/* ----- Globals */

extern irq_return_t (*interrupt_handlers[])(int irq);

/* ----- Prototypes */



#endif  /* __INTERRUPTS_H__ */
