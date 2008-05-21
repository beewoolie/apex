/* cmd-copy.c

   written by Marc Singer
   5 Nov 2004

   Copyright (C) 2004 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

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

static inline unsigned long swab32(unsigned long l)
{
  return (  ((l & 0x000000ffUL) << 24)
	  | ((l & 0x0000ff00UL) << 8)
	  | ((l & 0x00ff0000UL) >> 8)
	  | ((l & 0xff000000UL) >> 24));
}

#if !defined (CONFIG_SMALL)
//# define USE_COPY_VERIFY	/* Define to include verify feature */
#endif

int cmd_copy (int argc, const char** argv)
{
  struct descriptor_d din;
  struct descriptor_d dout;
  int swap = 0;
#if defined (USE_COPY_VERIFY)
  struct descriptor_d din_v;
  struct descriptor_d dout_v;
  int verify = 0;
#endif

  int result = 0;
  ssize_t cbCopy = 0;

  for (; argc > 1 && *argv[1] == '-'; --argc, ++argv) {
    switch (argv[1][1]) {
#if defined (USE_COPY_VERIFY)
    case 'v':
      verify = 1;
      break;
#endif
    case 's':
      swap = 1;
      break;
    default:
      return ERROR_PARAM;
    }
  }

  if (argc != 3)
    return ERROR_PARAM;

  if (   (result = parse_descriptor (argv[1], &din))
      || (result = open_descriptor (&din))) {
    printf ("Unable to open '%s'\n", argv[1]);
    goto fail_early;
  }

  if (   (result = parse_descriptor (argv[2], &dout))
      || (result = open_descriptor (&dout))) {
    printf ("Unable to open '%s'\n", argv[2]);
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
    char __aligned rgb[512];
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
	char __aligned rgbVerify[512];
	ssize_t cbVerify = din_v.driver->read (&din_v, rgbVerify,
					       sizeof (rgbVerify));
	if (cbVerify != cb) {
	  printf ("\rVerify failed: reread of input %d, expected %d, at"
		  " 0x%x+0x%x\n", cbVerify, cb, cbCopy, 512);
	  return ERROR_FAILURE;
	}
	if (memcmp (rgb, rgbVerify, cb)) {
	  printf ("\rVerify failed: reread input compare at 0x%x+0x%x\n",
		  cbCopy, 512);
	  return ERROR_FAILURE;
	}
      }
#endif

      if (swap) {
	int i;
	unsigned long* p = (unsigned long*) rgb;
	for (i = cb/4; i-- > 0; ++p)
	  *p = swab32 (*p);
      }

      SPINNER_STEP;
      cbWrote = dout.driver->write (&dout, rgb, cb);
      if (cbWrote != cb) {
	result = ERROR_RESULT (ERROR_FAILURE, "truncated write");
	goto fail;
      }

#if defined (USE_COPY_VERIFY)
      if (verify) {
	char rgbVerify[512];
	ssize_t cbVerify = dout_v.driver->read (&dout_v, rgbVerify,
						sizeof (rgbVerify));
	if (cbVerify != cb) {
	  printf ("\rVerify failed: reread of output %d, expected %d, at"
		  " 0x%x+0x%x\n", cbVerify, cb, cbCopy, 512);
	  return ERROR_FAILURE;
	}
	if (swap) {
	  int i;
	  unsigned long* p = (unsigned long*) rgbVerify;
	  for (i = cb/4; i-- > 0; ++p)
	    *p = swab32 (*p);
	}
	if (memcmp (rgb, rgbVerify, cb)) {
	  printf ("\rVerify failed: reread output compare at 0x%x+0x%x\n",
		  cbCopy, 512);
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
  .func = cmd_copy,
  COMMAND_DESCRIPTION ("copy data between devices")
  COMMAND_HELP(
"copy"
_USE_COPY_VERIFY(
" [-v]"
)
" [-s] SRC DST\n"
"  Copy data from SRC region to DST region.\n"
_USE_COPY_VERIFY(
"  Adding the -v performs redundant reads of the data to verify that\n"
"  what is read from SRC is correctly written to DST\n"
)
"  Adding the -s performs full word byte swap.  This is necessary when\n"
"  copying data stored in the opposite endian orientation from that which\n"
"  APEX is running.  This option requires that the length be an even\n"
"  multiple of words.\n"
"  The length of the DST region is ignored.\n\n"
"  e.g.  copy mem:0x20200000+0x4500 nor:0\n"
"        copy -s nor:0+1m 0x100000\n"
  )
};
