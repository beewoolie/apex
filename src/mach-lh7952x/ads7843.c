/* ads7843.c
     $Id$

   written by Marc Singer
   3 Aug 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#include <config.h>
#include <driver.h>
#include <service.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include <mach/hardware.h>

#define TALK 1

#define ADS_CTRL_START	(1<<7)
#define ADS_CTRL_AD(a)	(((a) & 0x7)<<4)


#if defined (TALK)
#define PRINTF(f...)		printf (f)
#define ENTRY(l)		printf ("%s\n", __FUNCTION__)
#define DBG(l,f...)		do { if (TALK >= l) printf (f); } while (0)
#else
#define PRINTF(f...)		do {} while (0)
#define ENTRY(l)		do {} while (0)
#define DBG(l,f...)		do {} while (0)
#endif

#define US_SETTLING		(20)		/* Signal stablization */

#if 0
static void execute_spi_command (int v, int cwrite)
{
  PRINTF ("spi 0x%04x -> 0x%x\n", v & 0x1ff, (v >> CODEC_ADDR_SHIFT) & 0x7f);

  CPLD_SPID = (v >> 8) & 0xff;
  CPLD_SPIC = CPLD_SPIC_LOAD | CPLD_SPIC_CS_CODEC;
  while (!(CPLD_SPIC & CPLD_SPIC_LOADED))
    ;
  CPLD_SPIC = CPLD_SPIC_CS_CODEC;
  while (!(CPLD_SPIC & CPLD_SPIC_DONE))
    ;

  CPLD_SPID = v & 0xff;
  CPLD_SPIC = CPLD_SPIC_LOAD | CPLD_SPIC_CS_CODEC;
  while (!(CPLD_SPIC & CPLD_SPIC_LOADED))
    ;
  CPLD_SPIC = CPLD_SPIC_CS_CODEC;
  while (!(CPLD_SPIC & CPLD_SPIC_DONE))
    ;

  CPLD_SPIC = 0;
}
#endif

#define MS_TIMEOUT 2*1000

static void inline msleep (int ms)
{
  int l = timer_read ();
  while (timer_delta (l, timer_read ()) < ms)
    ;
}

static void inline request (int a)
{
  //  unsigned long timeStart;
	/* --- Request X */
  CPLD_SPID = ADS_CTRL_START | ADS_CTRL_AD(a);
  CPLD_SPIC = CPLD_SPIC_CS_TOUCH | CPLD_SPIC_LOAD;
  while (!(CPLD_SPIC & CPLD_SPIC_LOADED))
    ;
  CPLD_SPIC = CPLD_SPIC_CS_TOUCH;
  while (!(CPLD_SPIC & CPLD_SPIC_DONE))
    ;
//  timeStart = timer_read ();
  msleep (2);
}

static int inline read (void)
{
  int v;

  CPLD_SPID = 0;
  CPLD_SPIC = CPLD_SPIC_CS_TOUCH | CPLD_SPIC_READ | CPLD_SPIC_START;
  while (!(CPLD_SPIC & CPLD_SPIC_LOADED))
    ;
  CPLD_SPIC = CPLD_SPIC_CS_TOUCH | CPLD_SPIC_READ;
  while (!(CPLD_SPIC & CPLD_SPIC_DONE))
    ;
  v = CPLD_SPID;

  CPLD_SPID = 0;
  CPLD_SPIC = CPLD_SPIC_CS_TOUCH | CPLD_SPIC_START | CPLD_SPIC_READ;
  while (!(CPLD_SPIC & CPLD_SPIC_LOADED))
    ;
  CPLD_SPIC = CPLD_SPIC_CS_TOUCH | CPLD_SPIC_READ;
  while (!(CPLD_SPIC & CPLD_SPIC_DONE))
    ;

  v = (v << 5) | (CPLD_SPID >> 3);

  return v;
}

static void inline disable (void)
{
  CPLD_SPIC = 0;
}

static int cmd_ads (int argc, const char** argv)
{
  int x;
  int y;
  extern struct driver_d* console;

  do {

    if (!(CPLD_INT & CPLD_INT_NTOUCH)) {
      request (5);
      x = read ();
      disable ();
      msleep (4);
      request (1);
      y = read ();
      disable ();
      printf ("(%d %d)\n", x, y);
    }
  } while (!console->poll (0, 1));
  {
    char ch;
    console->read (0, &ch, 1);
  }      

  return 0;
}


static __command struct command_d c_ads = {
  .command = "ads",
  .description = "test ADS7843 touch controller",
  .func = cmd_ads,
};

static __service_7 struct service_d ads7843_service = {
//  .init = ads_init,
//  .release = ads_release,
};

