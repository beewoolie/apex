/* main.cxx
     $Id: main.cc,v 1.59 2003/08/21 07:21:03 elf Exp $

   written by Marc Singer (aka Oscar Levi)
   1 December 1998
   
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

*/

  /* ----- Includes */

#include "standard.h"

#define DECLARE_VERSION
#include "version.h"

#include <sys/mman.h>
#include <sys/stat.h>
//#include <limits.h>
#include <errno.h>

#include "bsign.h"
#include "files.h"
#include "exitstatus.h"
#include "filewalk.h"
#include "args.h"
#include "rfc2440.h"


extern "C" void sha_memory(void*, unsigned32, unsigned32*);


  /* ----- Class Globals/Statics */

bool g_fProcessedFile;		// Set when we do something
FileWalk g_filewalk;		// Global file walking object

char* g_szPathTempHash;		// Pointer to temporary filename for hashing

int g_cFilesProcessed;		// Total files processed
int g_cFilesSkipped;		// Number of files skipped because irrelevent
int g_cFilesInaccessible;	// Number of files that could not be read
int g_cFilesGood;		// Number of files with good hash/signature
int g_cFilesBad;		// Number of files with bad hash/signature

void setup_signals (void);

void process_files (void);	// the execution routine

struct arguments g_arguments;

  /* ----- Methods */


/* cleanup_temp_hash

   function to remove the temporary used during verification.  Until
   we can verify without writing the data to disk, this is the best we
   can do.  This function is so declared because it is called from the
   signal handlers.

*/

void cleanup_temp_hash (bool fSignal)
{
  if (g_szPathTempHash) {
    int result = unlink (g_szPathTempHash); 
    //    if (result == 0 && fSignal)
    //      fprintf (stderr, "unlinked temp %s\n", g_szPathTempHash);
  }
  if (!fSignal) {
    char* sz = g_szPathTempHash;
    g_szPathTempHash = NULL;	// Prevent signal handler conflict
    delete sz;
  }
}


/* process_files

   performs the core processing.  It enumerates the files in the file
   processing list as given by the user's command line arguments.  For
   each, it determines if the file is one that is appropriate for
   processing.  If the file is to be hashed or signed and there is no
   signature section, it is first rewritten to create the signature
   section.  Then the hash is inserted and, if requested, the
   signature.  Checking the hash, verifying the signature, and
   removing signatures are all simple variations from there.

*/

