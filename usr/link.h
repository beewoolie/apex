/* link.h		-*- C++ -*-

   written by Marc Singer
   15 Jan 2007

   Copyright (C) 2007 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__LINK_H__)
#    define   __LINK_H__

/* ----- Includes */

#include "environment.h"
#include "mtdpartition.h"
#include <map>

/* ----- Types */

class Link {

  struct entry {
    void zero (void) {
      bzero (this, sizeof (*this)); }
    entry () { zero (); index = -1; }
    int index;			// index of this variable in APEX or 0x7f
    const char* key;
    const char* value;
    const char* active;		// Pointer to active copy
  };

  struct EntryMap : public std::map<int, entry> {
    bool contains (int id) {
      return find (id) != end (); }
    iterator find_index (int index) {
      for (iterator it = begin (); it != end (); ++it)
	if ((*it).second.index == index)
	  return it;
      return end (); }
    iterator  find_key (const char* key) {
      for (iterator it = begin (); it != end (); ++it)
	if (strcasecmp ((*it).second.key, key) == 0)
	  return it;
      return end (); }
  };

protected:
  void* pvApex;			/* Copy of APEX firmware from flash */
  void* pvApexSwab;		// Copy of APEX firmware swab'd
  size_t cbApex;		/* Length of APEX firmware */
  unsigned long crcApex;	/* CRC of APEX */

  bool endian_mismatch;		// Controls swab32_maybe
  int env_link_version;		/* 1: legacy; 2: current version */
  struct env_link* env_link;	// Fixed up env_link structure
  int mapping_offset;		// Offset of APEX within mmap'd region
  struct env_d* env;		// APEX environment variables
  int c_env;			// Count of environment variables in APEX

  int fhEnv;			// File handle for mmap'able environment
  void* pvEnv;			// mmap'd environment
  size_t cbEnv;			// Extent of mmap'd environment
  size_t cbEnvUsed;		// Bytes of environment that are in use

  int fhEnvChar;		// File handle for NOR-wise writes
  int fhEnvBlock;		// File handle for erasing
  int ibEnv;			// Index of environment data in file handles

  EntryMap* entries;		// Entries found in flash
  int idNext;			// Next available ID for flash environment

  void zero (void) {
    bzero (this, sizeof (*this)); }

  inline unsigned long swab32(unsigned long l) {
    return (  ((l & 0x000000ffUL) << 24)
	    | ((l & 0x0000ff00UL) << 8)
	    | ((l & 0x00ff0000UL) >> 8)
	    | ((l & 0xff000000UL) >> 24)); }

  inline unsigned long swab32_maybe (unsigned long l) {
    return endian_mismatch ? swab32 (l) : l; }

  inline void swab32_block_maybe (void* pv, int cb) {
    if (!endian_mismatch)
      return;

    unsigned long* pl = (unsigned long*) pv;
    cb /= 4;
    for (; cb--; ++pl)
      *pl = swab32 (*pl); }

  int load_env (void);
  int map_environment (void);
  int scan_environment (void);
  bool open_apex (const MTDPartition& mtd);
  int read_env (void);

public:

  Link () {
    zero ();
    entries = new EntryMap; }

  void open (void);

  void show_environment (void);

  void describe (const char* key);
  void dump (void);
  void eraseenv (void);
  void printenv (const char* key);
  void setenv (const char* key, const char* value);
  void unsetenv (const char* key);

};

/* ----- Globals */

/* ----- Prototypes */



#endif  /* __LINK_H__ */
