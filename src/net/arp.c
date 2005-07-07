/* arp.c
     $Id$

   written by Marc Singer
   7 Jul 2005

   Copyright (C) 2005 Marc Singer

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

const char szNetDriver[] = "eth:";

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

  printf ("%s: open %s -> %d\n", __FUNCTION__, szNetDriver, result);

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
