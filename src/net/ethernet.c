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
#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <apex.h>	/* printf */
#include <driver.h>
#include <network.h>
#include <ethernet.h>

//#define TALK

#if TALK > 0
# define DBG(l,f...)		if (l >= TALK) printf (f);
#else
# define DBG(l,f...)		do {} while (0)
#endif

#define ARP_TABLE_LENGTH	8
#define FRAME_TABLE_LENGTH	8 

#define ARP_SECONDS_LIVE	30		

#define ETH_F(f)	((struct header_ethernet*) (f->rgb))

#define ARP_F(f)	((struct header_arp*)\
			 (f->rgb\
			  + sizeof (struct header_ethernet)))

#define IPV4_F(f)		((struct header_ipv4*)\
			 (f->rgb\
			  + sizeof (struct header_ethernet)))

#define ICMP_F(f)	((struct header_icmp*)\
			 (f->rgb\
			  + sizeof (struct header_ethernet)\
			  + sizeof (struct header_ipv4)))

#define ICMP_PING_F(f)	((struct message_icmp_ping*)\
			 (f->rgb\
			  + sizeof (struct header_ethernet)\
			  + sizeof (struct header_ipv4)\
			  + sizeof (struct header_icmp)))

struct arp_entry {
  u8  address[6];
  u8  ip[4];
  u32 seconds;		/* Number of seconds that this entry is valid */
};

char host_ip_address[4] = { 192, 168, 8, 203 };
char host_mac_address[6] = { 0x00, 0x08, 0xee, 0x00, 0x77, 0x9c };

enum {
  state_free = 0,
  state_allocated = 1,
  state_queued = 2,
};

struct arp_entry arp_table[ARP_TABLE_LENGTH];
struct ethernet_frame frame_table[FRAME_TABLE_LENGTH];

static u16 checksum (void* pv, int cb)
{
  u16* p = (u16*) pv;
  u32 sum = 0;
  for (; cb > 0; cb -= 2) {
    unsigned short s = *p++;
    sum += HTONS (s);
  }

  return ~ ((sum & 0xffff) + (sum >> 16));
}

void ethernet_init (void)
{
}

/* ethernet_frame_allocate

*/

struct ethernet_frame* ethernet_frame_allocate (void)
{
  int i;
  for (i = 0; i < FRAME_TABLE_LENGTH; ++i)
    if (frame_table[i].state == state_free) {
      frame_table[i].state = state_allocated;

      return &frame_table[i];
    }
  return NULL;
}


void ethernet_frame_release (struct ethernet_frame* frame)
{
  frame->state = state_free;
}


/* ethernet_frame_reply

   turns a frame around to reply to the sender's hardware address.

*/

void ethernet_frame_reply (struct ethernet_frame* f)
{
  memcpy (ETH_F (f)->destination_address, ETH_F (f)->source_address, 6);
  memcpy (ETH_F (f)->source_address,		     host_mac_address, 6);
}


void ipv4_frame_reply (struct ethernet_frame* f)
{
  memcpy (IPV4_F (f)->destination_ip, IPV4_F (f)->source_ip, 4);
  memcpy (IPV4_F (f)->source_ip, host_ip_address, 4);
  IPV4_F (f)->checksum = 0;
  IPV4_F (f)->checksum = htons (checksum ((void*) IPV4_F (f), 
					  sizeof (struct header_ipv4)));
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
    if (memcmp (arp_table[i].ip, protocol_address, 4) == 0) {
      memcpy (arp_table[i].ip, hardware_address, 6);
      arp_table[i].seconds = ARP_SECONDS_LIVE;
      break;
    }
}


/* arp_receive

   accepts all ARP packets.  This code handles automatic ARP_REPLY
   generation for requests for our IP address.

*/

