/* rarp.c
     $Id$

   written by Marc Singer
   7 Jul 2005

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

   Implementation of rarp autoconfiguration protocol.

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
#include <spinner.h>

#include <network.h>
#include <ethernet.h>

#define TALK 2

#if TALK > 0
# define DBG(l,f...)		if (l <= TALK) printf (f);
#else
# define DBG(l,f...)		do {} while (0)
#endif

#define TRIES_MAX	4
#define MS_TIMEOUT	(1*1000)

int cmd_rarp (int argc, const char** argv)
{
  struct descriptor_d d;
  int result;
  struct ethernet_frame* frame;
  unsigned long timeStart;
  int tries = 0;

  if (   (result = parse_descriptor (szNetDriver, &d))
      || (result = open_descriptor (&d))) 
    return result;

  DBG (2,"%s: open %s -> %d\n", __FUNCTION__, szNetDriver, result);

  frame = ethernet_frame_allocate ();

  DBG (2,"%s: setup ethernet header %p\n", __FUNCTION__, frame);

  memset (ETH_F (frame)->destination_address, 0xff, 6);
  memcpy (ETH_F (frame)->source_address, host_mac_address, 6);
  ETH_F (frame)->protocol = HTONS (ETH_PROTO_RARP);

  DBG (2,"%s: setup rarp header\n", __FUNCTION__);

  ARP_F (frame)->hardware_type = HTONS (ARP_HARDW_ETHERNET);
  ARP_F (frame)->protocol_type = HTONS (ARP_PROTO_IP);
  ARP_F (frame)->hardware_address_length = 6;
  ARP_F (frame)->protocol_address_length = 4;
  ARP_F (frame)->opcode = HTONS (ARP_REVERSEREQUEST);

  DBG (2,"%s: setup rarp\n", __FUNCTION__);

  memcpy (ARP_F (frame)->sender_hardware_address, host_mac_address, 6);
  memset (ARP_F (frame)->sender_protocol_address, 0, 4);
  memcpy (ARP_F (frame)->target_hardware_address, 
	  ARP_F (frame)->sender_hardware_address, 10);
  frame->cb = sizeof (struct header_ethernet) + sizeof (struct header_arp);
  dump (frame->rgb, frame->cb, 0);

  do {
    DBG (1,"%s: send frame\n", __FUNCTION__);
    d.driver->write (&d, frame->rgb, 
		     sizeof (struct header_ethernet)
		     + sizeof (struct header_arp));
    ++tries;
    timeStart = timer_read ();

    do {
      frame->cb = d.driver->read (&d, frame->rgb, FRAME_LENGTH_MAX);
      if (frame->cb > 0) {
	DBG (1,"%s: received frame\n", __FUNCTION__);
	ethernet_receive (&d, frame);
	frame->cb = 0;
      }
    } while (UNCONFIGURED_IP
	     && timer_delta (timeStart, timer_read ()) < MS_TIMEOUT);
  } while (UNCONFIGURED_IP && tries < TRIES_MAX);

  ethernet_frame_release (frame);

  close_descriptor (&d);  
  return 0;
}

static __command struct command_d c_rarp = {
  .command = "rarp",
  .description = "rarp autoconfiguration protocol",
  .func = cmd_rarp,
  COMMAND_HELP(
"rarp\n"
"  Configure the host IP address using RARP.\n"
  )
};
