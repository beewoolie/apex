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

   o The ICMP echo reply is not properly turned around.  Though it
     works, it does so because we send the reply to the same MAC
     address as the sender.  Really, we should be finding our route to
     the destination with ARP.

*/

#include <config.h>
#include <linux/types.h>
#include <linux/string.h>
#include <linux/kernel.h>
#include <apex.h>	/* printf */
#include <driver.h>
#include <network.h>
#include <ethernet.h>
#include <alias.h>

//#define TALK

#if TALK > 0
# define DBG(l,f...)		if (l <= TALK) printf (f);
#else
# define DBG(l,f...)		do {} while (0)
#endif

#define ARP_TABLE_LENGTH	8
#define FRAME_TABLE_LENGTH	8 

#define ARP_SECONDS_LIVE	30		

struct arp_entry {
  u8  address[6];
  u8  ip[4];
  u32 seconds;		/* Number of seconds that this entry is valid */
};

char host_ip_address[4];
char gw_ip_address[4];
char host_mac_address[6] = { 0x00, 0x08, 0xee, 0x00, 0x77, 0x9c };
const char szNetDriver[] = "eth:";

enum {
  state_free      = 0,
  state_allocated = 1,
  state_queued    = 2,
};

struct arp_entry arp_table[ARP_TABLE_LENGTH];
struct ethernet_frame frame_table[FRAME_TABLE_LENGTH];

struct ethernet_receiver {
  int priority;
  pfn_ethernet_receiver pfn;
  void* context;
};
  
#define MAX_RECEIVERS	32
static struct ethernet_receiver receivers[MAX_RECEIVERS];
static int cReceivers;		/* Number of receivers */

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


/* arp_cache_update

   accepts the data from ARP_REPLY packets and updates the ARP cache
   accordingly.  Note that we don't snoop ARP entries.  Instead, we
   only maintain ARP translations for hosts we're interested in.

*/

void arp_cache_update (const char* hardware_address,
		       const char* protocol_address,
		       int force)
{
  int i;
  int iEmpty = -1;
  for (i = 0; i < ARP_TABLE_LENGTH; ++i) {
    if (iEmpty == -1 && memcmp (arp_table[i].address, "\0\0\0\0\0", 6) == 0)
      iEmpty = i;
    if (memcmp (arp_table[i].ip, protocol_address, 4) == 0) {
      memcpy (arp_table[i].address, hardware_address, 6);
      arp_table[i].seconds = ARP_SECONDS_LIVE;
      return;
    }
  }
  if (force && iEmpty != -1) {
    memcpy (arp_table[iEmpty].address, hardware_address, 6);
    memcpy (arp_table[iEmpty].ip,      protocol_address, 4);
  }
}


/* arp_cache_lookup

   returns a pointer to the MAC address for the given IP address or
   NULL if there is none.

*/

const char* arp_cache_lookup (const char* protocol_address)
{
  int i;
  for (i = 0; i < ARP_TABLE_LENGTH; ++i)
    if (memcmp (arp_table[i].ip, protocol_address, 4) == 0)
      return arp_table[i].address;

  return NULL;
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
    arp_cache_update (ARP_F (frame)->sender_hardware_address,
		      ARP_F (frame)->sender_protocol_address, 
		      0);
    break;

  case HTONS (ARP_REVERSEREPLY):
    if (memcmp (ARP_F (frame)->target_hardware_address,
		host_mac_address, 6))
      break;
    memcpy (host_ip_address, ARP_F (frame)->target_protocol_address, 4);
		/* Add ARP entry for the server */
    arp_cache_update (ARP_F (frame)->sender_hardware_address,
		      ARP_F (frame)->sender_protocol_address, 
		      1);
    {
      char sz[80];
      unsigned char* p = ARP_F (frame)->sender_protocol_address;
      sprintf (sz, "%d.%d.%d.%d", 
	       host_ip_address[0], host_ip_address[1], 
	       host_ip_address[2], host_ip_address[3]);
      alias_set ("hostip", sz);
      sprintf (sz, "%d.%d.%d.%d", p[0], p[1], p[2], p[3]);
      alias_set ("serverip", sz);
    }
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
    ethernet_frame_reply (frame); /* This isn't really valid, is it? */
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
  case HTONS (ETH_PROTO_RARP):
    arp_receive (d, frame);
    break;
  }
}


