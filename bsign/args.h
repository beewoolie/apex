/* args.h		-*- c++ -*-
     $Id: args.h,v 1.5 2003/08/21 05:06:20 elf Exp $

   written by Marc Singer
   10 Aug 2003

   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (c) 1998,2003 Marc Signer

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
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA
   02111-1307, USA

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__ARGS_H__)
#    define   __ARGS_H__

/* ----- Includes */

#include "filewalk.h"

/* ----- Types */

enum {
  modeError = -1,		// Option parsing failed
  modeCheck = 0,		// Check hash, (default)
  modeVerify,			// Verify signature
  modeSign,			// Embed signature
  modeHash,			// Embed hash
  modeExtract,			// Extract signature
  modeInfo,			// Show info about signature
  modeUnsigned,			// Look for unsigned files
  modeVersion,			// Show application version & copyright
  modeRemove,			// Remove signatures by KEYID
};

struct string_list_element {
  string_list_element* pNext;
  const char* sz;
};

struct arguments {
  int  mode;			// Operational mode
  bool fNoSymLinks;	        // Treat symlinks as invalid file format
  bool fForceResign;		// Resign files that are already signed
  bool fIgnore;			// Ignore directories and non-ELF in error msgs
  bool fSummary;		// Show processing summary at end
  bool fHideGoodSigs;		// Hide files with good signatures from output
  bool fQuiet;			// Inhibit messages, return code only
  bool fDebug;			// Show debug messages
  bool fVerbose;		// Verbose reports 
  bool fExpectSignature;	// Mode expects a signature
  bool fInverseResult;	        // Negate result for benefit of find program
  bool fAddSignature;		// Add new signature

  const char* szOutput;		// Name of output file
  const char* szPGOptions;	// Options to pass to privacy guard program
  const char* szApplication;	// Name of the application

  string_list_element* pKeyId;	// List of IDs, used when removing sigs 

};

/* ----- Globals */

extern FileWalk g_filewalk;		// Global file walking object
extern struct arguments g_arguments;

/* ----- Prototypes */

void parse_arguments (struct arguments& arguments, int argc, char** argv);


#endif  /* __ARGS_H__ */
