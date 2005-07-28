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
#include <alias.h>
#include <spinner.h>

#include <network.h>
#include <ethernet.h>

//#define TALK 2

#if TALK > 0
# define DBG(l,f...)		if (l <= TALK) printf (f);
#else
# define DBG(l,f...)		do {} while (0)
#endif

#define TRIES_MAX	4
#define MS_TIMEOUT	(1*1000)


/* autoconf_terminate

   is the function used by ethernet_service() to deterine when to
   terminate the loop.  It return zero when the loop can continue, -1
   on timeout, and 1 when the configuration is complete.

*/

static int autoconf_terminate (void* pv)
{
  struct ethernet_timeout_context* context
    = (struct ethernet_timeout_context*) pv;

  if (!context->time_start)
    context->time_start = timer_read ();

  SPINNER_STEP;

  if (!UNCONFIGURED_IP)
    return 1;

  return timer_delta (context->time_start, timer_read ()) < context->ms_timeout
    ? 0 : -1;
}


/* rarp_receiver

   accepts RARP packets from the ethernet receive layer.

*/

int rarp_receiver (struct descriptor_d* d, struct ethernet_frame* frame, 
		   void* context)
{
  extern char host_ip_address[]; /* *** FIXME: please? */
  extern char host_mac_address[];

#if 0
  printf ("%s len %d %d proto %x %x (%d %d)\n", __FUNCTION__, frame->cb, 
	  sizeof (struct header_ethernet) + sizeof (struct header_arp),
	  ETH_F (frame)->protocol,
	  HTONS (ETH_PROTO_RARP),
	  ARP_F (frame)->hardware_address_length,
	  ARP_F (frame)->protocol_address_length);
#endif

	/* Vet the frame */
  if (frame->cb
      < (sizeof (struct header_ethernet) + sizeof (struct header_arp)))
    return 0;			/* runt */

  if (ETH_F (frame)->protocol != HTONS (ETH_PROTO_RARP))
    return 0;
  
  if (   ARP_F (frame)->hardware_address_length != 6
      || ARP_F (frame)->protocol_address_length != 4)
    return -1;			/* unrecognized form, discard */

  DBG (2,"%s: opcode %d \n", __FUNCTION__, HTONS (ARP_F (frame)->opcode));

  switch (ARP_F (frame)->opcode) {

  case HTONS (ARP_REVERSEREPLY):
    if (memcmp (ARP_F (frame)->target_hardware_address,
		host_mac_address, 6))
      break;
    memcpy (host_ip_address, ARP_F (frame)->target_protocol_address, 4);
		/* Add ARP entry for the server */
    arp_cache_update (ARP_F (frame)->sender_hardware_address,
		      ARP_F (frame)->sender_protocol_address, 
		      1);
    /* *** FIXME: this should be done in the ethernet code since we'll
       *** be configuring in different ways */
#if defined (CONFIG_CMD_ALIAS)
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
#endif
    break;
  }

  return 1;
}


int cmd_rarp (int argc, const char** argv)
{
  struct descriptor_d d;
  int result;
  struct ethernet_frame* frame;
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
//  dump (frame->rgb, frame->cb, 0);

  register_ethernet_receiver (100, rarp_receiver, NULL);


  do {
    struct ethernet_timeout_context timeout;

    DBG (1,"%s: send frame\n", __FUNCTION__);
    d.driver->write (&d, frame->rgb, 
		     sizeof (struct header_ethernet)
		     + sizeof (struct header_arp));
    ++tries;

    memset (&timeout, 0, sizeof (timeout));
    timeout.ms_timeout = MS_TIMEOUT;
    result = ethernet_service (&d, autoconf_terminate, &timeout);

    /* result == 1 on success, -1 on timeout  */
  } while (result <= 0 && tries < TRIES_MAX);

  unregister_ethernet_receiver (rarp_receiver, NULL);

  if (UNCONFIGURED_IP)
    printf ("\rRARP failed\n");

#if defined (CONFIG_CMD_ALIAS)
  else {
    const char* sz = alias_lookup ("hostip");
    printf ("\rhostip %s\n", sz);
    sz = alias_lookup ("serverip");
    printf("serverip %s\n", sz);
  }
#endif

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
