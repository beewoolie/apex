/* arch-arm.h

   written by Marc Singer
   25 Jul 2006

   Copyright (C) 2006 Marc Singer

   -----------
   DESCRIPTION
   -----------

*/

#if !defined (__ARCH_ARM_H__)
#    define   __ARCH_ARM_H__

/* ----- Includes */

/* ----- Types */

/* ----- Globals */

/* ----- Prototypes */

extern void relocate_apex (void);
extern const char* cp15_ctrl (unsigned long);

#endif  /* __ARCH_ARM_H__ */
