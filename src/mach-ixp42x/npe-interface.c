/* npe-interface.c
     $Id$

   written by Marc Singer
   8 Mar 2005

   Copyright (C) 2005 Marc Singer

   -----------
   DESCRIPTION
   -----------

   Interface file that links compiles with the access library headers.
   This code is separated because of the extensive conflicts between
   the access library and the bootloader.

*/

#include <IxNpeDl.h>
#include <IxQMgr.h>
#include <IxNpeMh.h>
#include <IxEthAcc.h>
#include <ix_ossl.h>

//#include <_intel/include/npe_info.h>

#define NPE_VERSIONS_COUNT 12

extern const unsigned IX_NPEDL_MicrocodeImageLibrary[];

static int npeDownload (int npeId)
{
  UINT32 i, n;
  IxNpeDlVersionId list[NPE_VERSIONS_COUNT];
  IxNpeDlVersionId dlVersion;
  int major, minor;

  if (ixNpeDlAvailableVersionsCountGet(&n) != IX_SUCCESS)
    return 0;

  if (n > NPE_VERSIONS_COUNT)
    return 0;

  if (ixNpeDlAvailableVersionsListGet(list, &n) != IX_SUCCESS)
    return 0;
    
  major = minor = 0;
  for (i = 0; i < n; i++) {
    if (list[i].npeId == npeId) {
      if (list[i].buildId == buildId) {
	if (list[i].major > major) {
	  major = list[i].major;
	  minor = list[i].minor;
	} else if (list[i].major == major && list[i].minor > minor)
	  minor = list[i].minor;
      }
    }
  }

  printf ("NPE%c major[%d] minor[%d] build[%d]\n",
	  npeId  + 'A', major, minor, buildId);


    if (ixNpeDlNpeStopAndReset(npeId) != IX_SUCCESS) {
      printf("npeDownload: Failed to stop/reset NPE%c\n", npeId  + 'A');
      return 0;
    }
    dlVersion.npeId = npeId;
    dlVersion.buildId = buildId;
    dlVersion.major = major;
    dlVersion.minor = minor;

    if (ixNpeDlVersionDownload(&dlVersion, 1) != IX_SUCCESS) {
      printf("Failed to download to NPE%c\n", npeId  + 'A');
      return 0;
    }


#if 0				/* Verify */
    // verify download
    if (ixNpeDlLoadedVersionGet(npeId, &dlVersion) != IX_SUCCESS) {
      printf ("Failed to upload version from NPE%c\n", npeId  + 'A');
      return 0;
    }

    if (   dlVersion.buildId != buildId
	|| dlVersion.major != major
	|| dlVersion.major != major) {
      printf ("Failed to verify download NPE%c\n", npeId  + 'A');
      return 0;
    }
#endif

    return 1;

}

int _npe_init (void)
{
//  const unsigned* pl = IX_NPEDL_MicrocodeImageLibrary;
//  volatile unsigned l = *pl;

#if 0
  npe_alloc_end = npe_alloc_pool + sizeof(npe_alloc_pool);
  npe_alloc_free = (cyg_uint8 *)(((unsigned)npe_alloc_pool
				  + HAL_DCACHE_LINE_SIZE - 1)
				 & ~(HAL_DCACHE_LINE_SIZE - 1));
#endif

  if (ixQMgrInit() != IX_SUCCESS) {
    printf("Error initialising queue manager!\n");
    return 0;
  }

  if (!npeDownload(0))
    return 0;

  if (!npeDownload(1))
    return 0;

}
