/* service.h

   written by Marc Singer
   3 Nov 2004

   Copyright (C) 2004 Marc Singer

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

*/

#if !defined (__SERVICE_H___)
#    define   __SERVICE_H___

/* ----- Includes */

#include <attributes.h>

/* ----- Types */

struct service_d {
  void (*init) (void);
  void (*report) (void);	/* Informative output from service */
  void (*release) (void);
//  void (*service) (void);	/* Background service */
};

#define __service_0 __used __section(.service.0) /* target */
#define __service_1 __used __section(.service.1) /* exception & mmu*/
#define __service_2 __used __section(.service.2) /* timer */

#define __service_3 __used __section(.service.3) /* serial */
#define __service_4 __used __section(.service.4) /* drv-mem */
#define __service_5 __used __section(.service.5)
#define __service_6 __used __section(.service.6) /* drv-*, ethernet */
#define __service_7 __used __section(.service.7) /* env */

/* ----- Globals */

/* ----- Prototypes */

extern void init_services (void);
extern void release_services (void);

#endif  /* __SERVICE_H___ */