void udp_setup (struct ethernet_frame* frame, 
		const char* destination_ip, u16 destination_port,
		u16 source_port, size_t cb)
{
  const char* addr = arp_cache_lookup (destination_ip);
  if (!addr)
    addr = arp_cache_lookup (gw_ip_address);

  memset (IPV4_F (frame), 0, 
	  sizeof (struct header_ipv4) + sizeof (struct header_udp));

  /* Ethernet frame header */
  if (addr)
    memcpy (ETH_F (frame)->destination_address, addr, 6);
  else
    memset (ETH_F (frame)->destination_address, 0xff, 6);
  memcpy (ETH_F (frame)->source_address, host_mac_address, 6);
  ETH_F (frame)->protocol = HTONS (ETH_PROTO_IP);

  /* IPv4 header */
  IPV4_F (frame)->version_ihl = 4<<4 | 5;
  IPV4_F (frame)->length
    = htons (sizeof (struct header_ipv4) + sizeof (struct header_udp) + cb);
  IPV4_F (frame)->ttl = 64;
  IPV4_F (frame)->protocol = IP_PROTO_UDP;
  memcpy (IPV4_F (frame)->source_ip, host_ip_address, 4);
  memcpy (IPV4_F (frame)->destination_ip, destination_ip, 4);

  /* UDP header */
  UDP_F (frame)->source_port = HTONS (source_port);
  UDP_F (frame)->destination_port = HTONS (destination_port);
  UDP_F (frame)->length = sizeof (struct header_udp) + cb;

  /* Checksums, UDP and then TCP */
  UDP_F (frame)->checksum
    = checksum (UDP_F (frame), sizeof (struct header_udp) + cb);
  IPV4_F (frame)->checksum
    = checksum (IPV4_F (frame), sizeof (struct header_ipv4));
}
		

/* ethernet_service

   is the template function for processing network traffic.  It reads
   packets on the ethernet device given and receives them until the
   termination function returns a non-zero result.  The context value
   passed to this function is passed along to the termination
   function.  The return value is the non-zero result from the
   termination function.

*/

int ethernet_service (struct descriptor_d* d, 
		      int (*terminate) (void*), void* context) 
{
  struct ethernet_frame* frame = ethernet_frame_allocate ();
  int result;

  do {
    frame->cb = d->driver->read (d, frame->rgb, FRAME_LENGTH_MAX);
    if (frame->cb > 0) {
      ethernet_receive (d, frame);
      frame->cb = 0;
    }
    result = terminate (context);
  } while (result == 0);

  return result;
}


/* ethernet_timeout

   is a termination function for ethernet_service that terminates
   after a timeout is exceeded.  The context pointer is a
   ethernet_timeout_context structure.  It may zeroed and just the
   timeout set, or the time_start may also be set.

*/

int ethernet_timeout (void* pv)
{
  struct ethernet_timeout_context* context
    = (struct ethernet_timeout_context*) pv;

  if (!context->time_start)
    context->time_start = timer_read ();

  return timer_delta (context->time_start, timer_read ()) < context->ms_timeout
    ? 0 : -1;
}


/* register_ethernet_receive

   adds a frame receiving function to the list of active receivers.
   The function pointer will be called with the received frame as well
   as the context pointer when a frame is received.  The priority
   value is used to sort the receivers.  The higher the priority, the
   earlier the receiver will appear in the list.

   It returns zero on success, non-zero if the receiver cannot be
   added to the active receiver list.
   
*/

int register_ethernet_receiver (int priority, pfn_ethernet_receiver pfn, 
				void* context)
{
  if (cReceivers >= MAX_RECEIVERS)
    return -1;

  receivers[cReceivers].priority = priority;
  receivers[cReceivers].pfn	 = pfn;
  receivers[cReceivers].context	 = context;
  ++cReceivers;

  /* *** FIXME: we should perform a sort at this point. */

  return 0;
}


/* unregister_ethernet_receive

   removes a frame receiver from the list of active receivers.  Note
   that it doesn't use the original priority when removing a receiver.

   It returns zero on success, non-zero if the requested receiver
   isn't found.
   
*/

int unregister_ethernet_receiver (pfn_ethernet_receiver pfn, void* context)
{
  int i;

  for (i = 0; i < cReceivers; ++i) {
    if (receivers[i].pfn == pfn && receivers[i].context == context) {
      memset (&receivers[i], 0, sizeof (receivers[i]));
      break;
    }
  }

  if (i < cReceivers) {
    /* *** FIXME: we should perform a sort at this point. */
    /* Note that we don't decrement the count unless we can resort
       the list.  */
    return 0;
  }

  return -1;
}
