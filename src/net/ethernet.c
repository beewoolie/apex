/* ethernet.c
     $Id$

   written by Marc Singer
   5 May 2005

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

   NOTES
   -----

   o We can assume that if the packet is received, we have a matching
     MAC address or a broadcast address.  Later, we can get more
     choosy.  Or, perhaps this should be an option since some drivers
     will guarantee correctness.
 
   o We only save ARP mappings for addresses we've requested.  In
     other words, we need to request an address before the received
     ARP_REPLY packet will update the table.  This prevents the ARP
     cache from filling with the addresses of hosts we don't care
     about.

   o The multitude of byte-count compares and copies suggests that we
     be clever in how these are coded for optimal density.

*/

#include <config.h>
#include <network.h>

#define ARP_TABLE_LENGTH	8
#define FRAME_TABLE_LENGTH	8 
#define FRAME_LENGTH_MAX	1536

#define ARP_SECONDS_LIVE	30		

#define ETH_F(f) \
	((struct ethernet_header*) (f->rgb))
#define ARP_F(f) \
	((struct arp_header*) (f->rgb + sizeof (struct ethernet_header)))

struct arp_entry {
  u8  address[6];
  u32 ip;
  u32 seconds;		/* Number of seconds that this entry is valid */
};

char host_ip_address[4];
char host_mac_address[6];

enum {
  state_free = 0,
  state_allocated = 1,
  state_queued = 2,
};

struct ethernet_frame {
  size_t cb;
  int state;
  char rgb[FRAME_LENGTH_MAX];
};

struct arp_entry arp_table[ARP_TABLE_LENGTH];
struct ethernet_frame frame_table[FRAME_TABLE_LENGTH];

void ethernet_init (void)
{
}

/* ethernet_allocate_frame

*/

struct ethernet_frame* ethernet_allocate_frame (void)
{
  int i;
  for (i = 0; i < FRAME_TABLE_LENGTH; ++i)
    if (frame_table[i].state == state_free) {
      frame_table[i].state = state_allocated;

      return &frame_table[i];
    }
  return NULL;
}

/* ethernet_frame_reply

   turns a frame around to reply to the sender's hardware address.

*/

void ethernet_frame_reply (struct frame* f)
{
  memcpy (ETH_F (f)->destination_address, ETH_F (f)->source_address, 6);
  memcpy (ETH_F (f)->source_address,		     host_mac_address, 6);
}


/* arp_receive_reply

   accepts the data from ARP_REPLY packets and updates the ARP cache
   accordingly.  Note that we don't snoop ARP entries.  Instead, we
   only maintain ARP translations for hosts we're interested in.

*/

void arp_receive_reply (const char* hardware_address,
			const char* protocol_address)
{
  int i;
  for (i = 0; i < ARP_TABLE_LENGTH; ++i)
    if (memcmp (arp_table[i].ip, ip_address, 4) == 0) {
      memcpy (arp_table[i].ip, hardware_address, 6);
      arp_table[i].seconds = ARP_SECONDS_LIVE;
      break;
    }
}

/* arp_receive

   accepts all ARP packets.  This code handles automatic ARP_REPLY
   generation for requests for our IP address.

*/

void arp_receive (struct ethernet_frame* frame)
{
  if (frame->cb < (sizeof (struct header_ethernet) + sizeof (struct header_arp)
	    + 6*2 + 4*2))
    return;			/* runt */

  if (   ARP_F (frame)->hardware_address_length != 6
      || ARP_F (frame)->protocol_address_length != 4)
    return;			/* unrecognized form */

  switch (p->opcode) {
  case htons (ARP_REQUEST):
    if (memcmp (host_ip_address, p->target_protocol_address, 4))
      return;			/* Not a match */

	/* Send reply to request for our address */
    ethernet_frame_reply (frame);
    ARP_F (frame)->opcode = htons (ARP_REPLY);

    memcpy (ARP_F (frame)->target_hardware_address,
	    ARP_F (frame)->sender_hardware_address,
	    10);		/* Move both the HW and protocol address */
    memcpy (ARP_F (frame)->sender_hardware_address,
	    host_mac_address,
	    6);
    memcpy (arp_f (frame)->sender_protocol_address,
	    host_ip_address,
	    4);
    ethernet_send (frame);

    break;

  case htons (ARP_REPLY):
    arp_receive_reply (ARP_F (frame)->sender_hardware_address,
		       ARP_F (frame)->sender_protocol_address);
    break;
  }
  
}

/* ethernet_receive

   accepts packets into the network stack.

*/

void ethernet_receive (struct ethernet_frame* frame)
{
  struct header_ethernet* p;

  if (frame->cb < sizeof (struct header_ethernet))
    return;			/* runt */

  p = (struct header_ethernet*) frame->rgb;

  switch (p->protocol) {
  case htons (ETH_PROTO_IP):
    break;
  case htons (ETH_PROTO_ARP):
    arp_receive (frame);
    break;
  case htons (ETH_PROTO_RARP):
    break;
  }
}

void ethernet_send (struct ethernet_frame* frame)
{
  frame->state = state_queued;
}
