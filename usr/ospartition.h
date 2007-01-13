/* ospartition.h

 */

#include <string.h>

struct OSPartitionMatch;

struct OSPartition {
  int blocks;
  char* name;

  static OSPartition* g_rg;
  static void init (void); 

  void zero (void) { bzero (this, sizeof (*this)); }
  OSPartition () { zero (); }

  static bool find (unsigned long addr, char** name, unsigned long* offset);

};

struct OSPartitionMatch
{
  char* name;
  int offset;
};
