/*-------------------------------------------------------------------------
 * Filename:      xmodem.c
 * Version:       $Id: xmodem.c,v 1.2 2002/01/06 19:02:29 erikm Exp $
 * Copyright:     Copyright (C) 2001, Hewlett-Packard Company
 * Author:        Christopher Hoover <ch@hpl.hp.com>
 * Description:   xmodem functionality for uploading of kernels 
 *                and the like
 * Created at:    Thu Dec 20 01:58:08 PST 2001
 *-----------------------------------------------------------------------*/
/*
 * xmodem.c: xmodem functionality for uploading of kernels and 
 *            the like
 *
 * Copyright (C) 2001 Hewlett-Packard Laboratories
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */

#include <apex.h>
#include <driver.h>

#define SOH (0x01)
#define STX (0x02)
#define EOT (0x04)
#define ACK (0x06)
#define NAK (0x15)
#define CAN (0x18)
#define BS  (0x08)

/*

Cf:

  http://www.textfiles.com/apple/xmodem
  http://www.phys.washington.edu/~belonis/xmodem/docxmodem.txt
  http://www.phys.washington.edu/~belonis/xmodem/docymodem.txt
  http://www.phys.washington.edu/~belonis/xmodem/modmprot.col

*/

#define MS_TIMEOUT 10*1000
#define MAXERRORS 5

#define ERROR(...) do { } while (0)


extern struct driver_d* console_driver;

static char rgbXmodem[1024];

static inline void xmodem_send (char ch) 
{ 
  console_driver->write (0, &ch, 1);
}

static inline void xmodem_read_flush (void) {
//	serial_flush_input();
}
  
int xmodem_receive (unsigned int timeout) {
  unsigned long start = timer_read ();

  while (start + timeout < timer_read ())
    if (console_driver->poll (0, 1)) {
      unsigned char ch;
      console_driver->read (0, &ch, 1);
      return ch;
    }

  return -1;
}

int XModemReceive (char *bufBase, int bufLen)
{
  unsigned int errors = 0;
  unsigned int wantBlockNo = 1;
  unsigned int length = 0;
  int crc = 1;
  char nak = 'C';

  xmodem_read_flush();

	/* Ask for CRC; if we get errors, we will go with checksum */
  xmodem_send(nak);

  for (;;) {
    int blockBegin;
    int blockNo, blockNoOnesCompl;
    int blockLength;
    int cksum = 0;
    int crcHi = 0;
    int crcLo = 0;

    blockBegin = xmodem_receive (MS_TIMEOUT);
    if (blockBegin < 0)
      goto timeout;
    
    nak = NAK;

    switch (blockBegin) {
    case SOH:
    case STX:
      break;

    case EOT:
      xmodem_send(ACK);
      goto done;

    default:
      goto error;
    }

		/* block no */
    blockNo = xmodem_receive (MS_TIMEOUT);
    if (blockNo < 0)
      goto timeout;

		/* block no one's compliment */
    blockNoOnesCompl = xmodem_receive (MS_TIMEOUT);
    if (blockNoOnesCompl < 0)
      goto timeout;

    if (blockNo != (255 - blockNoOnesCompl)) {
      ERROR("bad block ones compl\n");
      goto error;
    }

    blockLength = (blockBegin == SOH) ? 128 : 1024;

    {
      int i;
      
      for (i = 0; i < blockLength; i++) {
	int cc = xmodem_receive (MS_TIMEOUT);
	if (cc < 0)
	  goto timeout;
	rgbXmodem[i] = cc;
      }
    }

    if (crc) {
      crcHi = xmodem_receive (MS_TIMEOUT);
      if (crcHi < 0)
	goto timeout;
      
      crcLo = xmodem_receive (MS_TIMEOUT);
      if (crcLo < 0)
	goto timeout;
    } else {
      cksum = xmodem_receive (MS_TIMEOUT);
      if (cksum < 0)
	goto timeout;
    }

    if (blockNo == ((wantBlockNo - 1) & 0xff)) {
			/* a repeat of the last block is ok, just ignore it. */
			/* this also ignores the initial block 0 which is */
			/* meta data. */
      goto next;
    } else if (blockNo != (wantBlockNo & 0xff)) {
      ERROR("unexpected block no, 0x%08x, expecting 0x%08x\n", 
	    blockNo, wantBlockNo);
      goto error;
    }

    if (crc) {
      int crc = 0;
      int i, j;
      int expectedCrcHi;
      int expectedCrcLo;

      for (i = 0; i < blockLength; i++) {
	crc = crc ^ (int) rgbXmodem[i] << 8;
	for (j = 0; j < 8; j++)
	  if (crc & 0x8000)
	    crc = crc << 1 ^ 0x1021;
	  else
	    crc = crc << 1;
      }

      expectedCrcHi = (crc >> 8) & 0xff;
      expectedCrcLo = crc & 0xff;

      if ((crcHi != expectedCrcHi) ||
	  (crcLo != expectedCrcLo)) {
	ERROR("crc error, expected 0x%02x 0x%02x, got 0x%02x 0x%02x\n", 
	      expectedCrcHi, expectedCrcLo, crcHi, crcLo);
	goto error;
      }
    } else {
      unsigned char expectedCksum = 0;
      int i;
      
      for (i = 0; i < blockLength; i++)
	expectedCksum += rgbXmodem[i];

      if (cksum != expectedCksum) {
	ERROR("checksum error, expected 0x%02x, got 0x%02x\n", 
	      expectedCksum, cksum);
	goto error;
      }
    }

    wantBlockNo++;
    length += blockLength;

    if (length > bufLen) {
      ERROR("out of space\n");
      goto error;
    }

    {
      int i;
      for (i = 0; i < blockLength; i++)
	*bufBase++ = rgbXmodem[i];
    }

  next:
    errors = 0;
    xmodem_send(ACK);
    continue;

  error:
  timeout:
    errors++;
    if (errors == MAXERRORS) {
      /* Abort */
      int i;
      
			// if using crc, try again w/o crc
      if (nak == 'C') {
	nak = NAK;
	errors = 0;
	crc = 0;
	goto timeout;
      }

      ERROR("too many errors; giving up\n");
      for (i = 0; i < 5; i ++)
	xmodem_send(CAN);
      for (i = 0; i < 5; i ++)
	xmodem_send(BS);
      return -1;
    }

    xmodem_read_flush();
    xmodem_send(nak);
  }
  
 done:
  return length;
}
