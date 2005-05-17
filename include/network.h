/* network.h
     $Id$

   written by Marc Singer
   5 May 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

   Declarations for APEXs simple network stack.

*/

#if !defined (__NETWORK_H__)
#    define   __NETWORK_H__

/* ----- Includes */

/* ----- Types */

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned long  u32;

#define ntohs(v) ((((v) >> 8) & 0xff) | (((v) & 0xff) << 8))
#define htons(v) ((((v) >> 8) & 0xff) | (((v) & 0xff) << 8))

struct header_ethernet {
  u8 destination_address[6];
  u8 source_address[6];
  u16 protocol;
};

struct header_arp {
  u16 hardware_type;
  u16 protocol_type;
  u8  hardware_address_length;
  u8  protocol_address_length;
  u16 opcode;
};

struct header_ipv4 {
  u8  version_ihl;		/* version (4 lsb) and header length (4 msb) */
  u8  tos;			/* type of service */
  u16 length;			/* packet length */
  u32 fragment;
  u8  ttl;
  u8  protocol;
  u16 checksum;
  u32 source_ip;
  u32 destination_ip;
};

struct header_udp {
  u16 source_port;
  u16 destination_port;
  u16 length;
  u16 checksum;
};

/* ----- Globals */

/* ----- Prototypes */

#define ETH_PROTO_IP	0x0800
#define ETH_PROTO_ARP	0x0806
#define ETH_PROTO_RARP	0x8035

#define IP_PROTO_ICMP	1
#define IP_PROTO_UDP	17

#define ARP_HARDW_ETHERNET	1
#define ARP_HARDW_IEEE802	6

#define ARP_REQUEST		1
#define ARP_REPLY		2
#define ARP_REVERSEREQUEST	3
#define ARP_REVERSEREPLY	4
#define ARP_NAK			10

#define PORT_TFTP		69

#define TFTP_RRQ		1
#define TFTP_WRQ		2
#define TFTP_DATA		3
#define TFTP_ACK		4
#define TFTP_ERROR		5
#define TFTP_OACK		6

#define TFTP_ERROR_NONE		0
#define TFTP_ERROR_FILENOTFOUND	1
#define TFTP_ERROR_ACCESSERROR	2
#define TFTP_ERROR_DISKFULL	3
#define TFTP_ERROR_ILLEGALOP	4
#define TFTP_ERROR_UNKNOWNID	5
#define TFTP_ERROR_FILEEXISTS	6
#define TFTP_ERROR_NOUSER	7
#define TFTP_ERROR_OPTIONTERMINATE 8


#endif  /* __NETWORK_H__ */
