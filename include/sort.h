/* sort.h

   written by Marc Singer
   17 May 2005

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

   Linkage for kernel's sort routine.  We import it for the sake of
   drv-jffs2.

*/

#if !defined (__SORT_H__)
#    define   __SORT_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

extern void sort (void *base, size_t num, size_t size,
		  int (*cmp)(const void *, const void *),
		  void (*swap)(void *, void *, int size));

#endif  /* __SORT_H__ */