void arp_receive (struct descriptor_d* d, struct ethernet_frame* frame)
{
  DBG (1,"%s\n", __FUNCTION__);

  DBG (2,"%s: checking length\n", __FUNCTION__);
  if (frame->cb < (sizeof (struct header_ethernet) + sizeof (struct header_arp)
	    + 6*2 + 4*2))
    return;			/* runt */

  DBG (2,"%s: checking protocol lengths %d %d\n", __FUNCTION__,
	  ARP_F (frame)->hardware_address_length,
	  ARP_F (frame)->protocol_address_length);
  if (   ARP_F (frame)->hardware_address_length != 6
      || ARP_F (frame)->protocol_address_length != 4)
    return;			/* unrecognized form */

  DBG (2,"%s: opcode %d \n", __FUNCTION__, HTONS (ARP_F (frame)->opcode));

  switch (ARP_F (frame)->opcode) {
  case HTONS (ARP_REQUEST):
    if (memcmp (host_ip_address, ARP_F (frame)->target_protocol_address, 4))
      return;			/* Not a match */

	/* Send reply to request for our address */
    ethernet_frame_reply (frame);
    ARP_F (frame)->opcode = HTONS (ARP_REPLY);

    memcpy (ARP_F (frame)->target_hardware_address,
	    ARP_F (frame)->sender_hardware_address,
	    10);		/* Move both the HW and protocol address */
    memcpy (ARP_F (frame)->sender_hardware_address,
	    host_mac_address,
	    6);
    memcpy (ARP_F (frame)->sender_protocol_address,
	    host_ip_address,
	    4);
    frame->cb = sizeof (struct header_ethernet) + sizeof (struct header_arp);
    d->driver->write (d, frame->rgb, frame->cb);
    break;

  case HTONS (ARP_REPLY):
    arp_receive_reply (ARP_F (frame)->sender_hardware_address,
		       ARP_F (frame)->sender_protocol_address);
    break;
  }
  
}

void icmp_receive (struct descriptor_d* d, struct ethernet_frame* frame)
{
  int l;

  DBG (1,"%s\n", __FUNCTION__);

  DBG (2,"%s: checking length\n", __FUNCTION__);
  if (frame->cb < (sizeof (struct header_ethernet)
		   + sizeof (struct header_ipv4)
		   + sizeof (struct header_icmp)))
    return;			/* runt */

  DBG (2,"%s: icmp %d received\n", __FUNCTION__, ICMP_F (frame)->type);

  l = htons (IPV4_F (frame)->length) - sizeof (struct header_ipv4);
  DBG (2,"%s: checksum %x  calc %x  over %d\n", __FUNCTION__, 
	  ICMP_F (frame)->checksum,
	  checksum (ICMP_F (frame), l), l);

  if (checksum (ICMP_F (frame), l) != 0) {
    DBG (1,"%s: icmp discarded, header checksum incorrect\n", __FUNCTION__);
    return;
  }
  
  switch (ICMP_F (frame)->type) {
  case ICMP_TYPE_ECHO:
    ethernet_frame_reply (frame);
    ipv4_frame_reply (frame);
    ICMP_F (frame)->type = ICMP_TYPE_ECHO_REPLY;
    ICMP_F (frame)->checksum = 0;
    ICMP_F (frame)->checksum = htons (checksum (ICMP_F (frame), l));
    d->driver->write (d, frame->rgb, frame->cb);
    break;
  }
}

/* ethernet_receive

   accepts packets into the network stack.

*/

void ethernet_receive (struct descriptor_d* d, struct ethernet_frame* frame)
{
  DBG (1,"%s\n", __FUNCTION__);

  if (frame->cb < sizeof (struct header_ethernet))
    return;			/* runt */

  switch (ETH_F (frame)->protocol) {
  case HTONS (ETH_PROTO_IP):
    /* *** FIXME: verify that the destination IP address is ours */
    switch (IPV4_F (frame)->protocol) {
    case IP_PROTO_ICMP:
      icmp_receive (d, frame);
      break;
    case IP_PROTO_UDP:
      break;
    }
    break;
  case HTONS (ETH_PROTO_ARP):
    arp_receive (d, frame);
    break;
  case HTONS (ETH_PROTO_RARP):
    break;
  }
}

