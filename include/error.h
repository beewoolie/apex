/* error.h
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

#if !defined (__ERROR_H__)
#    define   __ERROR_H__

extern const char* error_description;

#define ERROR_FALSE	(-1)
#define ERROR_IMPORTANT (-1)	/* Errors less than this will be reported */
#define ERROR_FAILURE	(-7)
#define ERROR_NOCOMMAND (-8)
#define ERROR_PARAM	(-9)
#define ERROR_OPEN	(-10)
#define ERROR_AMBIGUOUS (-11)
#define ERROR_NODRIVER	(-12)
#define ERROR_UNSUPPORTED (-13)
#define ERROR_BADPARTITION (-14)
#define ERROR_FILENOTFOUND (-15)
#define ERROR_IOFAILURE	   (-16)
#define ERROR_CRCFAILURE   (-17)

#if defined (CONFIG_SMALL)
# define ERROR_RETURN(v,m) return (v)
# define ERROR_RESULT(v,m) (v)
#else
# define ERROR_RETURN(v,m) ({ error_description = (m); return (v); })
# define ERROR_RESULT(v,m) ({ error_description = (m); (v); })
#endif

#endif  /* __ERROR_H__ */
