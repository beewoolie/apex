/* bootstrap.h
     $Id$

   written by Marc Singer
   9 Nov 2004

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

*/

#if !defined (__BOOTSTRAP_H__)
#    define   __BOOTSTRAP_H__

/* ----- Includes */

#include <attributes.h>

/* ----- Types */

/* ----- Globals */

extern char APEX_VMA_ENTRY;
extern char APEX_VMA_VECTOR_START;
extern char APEX_VMA_VECTOR_END;
extern char APEX_VMA_COPY_START;
extern char APEX_VMA_COPY_END;
extern char APEX_VMA_STACKS_START;
extern char APEX_VMA_STACKS_END;
extern char APEX_VMA_STACK_START;
extern char APEX_VMA_IRQSTACK_START;
extern char APEX_VMA_BSS_START;
extern char APEX_VMA_BSS_END;
extern char APEX_VMA_BOOTSTRAP_STACK_START;

/* ----- Prototypes */


#endif  /* __BOOTSTRAP_H__ */