void process_files ()
{
  bool fWrite
    = (g_arguments.mode == modeHash || g_arguments.mode == modeSign);

  while (const char* pch = g_filewalk.next ()) {

    int fh = -1;
    int fhNew = -1;
    char* pbMap = (char*) -1;
    long cb = 0;
    struct stat& stat = *g_filewalk.stat ();
    char rgb[size_elf_header ()];
    bool fElf = false;

    if (g_filewalk.stat () == NULL) {
      set_exitstatus (filenotfound, 
		      "unable to stat file '%s'", pch);
      goto do_filename_done;
    }

    set_exitstatus (noerror);
    g_fProcessedFile = true;
    ++g_cFilesProcessed;

    if (!S_ISREG (stat.st_mode)) {
      set_exitstatus (unsupportedfiletype, 
		      "unable to process non-regular file '%s'", pch);
      goto do_filename_done;
    }

    fh = open (pch, fWrite ? O_RDWR : O_RDONLY);

    //  printf ("do_filename: '%s'\n", pch);

    if (fh == -1) {
      switch (errno) {
      case EACCES:
	set_exitstatus (permissiondenied,
			"insufficient priviledge to open file '%s'", pch);
	++g_cFilesInaccessible;
	break;
      default:
	set_exitstatus (filenotfound, "file not found '%s' %d", pch, errno);
	break;
      }
      goto do_filename_done;
    }
   
    //  fprintf (stderr, "fh %d\n", fh);
    if ((   g_arguments.mode == modeSign
	 || g_arguments.mode == modeHash
	 || g_arguments.mode == modeRemove)
	&& (g_arguments.fNoSymLinks && S_ISLNK (stat.st_mode))) {
      set_exitstatus (unsupportedfiletype, 
		      "ignoring symbolic link '%s'", pch);
      ++g_cFilesSkipped;
      goto do_filename_done;
    }

    if (!(cb = stat.st_size)) {
      set_exitstatus (unsupportedfiletype, "empty file '%s'", pch);
      ++g_cFilesSkipped;
      goto do_filename_done;
    }
    
    if (cb >= size_elf_header ()) {
      fElf = is_elf_header (rgb, read (fh, rgb, size_elf_header ()));
      lseek (fh, SEEK_SET, 0);
    }			    

    // *** FIXME: at the moment, we only work with ELF format files.
    // This test is here in order to avoid mmap'ing the file
    // unnecessarily.

    if (!fElf) {
      set_exitstatus (unsupportedfiletype, "file '%s' is not ELF", pch);
      ++g_cFilesSkipped;
      goto do_filename_done;
    }

    	// --- Begin the real work.  We have a file of a supported type.

    if ((pbMap = (char*) mmap (NULL, cb, PROT_READ, MAP_PRIVATE, fh, 0))
	== (caddr_t) -1) {
      set_exitstatus (permissiondenied, "unable to mmap '%s'", pch);
      goto do_filename_done;
    }

	// --- Rewrite if we have no signature section
    SigInfo info;
    bzero (&info, sizeof (info));
    if ((g_arguments.mode == modeHash || g_arguments.mode == modeSign)
	&& !is_elf_rewritten (pbMap, cb, &info)) {
      
      		// Open temporary file
      char* szPath = path_of (g_arguments.szOutput
			      ? g_arguments.szOutput : pch);
//    printf ("path_of '%s'\n", szPath);
      char* szFileNew = new char [strlen (szPath) + 32];
      strcpy (szFileNew, szPath);
      strcat (szFileNew, "/bsign.XXXXXX");
      g_szPathTempHash = szFileNew; // Permit signal cleanup
      fhNew = mkstemp (szFileNew);
      delete szPath;
      //    printf ("fhNew %d\n", fhNew);
      if (fhNew == -1) {
	szPath = path_of (g_arguments.szOutput ? g_arguments.szOutput : pch);
	set_exitstatus (permissiondenied, 
			"unable to create file in '%s' for '%s'",
			szPath, pch);
	delete szPath;
	++g_cFilesInaccessible;
	goto do_filename_done;
      }

				// Perform rewrite
      //      if (g_arguments.fVerbose)
      //	fprintf (stdout, "%s\n", pch);
      if (g_exitStatus = rewrite_elf (pbMap, cb, fhNew, &info)) {
	set_exitstatus (g_exitStatus, "error %d while rewriting '%s'",
			g_exitStatus, pch);
	goto do_filename_done;
      }
      munmap (pbMap, cb);
      close (fh);
      fh = -1;
      close (fhNew);
      fhNew = -1;

				// Perform replacement
      if (!dup_status (g_szPathTempHash, pch)) {
	set_exitstatus (permissiondenied, 
			"insufficient priviledge to duplicate '%s'"
			" ownership", pch);
	++g_cFilesInaccessible;
	goto do_filename_done;
      }
      if (!replace_file (g_arguments.szOutput
			 ? g_arguments.szOutput
			 : pch, g_szPathTempHash)) {
	// I'd be suprised if I ever get here.  I think that by this
	// time, I have already written the file in the same directory
	// and performed chown to set permissions.
	set_exitstatus (permissiondenied, "unable to create output file '%s'", 
			g_arguments.szOutput ? g_arguments.szOutput : pch);
	++g_cFilesInaccessible;
	goto do_filename_done;
      }

				// Reopen and remap
      fh = open (g_arguments.szOutput
		 ? g_arguments.szOutput
		 : pch, fWrite ? O_RDWR : O_RDONLY);
      cb = lseek (fh, 0, SEEK_END);
      lseek (fh, 0, SEEK_SET);
      if ((pbMap = (char*) mmap (NULL, cb, PROT_READ, MAP_PRIVATE, fh, 0))
	  == (caddr_t) -1) {
	set_exitstatus (permissiondenied, "unable to re-mmap '%s'", pch);
	goto do_filename_done;
      }
    }

    // At this point, if we are going to write to the file then we
    // know that there is a signature section.  It may or may not be empty. 

    switch (g_arguments.mode) {
    case modeCheck:
    case modeVerify:
    case modeExtract:
    case modeInfo:
    case modeUnsigned:
      if (g_arguments.fVerbose)
	fprintf (stdout, "%s\n", pch);
      g_exitStatus
	= check_elf (pbMap, cb, g_arguments.fExpectSignature, &info);
      break;

    case modeHash:
    case modeSign:
      if (is_elf (pbMap, cb)) {

	if (info.version && !g_arguments.fForceResign) {
	  set_exitstatus (noerror, "Skipping already signed file");
	  ++g_cFilesSkipped;
	  goto do_filename_done;
	}

	if (g_arguments.fVerbose)
	  fprintf (stdout, "%s\n", pch);
	switch (g_exitStatus = hash_elf (pbMap, cb, fh, &info,
					 g_arguments.mode == modeSign)) {
	case noerror:
	  break;
	case badpassphrase:
	  set_exitstatus (g_exitStatus, "incorrect passphrase"
			  " or gpg not installed");
	  goto do_filename_done;
	default:
	  set_exitstatus (g_exitStatus, "error %d while hashing '%s'",
			  g_exitStatus, pch);
	  goto do_filename_done;
	}
      }      
      else {
	// I know this is redundant
	set_exitstatus (unsupportedfiletype, "file '%s' is not ELF", pch);
	++g_cFilesSkipped;
	goto do_filename_done;
      }
      goto do_filename_done;	// Skip check-oriented status report
    }

    switch (g_arguments.mode) {
    case modeExtract:
      if (g_arguments.mode == modeExtract && info.ibSignature) {
	for (size_t i = 0; i < info.cbSignature; ++i)
	  fputc (pbMap[info.ibSignature + i], stdout);
      }
      break;

    case modeInfo:
      if (info.version) {
	fprintf (stdout, "version: %d\n", info.version);
	char* pch = strchr (pbMap + info.ibSection, ';'); 
	if (pch) while (*++pch == ' ') ;
	char* pchEnd = ((size_t) (pch - (pbMap + info.ibSection))
			< info.cbSection ? strchr (pch, '\n') : NULL);
	if (pch && pchEnd
	    && pchEnd < pbMap + info.ibSection + info.cbSection)
	  fprintf (stdout, "id: %*.*s\n", 
		   pchEnd - pch, pchEnd - pch, pch);
      }
      if (info.ibHash) {
	fprintf (stdout, "hash: {SHA1}");
	for (size_t i = 0; i < info.cbHash; ++i)
	  fprintf (stdout, "%s%s%02x", 
		   (i % 2) == 0 ? " " : "",
		   (i % 10) == 0 && i > 0 ? " " : "",
		   ((unsigned char*) pbMap)[info.ibHash + i]);
	fprintf (stdout, "\n");
      }
      if (info.ibSignature) {
	fprintf (stdout, "signature_size: %d\n", info.cbSignature);
	fprintf (stdout, "signature:");
	for (size_t i = 0; i < info.cbSignature; ++i)
	  fprintf (stdout, "%s%s %02x", 
		   (i % 16) == 0 ? "\n " : "",
		   (i % 8) == 0 ? " " : "",
		   ((unsigned char*) pbMap)[info.ibSignature + i]
		   );
	fprintf (stdout, "\n");
	rfc2440 sig(pbMap + info.ibSignature, info.cbSignature);
	fprintf (stdout, "signer: %s\n", sig.describe_id ());
	fprintf (stdout, "timestamp: %s (%d)\n", 
		 sig.timestring (), sig.timestamp ());
      }
      break;
    }

    switch (g_exitStatus) {
    case noerror:
      set_exitstatus (g_exitStatus, "good %s found in '%s'.", 
		      g_arguments.fExpectSignature ? "signature" : "hash", 
		      pch);
      ++g_cFilesGood;
      break;
    case badhash:
      set_exitstatus (g_exitStatus, "invalid hash in '%s'.", pch);
      ++g_cFilesBad;
      break;
    case badsignature:
      set_exitstatus (g_exitStatus, 
		      "failed to verify signature in '%s'.", pch);
      ++g_cFilesBad;
      break;
    case nohash:
      set_exitstatus (g_exitStatus, "no hash found in '%s'.", pch);
      ++g_cFilesBad;
      break;
    case nosignature:
      set_exitstatus (g_exitStatus, "no signature found in '%s'.", pch);
      ++g_cFilesBad;
      break;
    case programnotfound:
      set_exitstatus (g_exitStatus, "unable to verify signature, "
		      "probably because gpg is not installed");
      break;
    case unusednotzero:
      set_exitstatus (g_exitStatus, "unused bytes in signature section "
		      "are not zero--may indicate tampering");
      ++g_cFilesBad;
      break;
    default:
      set_exitstatus (g_exitStatus, "error %d while verifying '%s'.", pch);
      ++g_cFilesBad;
      break;
    }


#if 0
    if (g_arguments.mode == modeSign || g_arguments.mode == modeHash) {
      if (is_elf (pbMap, cb)) {
	if (g_arguments.fVerbose)
	  fprintf (stdout, "%s\n", pch);
	switch (g_exitStatus = hash_elf (pbMap, cb, fhNew, 
					 g_arguments.mode == modeSign)) {
	case noerror:
	  break;
	case badpassphrase:
	  set_exitstatus (g_exitStatus, "incorrect passphrase"
			  " or gpg not installed");
	  goto do_filename_done;
	default:
	  set_exitstatus (g_exitStatus, "error %d while hashing '%s'",
			  g_exitStatus, pch);
	  goto do_filename_done;
	}
      }      
      else {
	set_exitstatus (unsupportedfiletype, "file '%s' is not ELF", pch);
	++g_cFilesSkipped;
	goto do_filename_done;
      }
    }
    else {			// check only
      if (!is_elf (pbMap, cb)) {
	set_exitstatus (unsupportedfiletype, "file '%s' is not ELF", pch);
	++g_cFilesSkipped;
	goto do_filename_done;
      }

      if (g_arguments.fVerbose)
	fprintf (stdout, "%s\n", pch);

      SigInfo info;
      bzero (&info, sizeof (info));
      g_exitStatus = check_elf (pbMap, cb, 
				g_arguments.fExpectSignature, &info);
      
      if (g_arguments.mode == modeInfo) {
	if (info.version) {
	  fprintf (stdout, "version: %d\n", info.version);
	  char* pch = strchr (pbMap + info.ibSection, ';'); 
	  if (pch) while (*++pch == ' ') ;
	  char* pchEnd = ((size_t) (pch - (pbMap + info.ibSection))
			  < info.cbSection ? strchr (pch, '\n') : NULL);
	  if (pch && pchEnd
	      && pchEnd < pbMap + info.ibSection + info.cbSection)
	    fprintf (stdout, "id: %*.*s\n", 
		     pchEnd - pch, pchEnd - pch, pch);
	}
	if (info.ibHash) {
	  fprintf (stdout, "hash: {SHA1}");
	  for (size_t i = 0; i < info.cbHash; ++i)
	    fprintf (stdout, "%s%s%02x", 
		     (i % 2) == 0 ? " " : "",
		     (i % 10) == 0 && i > 0 ? " " : "",
		     ((unsigned char*) pbMap)[info.ibHash + i]);
	  fprintf (stdout, "\n");
	}
	if (info.ibSignature) {
	  fprintf (stdout, "signature_size: %d\n", info.cbSignature);
	  fprintf (stdout, "signature:");
	  for (size_t i = 0; i < info.cbSignature; ++i)
	    fprintf (stdout, "%s%s %02x", 
		     (i % 16) == 0 ? "\n " : "",
		     (i % 8) == 0 ? " " : "",
		     ((unsigned char*) pbMap)[info.ibSignature + i]
		     );
	  fprintf (stdout, "\n");
	  rfc2440 sig(pbMap + info.ibSignature, info.cbSignature);
	  fprintf (stdout, "signer: %s\n", sig.describe_id ());
	  fprintf (stdout, "timestamp: %s (%d)\n", 
		   sig.timestring (), sig.timestamp ());
	}
      }

      if (g_arguments.mode == modeExtract && info.ibSignature) {
	  for (size_t i = 0; i < info.cbSignature; ++i)
	    fputc (pbMap[info.ibSignature + i], stdout);
      }

      switch (g_exitStatus) {
      case noerror:
	set_exitstatus (g_exitStatus, "good %s found in '%s'.", 
			g_arguments.fExpectSignature ? "signature" : "hash", 
			pch);
	++g_cFilesGood;
	break;
      case badhash:
	set_exitstatus (g_exitStatus, "invalid hash in '%s'.", pch);
	++g_cFilesBad;
	break;
      case badsignature:
	set_exitstatus (g_exitStatus, 
			"failed to verify signature in '%s'.", pch);
	++g_cFilesBad;
	break;
      case nohash:
	set_exitstatus (g_exitStatus, "no hash found in '%s'.", pch);
	++g_cFilesBad;
	break;
      case nosignature:
	set_exitstatus (g_exitStatus, "no signature found in '%s'.", pch);
	++g_cFilesBad;
	break;
      case programnotfound:
	set_exitstatus (g_exitStatus, "unable to verify signature, "
			"probably because gpg is not installed");
	break;
      case unusednotzero:
	set_exitstatus (g_exitStatus, "unused bytes in signature section "
			"are not zero--may indicate tampering");
	++g_cFilesBad;
	break;
      default:
	set_exitstatus (g_exitStatus, "error %d while verifying '%s'.", pch);
	++g_cFilesBad;
	break;
      }
    }
#endif
#if 0
    if (fhNew != -1) {
      close (fhNew);
      fhNew = -1;
      if (!dup_status (g_szPathTempHash, pch)) {
	set_exitstatus (permissiondenied, 
			"insufficient priviledge to duplicate '%s'"
			" ownership", pch);
	++g_cFilesInaccessible;
	goto do_filename_done;
      }
      if (!replace_file (g_arguments.szOutput
			 ? g_arguments.szOutput
			 : pch, g_szPathTempHash)) {
	// I'd be suprised if I ever get here.
	// I think that by this time, I have
	// already written the file in the
	// same directory and performed chown
	// to set the permissions.
	set_exitstatus (permissiondenied, "unable to create output file '%s'", 
			g_arguments.szOutput ? g_arguments.szOutput : pch);
	++g_cFilesInaccessible;
	goto do_filename_done;
      }
    }
#endif

  do_filename_done:
    if (pbMap != (caddr_t) -1)
      munmap (pbMap, cb);
    if (fhNew != -1)
      close (fhNew);
    if (fh != -1)
      close (fh);
    cleanup_temp_hash (false);

    if (!g_arguments.fQuiet) {
      if (g_arguments.fIgnore 
	  && (   g_exitStatus == isdirectory
	      || g_exitStatus == unsupportedfiletype))
	set_exitstatus_reported ();
      if (g_arguments.fHideGoodSigs && g_exitStatus == noerror)
	set_exitstatus_reported ();
      if (!is_exitstatus_reported ()) {
	fprintf (stdout, "%s: %s\n", 
		 g_arguments.szApplication, g_szExitStatus);
	set_exitstatus_reported ();
      }
    }
  }
}


