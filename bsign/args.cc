/* args.cc
     $Id: args.cc,v 1.5 2003/08/21 05:06:20 elf Exp $

   written by Marc Singer
   10 Aug 2003

   This file is part of the project BSIGN.  See the file README for
   more information.

   Copyright (c) 1998,2003 Marc Singer

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

#include "standard.h"

#if defined (HAVE_ARGP_H)
# include <argp.h>
#endif

#include "args.h"
#include "_version.h"

#include "exitstatus.h"

enum {
  keyForceResign = 1024,
  keyNoSymLinks,
  keyVersion,
};

#if defined (HAVE_ARGP_H)
static struct argp_option options[] = {
  { "sign", 's', NULL, 0, "Rewrite files with hash and signature" },
  { "remove-signature", 'R', "KEYID", 0, "Remove signatures matching KEYID."
			"  May appear more than once to specify more than one"
			" KEYID." },
  { "hash", 'H', NULL, 0, "Rewrite files with hash only" },
  { "check-hash", 'c', NULL, 0, "Check hash" },
  { "show-info", 'w', NULL, 0, "Describe signature information" },
  { "extract-sig", 'x', NULL, 0, "Extract digital signature" },
  { "verify", 'V', NULL, 0, "Verify signatures of input files" },
  { "force-resign", keyForceResign, NULL, 0,
			"Force new signatures for already signed files" },

  { "pgoptions", 'p', "OPTS", 0, "Pass options to the privacy guard program" },

  { "output", 'o', "FILE", 0, "Save rewritten file to FILE", 10 }, 

  { "include", 'i', "PATH", 0, "Add PATH to the list of paths to enumerate", 
    10 },
  { "exclude", 'e', "PATH", 0, "Add PATH to the list of paths to exclude" },
  { "nosymlinks", keyNoSymLinks, NULL, 0, 
			"Treat symlinks as an unsupported file type" },
  { "files", 'f', "FILE", 0, "Process filenames from FILE, one per line."
			"  Use - for standard input." },
  { "ignore-unsupported", 'I', NULL, 0, "Ignore directories and non-ELF files"
			" in error messages" },

  { "add-signature", 'a', NULL, 0, "Add new signature instead of replacing" },

  { "debug", 'd', NULL, 0, "Show debug messages", 40 },
  { "quiet", 'q', NULL, 0, 
			 "Inhibit messages, caller must rely on exit status" },
  { "verbose", 'v', NULL, 0, "Be verbose in reporting program state" },


  { "summary", 'S', NULL, 0, 
			"Print processing summary after last input file" },
  { "hide-good-sigs", 'G', NULL, 0, "Do not report when good signatures" },

  { "version", keyVersion, NULL, 0, "Show program version & copyright", 100},

  { 0 },
};

static error_t parse_options (int key, char* arg, struct argp_state* pState)
{
  struct arguments& arguments = *(struct arguments*) pState->input;

  switch (key) {
  case 's':
    arguments.mode = modeSign;
    break;

  case 'H':
    arguments.mode = modeHash;
    break;

  case 'c':
    arguments.mode = modeCheck;
    break;

  case 'w':
    arguments.mode = modeInfo;
    break;

  case 'x':
    arguments.mode = modeExtract;
    break;

  case 'V':
    arguments.mode = modeVerify;
    break;

  case 'R':
    arguments.mode = modeRemove;
    {
      string_list_element* p = new string_list_element;
      p->pNext = arguments.pKeyId;
      p->sz = arg;
      arguments.pKeyId = p;
    }
    break;

  case 'S':
    arguments.fSummary = true;
    break;

  case 'I':
    arguments.fIgnore = true;
    break;

  case 'a':
    arguments.fAddSignature = true;
    break;

  case 'G':
    arguments.fHideGoodSigs = true;
    break;

  case 'v':
    arguments.fVerbose = true;
    break;

  case 'd':
    arguments.fDebug = true;
    break;

  case 'q':
    arguments.fQuiet = true;
    break;

  case keyNoSymLinks:
    arguments.fNoSymLinks = true;
    break;

  case keyForceResign:
    arguments.fForceResign = true;
    break;

  case 'o':
    arguments.szOutput = arg;
    break;

  case 'f':
    g_filewalk.source (arg);
    break;

  case 'p':
    arguments.szPGOptions = arg;
    break;

  case 'i':
    g_filewalk.include (arg);
    break;

  case 'e':
    g_filewalk.exclude (arg);
    break;

  case 'u':
    arguments.mode = modeUnsigned;
    break;

  case keyVersion:
    arguments.mode = modeVersion;
    break;

  case ARGP_KEY_ARG:
    g_filewalk.insert (arg);
    break;

  case ARGP_KEY_END:
    arguments.fExpectSignature 
      = (arguments.mode == modeVerify)
      | (arguments.mode == modeUnsigned);
    arguments.fInverseResult = (arguments.mode == modeUnsigned);

    if (arguments.mode == modeExtract)
      arguments.fQuiet = true;

    if (arguments.mode != modeVersion && !g_filewalk.is_ready ())
      set_exitstatus (invalidargument, 
		      "use only one method for specifying input files."); 
    
    if (arguments.szOutput && !g_filewalk.is_only_one ())
      set_exitstatus (invalidargument, 
		      "--output option requires one"
		      " and only one input filename");

    if (arguments.pKeyId &&  arguments.mode != modeRemove)
      set_exitstatus (invalidargument, 
		      "--remove-signature cannot be mixed with other"
		      " operational modes");

    break;
  }

  return 0;
}

static struct argp argp = { options, parse_options, 
                            "[FILE]...", 
                            "Embed/verify secure hashes and digital signatures"
			    " in executable files." };

#endif

static const char* parse_application (char* pch)
{
  char* pchSep    = rindex (pch, '\\');
  char* pchSepAlt = rindex (pch, '/');
  char* pchColon  = rindex (pch, ':');
  char* pchDot    = rindex (pch, '.');

  if (pchSepAlt > pchSep)
    pchSep = pchSepAlt;
  if (pchColon > pchSep)
    pchSep = pchColon;
  pch = pchSep ? pchSep + 1 : pch;

  if (pchDot && strcasecmp (pch, ".exe"))
    *pchDot = 0;
  return pch;
}


void parse_arguments (struct arguments& arguments, int argc, char** argv)
{
  bzero (&arguments, sizeof (arguments));
  // *** FIXME: set defaults

  arguments.szApplication = parse_application (argv[0]);

#if defined (HAVE_ARGP_H)
  argp_parse (&argp, argc, argv, 0, 0, &arguments);
#endif
}
