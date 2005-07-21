/* tftp.c
     $Id$

   written by Marc Singer
   8 Jul 2005

   Copyright (C) 2005 Marc Singer

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

   The tftp descriptor looks like this:

     tftp://host/path
   or
     tftp:/path using a default host of $server, perhaps.
   

  NOTES
  -----

   o The meshing o the tftp protocol and data handling within APEX is
     a bit more complex that would be idea.  tftp only considers
     streams of bytes to be accessible linearly from the start to the
     end.  There is no seeking.  There is a timeout on each block, so
     APEX cannot hold off indefinitely with continuing the stream
     transfer.  The only flow control that APEX can assert when
     reading files is to throttle the acknowlegements.  So, what it
     shall do is request the first block.  Let the next layer read
     data from the buffered data and wait for the whole block to be
     consumed before acknowledging that the block was received.
   o Performance may be improved by caching one or more data blocks
     and allowing there to be a sliding window of available data.
     This could, potentially, allow for a reasonable overlap of
     requests and processing.  Especially if APEX is performing some
     sort of transformation on the data, e.g. checksum or writing to
     flash.  For transfers to memory, there is probably little to be
     gained with a sliding window.  However, this driver doesn't know
     how the data will be used. 
   o We need to select a random port number.  We need to see if there
     is some place where we can sample semi-random data.  At least so
     that we have a chance to detect out-dated connections.

*/

#include <config.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <linux/types.h>
//#include <linux/ctype.h>
#include <apex.h>
#include <command.h>
#include <driver.h>
#include <error.h>
#include <alias.h>
#include <spinner.h>

#include <network.h>
#include <ethernet.h>

#define TALK 2

#if TALK > 0
# define DBG(l,f...)		if (l <= TALK) printf (f);
#else
# define DBG(l,f...)		do {} while (0)
#endif

#define DRIVER_NAME	"tftp"

#define MS_TIMEOUT	(5*1000)

#define BLOCK_LENGTH	(512)
#define BLOCKS_CACHED	(4)	/* Number of blocks in the cache */

enum {
  stateIdle = 0,
  stateReading,
};

struct tftp_info {
  int state;
  int source_port;
  int destination_port;
  int mode;			/* reading or writing, uses an opcode */
  int block;			/* block number of last received block */

  unsigned char rgb[BLOCK_LENGTH*BLOCKS_CACHED];
  struct ethernet_frame* frame;
};

struct tftp_info tftp;

int cmd_tftp (int argc, const char** argv)
{
  struct descriptor_d d;
  int result;
  struct ethernet_frame* frame;
  unsigned long timeStart;
  char server_ip[] = { 192, 168, 8, 1 };
  u16 port = 1169;
  int cb;

  if (argc != 4)
    return ERROR_PARAM;

  if (   (result = parse_descriptor (szNetDriver, &d))
      || (result = open_descriptor (&d))) 
    return result;

  DBG (2,"%s: open %s -> %d\n", __FUNCTION__, szNetDriver, result);

  frame = ethernet_frame_allocate ();

  DBG (2,"%s: setup ethernet header %p\n", __FUNCTION__, frame);

  TFTP_F (frame)->opcode = TFTP_RRQ;
  cb = sizeof (struct message_tftp) 
    + sprintf (TFTP_F (frame)->data, "%s%coctet", argv[3], 0);
  frame->cb = sizeof (struct header_ethernet)
    + sizeof (struct header_ipv4)
    + sizeof (struct header_udp)
    + cb;

  udp_setup (frame, server_ip, PORT_TFTP, port, cb);
  
  d.driver->write (&d, frame->rgb, frame->cb);
  timeStart = timer_read ();
  printf ("sending\n");
  dump (frame->rgb, frame->cb, 0);

  do {
    SPINNER_STEP;

    frame->cb = d.driver->read (&d, frame->rgb, FRAME_LENGTH_MAX);
    if (frame->cb > 0) {
      DBG (1,"%s: received frame\n", __FUNCTION__);
      dump (frame->rgb, frame->cb, 0);
      ethernet_receive (&d, frame);
      frame->cb = 0;
    }
  } while (timer_delta (timeStart, timer_read ()) < MS_TIMEOUT);





  ethernet_frame_release (frame);

  close_descriptor (&d);  
  return 0;
}

static __command struct command_d c_tftp = {
  .command = "tftp",
  .description = "tftp transfer",
  .func = cmd_tftp,
  COMMAND_HELP(
"tftp SERVER PATH REGION\n"
"  Transfers the file, PATH, from the tftp SERVER to REGION via tftp.\n"
  )
};

static ssize_t tftp_read (struct descriptor_d* d, void* pv, size_t cb)
{
  
  switch (tftp.state) {
  case stateIdle:		/* Need to open the connection */
    
    if (!tftp.source_port)
      /* *** need a function to allocate port numbers */
      tftp.source_port = 23000;
    else
      ++tftp.source_port;
    tftp.destination_port = 69;
    tftp.mode = TFTP_RRQ;
    tftp.block = 0;
    tftp.frame = ethernet_frame_allocate ();

    TFTP_F(frame)->opcode = tftp.mode;
    {
      char* pch = TFTP_F(frame)->data;
      size_t cb = strlcpy (pch, d->pb[iRoot], 400);
      strcpy (pch + cb, "octet");
    }      
    /* Finish headers */
    /* Register port listener */
    /* Transmit packet */
    /* Handle state machine? */
    break;

  return 0;


}


/* tftp_open

   the only thing this can do is verify whether or not it is
   reasonable to open the file.  We could do some vetting of the
   addresses, perhaps check the host IP address.  Maybe we could ARP
   for the server to make sure it is accessible.

*/

static int tftp_open (struct descriptor_d* d)
{
  return 0;
}


  
#if 0
static ssize_t tftp_write (struct descriptor_d* d, void*, size_t cb)
{
  return 0;
}
#endif

static __driver_6 struct driver_d tftp_driver = {
  .name = DRIVER_NAME,
  .description = "trivial FTP driver",
  .flags = DRIVER_DESCRIP_FS,
  .open = tftp_open,
  .close = tftp_close,
  .read = tftp_read,
//  .write = tftp_write,
//  .erase = cf_erase,
  .seek = tftp_seek,
#if defined CONFIG_CMD_INFO
//  .info = tftp_info,
#endif
};

