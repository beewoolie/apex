/* files.h					-*- C++ -*-
     $Id: files.h,v 1.9 2003/08/11 07:45:04 elf Exp $
   
   written by Marc Signer (aka Oscar Levi)
   10 Dec 1998

   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (c) 1998,2003 Marc Singer

   This program is free software; you can redistribute it and/or modify 
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA

   -----------
   DESCRIPTION
   -----------


*/

#if !defined (__FILES_H__)
#    define   __FILES_H__

/* ----- Includes */

/* ----- Globals */


bool dup_status (const char* szFileNew, const char* szFile);
//bool is_file (const char* szPath);
//bool is_symlink (const char* szPath);
char* path_of (const char* szPath);
bool replace_file (const char* szFileOutput, const char* szFile);
char* resolve_link (const char* szPathOriginal);

#endif  /* __FILES_H__ */
