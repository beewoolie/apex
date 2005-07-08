/* arp.c
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

   ARP command for testing.

*/

#include <config.h>
#include <linux/string.h>
#include <linux/types.h>
//#include <linux/ctype.h>
#include <apex.h>
#include <command.h>
#include <driver.h>
#include <error.h>
//#include <spinner.h>

#include <network.h>
#include <ethernet.h>

//#define TALK

#if TALK > 0
# define DBG(l,f...)		if (l <= TALK) printf (f);
#else
# define DBG(l,f...)		do {} while (0)
#endif

int cmd_arp (int argc, const char** argv)
{
  extern struct driver_d* console_driver;
  struct descriptor_d d;
  int result;
  struct ethernet_frame* frame;
  char ch;

  if (   (result = parse_descriptor (szNetDriver, &d))
      || (result = open_descriptor (&d))) 
    return result;

  DBG (2,"%s: open %s -> %d\n", __FUNCTION__, szNetDriver, result);

  frame = ethernet_frame_allocate ();

  do {
    frame->cb = d.driver->read (&d, frame->rgb, FRAME_LENGTH_MAX);
    if (frame->cb > 0) {
      ethernet_receive (&d, frame);
      frame->cb = 0;
    }
  } while (!console_driver->poll (0, 1));

  console_driver->read (0, &ch, 1);

  ethernet_frame_release (frame);

  close_descriptor (&d);  
  return 0;
}

static __command struct command_d c_arp = {
  .command = "arp",
  .description = "test arp protocol",
  .func = cmd_arp,
  COMMAND_HELP(
"arp\n"
"  Enter a loop to handle arp requests.\n"
  )
};