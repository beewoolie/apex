/* command.h
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

#if !defined (__COMMAND_H__)
#    define   __COMMAND_H__

/* ----- Includes */

/* ----- Types */

struct command_d {
  const char* command;
  const char* description;
  const char* help;
  int cbPrefix;			/* Length of prefix (?) */
  int (*func)(int argc, const char** argv);
};

#define __command __attribute__((used,section("command")))


/* ----- Globals */

/* ----- Prototypes */



#endif  /* __COMMAND_H__ */
