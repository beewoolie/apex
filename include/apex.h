/* apex.h
     $Id$

   written by Marc Singer
   3 Nov 2004

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

#if !defined (__INIT_H__)
#    define   __INIT_H__

/* ----- Includes */

#include <stdarg.h>
#include <apex/types.h>

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

#if !defined (barrier)
# define barrier() __asm volatile ("":::"memory")
#endif

extern void init_drivers (void);
extern int __attribute__((format (printf, 1, 2)))
     printf (const char * fmt, ...);
extern int putchar (int ch);
extern int puts (const char * fmt);
extern int snprintf(char * buf, size_t size, const char * fmt, ...);
extern int read_command (const char* szPrompt, 
			 int* pargc, const char*** pargv);
extern int parse_command (char* rgb, int* pargc, const char*** pargv);
extern int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);

extern void __attribute__((noreturn)) exec_monitor (void);

extern unsigned long timer_read (void);
extern unsigned long timer_delta (unsigned long, unsigned long);
extern void usleep (unsigned long); 

extern void dump (const unsigned char* rgb, int cb, unsigned long index);

#endif  /* __INIT_H__ */
