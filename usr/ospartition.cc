/* ospartition.cc

 */


#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <regex.h>
#include "ospartition.h"
#include <fcntl.h>

OSPartition*  OSPartition::g_rg;

/* OSPartition::find

   scan the partitions look for the one that contains the given
   offset.  This is used to recover the partition from a raw NOR flash
   address.

*/

bool OSPartition::find (unsigned long addr, char** name, unsigned long* offset)
{
  if (!g_rg)
    init ();
  if (!g_rg)
    return false;

  unsigned long base = 0;
  for (OSPartition* p = g_rg; p->blocks; base += p->blocks*1024, ++p) {
//    printf ("%6d %s\n", p->blocks, p->name);
    if (base + p->blocks*1024 > addr) {
      *name = p->name;
      *offset = addr - base;
      return true;
    }
  }

  return false;
}

void OSPartition::init (void)
{
  char rgb[4096];
  int fh = open ("/proc/partitions", O_RDONLY);
  if (fh == -1)
    return;
  int cb = read (fh, &rgb, sizeof (rgb) - 1);
  close (fh);
  rgb[cb] = 0;

  if (g_rg)
    delete g_rg;
  g_rg = new OSPartition[(cb + 19)/20];

  regex_t rx;
#define SP "[[:space:]]+"
#define D  "[[:digit:]]+"
#define RX SP D SP D SP  "(" D ")" SP "(mtdblock" D ")"

  if (regcomp (&rx, RX, REG_EXTENDED | REG_NEWLINE))
    return;

  regmatch_t m[3];
  char* pb = rgb;
  int c = 0;
  while (*pb) {
    int cbLine = strcspn (pb, "\n");
    char ch = pb[cbLine];
    pb[cbLine] = 0;
    if (regexec (&rx, pb, sizeof (m)/sizeof (*m), m, 0) == 0) {
      g_rg[c].blocks = atoi (pb + m[1].rm_so);
      int cch = m[2].rm_eo - m[2].rm_so;
      g_rg[c].name = new char [cch + 1];
      memcpy (g_rg[c].name, pb + m[2].rm_so, cch);
      g_rg[c].name[cch] = 0;
      printf ("%8d %s\n", g_rg[c].blocks, g_rg[c].name);
      ++c;
    }
    pb += cbLine + (ch ? 1 : 0);
  }
}
