/* rfc2440.h		-*- C++ -*-
     $Id: rfc2440.h,v 1.1 2003/08/11 08:26:23 elf Exp $

   written by Marc Singer
   11 Aug 2003

   Copyright (C) 2003 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__RFC2440_H__)
#    define   __RFC2440_H__

/* ----- Includes */

/* ----- Types */

class rfc2440 {
protected:
  const char* m_rgb;
  size_t m_cb;

  void zero (void) {
    bzero (this, sizeof (*this)); }
  void release_this (void) {}
  void init (const char* rgb, size_t cb) {
    m_rgb = rgb; m_cb = cb; }

public:
  rfc2440 () {
    zero (); }
  rfc2440 (const char* rgb, size_t cb) {
    zero (); init (rgb, cb); }
  ~rfc2440 () {
    release_this (); }

  bool is_signature (void); 
  size_t size (void);
  time_t timestamp (void);
  const char* timestring (void);
  const char* describe_id (void);

};

/* ----- Globals */

/* ----- Prototypes */



#endif  /* __RFC2440_H__ */
