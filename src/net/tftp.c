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

#define MS_TIMEOUT	(5*1000)

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
