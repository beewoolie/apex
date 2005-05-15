/* spinner.h
     $Id$

   written by Marc Singer
   8 Nov 2004

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

#if !defined (__SPINNER_H__)
#    define   __SPINNER_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#if defined (CONFIG_SPINNER)
# define SPINNER_STEP spinner_step()
# define SPINNER_CLEAR spinner_clear()
#else
# define SPINNER_STEP
# define SPINNER_CLEAR
#endif

extern void (*hook_spinner) (unsigned value);

void spinner_step (void);
void spinner_clear (void);

#endif  /* __SPINNER_H__ */