/* main

   entry point.  Note that this main is simplified to an idiom because
   of the options code we use.  Files are processed when non-option
   command line g_arguments are found.  Refer to OPTION_F_NOPTION.

*/

int main (int argc, char** argv)
{
  setup_signals ();

  parse_arguments (g_arguments, argc, argv);

  if (!is_exitstatus ())
    process_files ();

  if (g_arguments.mode == modeVersion) {
    fprintf (stdout, "%s " SZ_VERSION "\n"
"\n" SZ_COPYRIGHT "\n"
"This is free sofyware; see the source for copying conditions.  There is NO\n"
"warranty; not even for MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.\n"
, g_arguments.szApplication);
    g_fProcessedFile = true;
    //    set_exitstatus_reported ();
  }

  //  printf ("done with args (%d)\n", result);

  if (!is_exitstatus () && !g_fProcessedFile)
    set_exitstatus (filenotfound, "No input files specified.");

     // Invert OK codes when asked to do so
  if (g_arguments.fInverseResult) {
    switch (g_exitStatus) {
    case noerror:
    case unsupportedfiletype:
    default:
      g_exitStatus = filenotfound; // Anything to say 'no need to sign it'
      break;
    case nosignature:
    case nohash:
    case badhash:
    case badsignature:
      g_exitStatus = noerror;
      break;
    }
  }

  if (g_arguments.fSummary)
    fprintf (stdout, 
	     "Summary processed:%d skipped:%d inaccessible:%d\n"
	     "Results succeeded:%d failed:%d\n",
	     g_cFilesProcessed, g_cFilesSkipped, g_cFilesInaccessible,
	     g_cFilesGood, g_cFilesBad);

  if (is_exitstatus () && !is_exitstatus_reported () && !g_arguments.fQuiet)
    fprintf (stdout, "%s: %s\n", g_arguments.szApplication, g_szExitStatus);

  return g_exitStatus;
}  /* main */
