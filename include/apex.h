/* apex.h
     $Id$

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 The Buici Company

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__INIT_H__)
#    define   __INIT_H__

/* ----- Includes */

#include <stdarg.h>
#include <linux/types.h>

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

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

#endif  /* __INIT_H__ */
