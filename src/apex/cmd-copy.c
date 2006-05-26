/* cmd-copy.c
     $Id$

   written by Marc Singer
   5 Nov 2004

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

#include <config.h>
#include <linux/string.h>
#include <linux/types.h>
#include <linux/ctype.h>
#include <apex.h>
#include <command.h>
#include <driver.h>
#include <error.h>
#include <spinner.h>


#if !defined (CONFIG_SMALL)
//# define USE_COPY_VERIFY	/* Define to include verify feature */
#endif

int cmd_copy (int argc, const char** argv)
{
  struct descriptor_d din;
  struct descriptor_d dout;
#if defined (USE_COPY_VERIFY)
  struct descriptor_d din_v;
  struct descriptor_d dout_v;
  int verify = 0;
#endif

  int result = 0;
  ssize_t cbCopy = 0;

#if defined (USE_COPY_VERIFY)
  if (argc > 1 && strcmp (argv[1], "-v") == 0) {
    verify = 1;
    --argc;
    ++argv;
  }
#endif

  if (argc != 3)
    return ERROR_PARAM;

  if (   (result = parse_descriptor (argv[1], &din))
      || (result = open_descriptor (&din))) {
    printf ("Unable to open target %s\n", argv[1]);
    goto fail_early;
  }

  if (   (result = parse_descriptor (argv[2], &dout))
      || (result = open_descriptor (&dout))) {
    printf ("Unable to open target %s\n", argv[2]);
    goto fail;
  }

  if (!din.driver->read || !dout.driver->write) {
    result = ERROR_UNSUPPORTED;
    goto fail;
  }

  if (!dout.length)
    dout.length = DRIVER_LENGTH_MAX;

#if defined (USE_COPY_VERIFY)
  /* Create descriptors for rereading and verification */
  /* *** FIXME: we ought to perform a dup () */
  memcpy (&din_v, &din, sizeof (din));
  memcpy (&dout_v, &dout, sizeof (dout));
  dout_v.length = din_v.length;
#endif

  {
    char rgb[1024];
    ssize_t cb;
    int report_last = -1;
    int step = DRIVER_PROGRESS (&din, &dout);
    if (step)
      step += 10;

    for (; (cb = din.driver->read (&din, rgb, sizeof (rgb))) > 0;
	 cbCopy += cb) {
      int report;
      size_t cbWrote;
      if (cb == 0) {
	result = ERROR_RESULT (ERROR_FAILURE, "premature end of input");
	goto fail;
      }

#if defined (USE_COPY_VERIFY)
      if (verify) {
	char rgbVerify[1024];
	ssize_t cbVerify = din_v.driver->read (&din_v, rgbVerify,
					       sizeof (rgbVerify));
	if (cbVerify != cb) {
	  printf ("\rVerify failed: reread of input %d, expected %d, at"
		  " 0x%x+0x%x\n", cbVerify, cb, cbCopy, 1024);
	  return ERROR_FAILURE;
	}
	if (memcmp (rgb, rgbVerify, cb)) {
	  printf ("\rVerify failed: reread input compare at 0x%x+0x%x\n",
		  cbCopy, 1024);
	  return ERROR_FAILURE;
	}
      }
#endif

      SPINNER_STEP;
      cbWrote = dout.driver->write (&dout, rgb, cb);
      if (cbWrote != cb) {
	result = ERROR_RESULT (ERROR_FAILURE, "truncated write");
	goto fail;
      }

#if defined (USE_COPY_VERIFY)
      if (verify) {
	char rgbVerify[1024];
	ssize_t cbVerify = dout_v.driver->read (&dout_v, rgbVerify,
						sizeof (rgbVerify));
	if (cbVerify != cb) {
	  printf ("\rVerify failed: reread of output %d, expected %d, at"
		  " 0x%x+0x%x\n", cbVerify, cb, cbCopy, 1024);
	  return ERROR_FAILURE;
	}
	if (memcmp (rgb, rgbVerify, cb)) {
	  printf ("\rVerify failed: reread output compare at 0x%x+0x%x\n",
		  cbCopy, 1024);
	  return ERROR_FAILURE;
	}
      }
#endif

      report = cbCopy>>step;
      if (step && report != report_last) {
	printf ("\r   %d KiB\r", cbCopy/1024);
	report_last = report;
      }
    }

    if (cb < 0) {
      printf ("\rcopy error\n");
      result = cb;
    }
  }

  if (result == 0)
    printf ("\r%d bytes transferred\n", cbCopy);

 fail:
  close_descriptor (&din);
  close_descriptor (&dout);
 fail_early:

  return result;
}

	/* Work-around for gcc-2.95 */
#if defined (USE_DIAG)
# define _USE_COPY_VERIFY(s) s
#else
# define _USE_COPY_VERIFY(s)
#endif

static __command struct command_d c_copy = {
  .command = "copy",
  .description = "copy data between devices",
  .func = cmd_copy,
  COMMAND_HELP(
"copy"
_USE_COPY_VERIFY(
" [-v]"
)
" SRC DST\n"
"  Copy data from SRC region to DST region.\n"
_USE_COPY_VERIFY(
"  Adding the -v performs redundant reads of the data to verify that\n"
"  what is read from SRC is correctly written to DST\n"
)
"  The length of the DST region is ignored.\n\n"
"  e.g.  copy mem:0x20200000+0x4500 nor:0\n"
  )
};
