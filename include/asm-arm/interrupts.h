/* interrupts.h
     $Id$

   written by Marc Singer
   21 Jan 2005

   Copyright (C) 2005 Marc Singer

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
