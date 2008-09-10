/* cmd-image.c

   written by Marc Singer
   3 Aug 2008

   Copyright (C) 2008 Marc Singer

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License
   version 2 as published by the Free Software Foundation.
   Please refer to the file debian/copyright for further details.

   -----------
   DESCRIPTION
   -----------

   o Verifying APEX images.  The format of the APEX image is conducive
     to verification as it is streamed.  Only the header (or the vital
     data from it) needs to be stored as the image is processed.

   o rgbHeader.  We copy the header from the file to a buffer because
     this allows us to verify and load the header in one procedure and
     then process the image in another without requiring malloc().
     The intent is to be able to work with images as a stream and not
     require that the source media support seeking.  The limit on the
     size of the header derives from the fact that the header size
     field is 8 bits and it represents a count of 16 byte pieces.
     This 2^8 * 16 or 4KiB.

   o The source region may not specify a length.  It is useful to
     allow passing of a start address for a region without a length.
     In this case, the image handling code timidly extends the region
     to account for the bytes it knows should exist.

   o Interruptability.  Once we start to read the payloads, this
     command must be interuptable.


*/

#include <config.h>
#include <linux/types.h>
#include <linux/string.h>
#include <apex.h>
#include <command.h>
#include <error.h>
#include <linux/kernel.h>
#include <environment.h>
//#include <service.h>
#include <alias.h>
#include <lookup.h>
#include <driver.h>
#include "region-copy.h"
#include "region-checksum.h"
#include <time.h>

#include <debug_ll.h>

extern unsigned long compute_crc32 (unsigned long crc, const void *pv, int cb);

static const uint8_t signature[] = { 0x41, 0x69, 0x30, 0xb9 };
static char __xbss(image) rgbHeader[(1<<8)*16];

enum {
  sizeZero      = 0x3,          // 11b
  sizeOne       = 0x2,          // 10b
  sizeFour      = 0x1,          // 01b
  sizeVariable  = 0x0,          // 00b
};

enum {
  fieldImageDescription                 = 0x10 | sizeVariable,
  fieldImageCreationDate                = 0x14 | sizeFour,
  fieldPayloadLength                    = 0x30 | sizeFour,
  fieldPayloadLoadAddress               = 0x34 | sizeFour,
  fieldPayloadEntryPoint                = 0x38 | sizeFour,
  fieldPayloadType                      = 0x3c | sizeOne,
  fieldPayloadDescription               = 0x40 | sizeVariable,
  fieldPayloadCompression               = 0x44 | sizeZero,
  fieldLinuxKernelArchitectureID        = 0x80 | sizeFour,
  fieldNOP                              = 0xfc | sizeZero,
};

enum {
  typeNULL = 0,
  typeLinuxKernel = 1,
  typeLinuxInitrd = 2,
};

enum {
  opShow = 1,
  opVerify = 2,
  opLoad = 3,
};

static inline uint32_t swabl (uint32_t v)
{
  return 0
    | ((v & (uint32_t) (0xff <<  0)) << 24)
    | ((v & (uint32_t) (0xff <<  8)) <<  8)
    | ((v & (uint32_t) (0xff << 16)) >>  8)
    | ((v & (uint32_t) (0xff << 24)) >> 24);
}

bool tag_lengths (uint8_t* pb, int* pcbTag, int* pcbData)
{
  uint8_t tag = *pb;
  *pcbTag = 1;
  *pcbData = 0;

//      printf ("tag %x  size %x", tag, tag & 3);
  switch (tag & 3) {
  case sizeZero:	*pcbData = 0; break;
  case sizeOne:		*pcbData = 1; break;
  case sizeFour:	*pcbData = 4; break;
  case sizeVariable:
//        printf (" (variable)");
    *pcbData = pb[1];
    if (*pcbData > 127)
      return false;
    ++*pcbTag;
    break;
  }
//      printf ("  cbTag %d cbData %d\n", cbTag, cbData);
  return true;
}

int verify_apex_image (struct descriptor_d* d)
{
  int result = 0;
  size_t cbHeader;
  uint32_t crc;

  if (d->length - d->index < 5)
    d->length = d->index + 5;

	/* First read the signature and header length */
  result = d->driver->read (d, rgbHeader, 5);
  if (result != 5)
    ERROR_RETURN (ERROR_IOFAILURE, "signature read error");
  if (memcmp (rgbHeader, signature, sizeof (signature)))
    ERROR_RETURN (ERROR_FAILURE, "invalid signature");
  cbHeader = (unsigned char) rgbHeader[4];
  cbHeader *= 16;
  if (!cbHeader)
    ERROR_RETURN (ERROR_FAILURE, "invalid header length");
  if (cbHeader > sizeof (rgbHeader))
    ERROR_RETURN (ERROR_FAILURE, "impossible header length");

  if (d->length - d->index < cbHeader - 5)
    d->length = d->index + cbHeader - 5;

  result = d->driver->read (d, rgbHeader + 5, cbHeader - 5);
  if (result != cbHeader - 5)
    ERROR_RETURN (ERROR_IOFAILURE, "header read error");

      crc = compute_crc32 (0, rgbHeader, cbHeader);
  if (crc != 0)
    ERROR_RETURN (ERROR_FAILURE, "broken header CRC");

  return result;
}

#if defined (CONFIG_CMD_IMAGE_SHOW)

const char* describe_apex_image_type (unsigned long v)
{
  switch (v) {
  default:
  case typeNULL:	return "undefined-image-type";		break;
  case typeLinuxKernel:	return "Kernel";			break;
  case typeLinuxInitrd:	return "Initrd";			break;
  }
};

const char* describe_size (uint32_t cb)
{
  static __xbss(image) char sz[60];

  float s = cb/1024.0;
  char unit = 'K';

  if (s > 1024) {
    s /= 1024.0;
    unit = 'M';
  }
  snprintf (sz, sizeof (sz), "%d bytes (%d.%02d %ciB)",
            cb, (int) s, ((int) (s*100.0))%100, unit);
  return sz;
}

int report_apex_image_header (void)
{
  int cbHeader = rgbHeader[4]*16;
  int i;

  for (i = 0; i < cbHeader; ) {
    int cbTag;
    int cbData;
    uint32_t v = 0;
    char* sz = NULL;

    if (tag_lengths (rgbHeader + i, &cbTag, &cbData))
      ERROR_RETURN (ERROR_FAILURE, "invalid header tag");

    switch (rgbHeader[i] & 3) {
    case sizeZero:
      break;
    case sizeOne:
      v = (unsigned char) rgbHeader[i + cbTag];
      break;
    case sizeFour:
      memcpy (&v, rgbHeader + i + cbTag, sizeof (uint32_t));
      v = swabl (v);
      break;
    case sizeVariable:
      sz = rgbHeader + i + cbTag;
      break;
    }

    switch (rgbHeader[i]) {
    case fieldImageDescription:
      printf ("Image Description:       '%s' (%d bytes)\n", sz,
              strlen (sz) + 1);
      break;
    case fieldImageCreationDate:
      {
        struct tm tm;
        time_t t = v;
        char sz[32];
        asctime_r (gmtime_r (&t, &tm), sz);
        sz[24] = 0;             /* Knock out the newline */
        printf ("Image Creation Date:     %s (0x%x %d)", sz, v, v);
      }
      break;
    case fieldPayloadLength:
      printf ("Payload Length:          %s\n", describe_size (v));
      break;
    case fieldPayloadLoadAddress:
      printf ("Payload Address:         0x%08x\n", v);
      break;
    case fieldPayloadEntryPoint:
      printf ("Payload Entry Point:     0x%08x\n", v);
      break;
    case fieldPayloadType:
      printf ("Payload Type:            %s\n", describe_apex_image_type (v));
      break;
    case fieldPayloadDescription:
      printf ("Payload Description:    '%s' (%d bytes)\n", sz,
              strlen (sz) + 1);
      break;
//    case fieldPayloadCompression:
//      printf ("Payload Compression:    gzip\n");
//      break;
    case fieldLinuxKernelArchitectureID:
      printf ("Linux Kernel Arch ID:    %d (0x%x)\n", v, v);
      break;
    default:
      break;                    // It's OK to skip unknown tags
    }
  }
  return 0;
}

#endif


/** Copy APEX image payloads to memory.  The header must be loaded
    into the global rgbHeader and the descriptor must be ready to read
    the first byte of the first payload.  For kernel payloads, it sets
    environment variables for the entry point and Linux kernel
    architecture ID.  For initrd payloads it sets the environment
    variables for the start address and size of the initrd.  Also, if
    an image has no load address, the payload will be loaded to the
    default address built into APEX. */

int load_apex_image (struct descriptor_d* d)
{
  int result = 0;
  int cbHeader = rgbHeader[4]*16;

  int type = 0;
  unsigned int addrLoad = ~0;
  unsigned int addrEntry = ~0;
  const char* szPayload = NULL;
  int i;

  for (i = 0; i < cbHeader; ) {
    int cbTag;
    int cbData;
    uint32_t v = 0;
    char* sz = NULL;

    if (tag_lengths (rgbHeader + i, &cbTag, &cbData))
      ERROR_RETURN (ERROR_FAILURE, "invalid header tag");

    switch (rgbHeader[i] & 3) {
    case sizeZero:
      break;
    case sizeOne:
      v = (unsigned char) rgbHeader[i + cbTag];
      break;
    case sizeFour:
      memcpy (&v, rgbHeader + i + cbTag, sizeof (uint32_t));
      v = swabl (v);
      break;
    case sizeVariable:
      sz = rgbHeader + i + cbTag;
      break;
    }

    switch (rgbHeader[i]) {
    case fieldImageDescription:
      printf ("Image '%s'\n", sz);
      break;
//    case fieldImageCreationDate:
//      printf ("Image Creation Date:     0x%x (%d)", v, v);
//      break;
    case fieldPayloadLength:
      /* Make sure we have a good load address */
      if (addrLoad == ~0 && type == typeLinuxKernel)
        addrLoad = lookup_alias_or_env_int ("bootaddr", addrLoad);
      if (addrLoad == ~0 && type == typeLinuxInitrd)
        ;                       /* *** FIXME: we should have a place */
      if (addrLoad == ~0)
        ERROR_RETURN (ERROR_FAILURE, "no load address for payload");

		/* Copy and verify the CRC */
      {
        struct descriptor_d dout;
        unsigned long crc;
        unsigned long crc_calc;
        ssize_t cbPadding = 16 - ((v + sizeof (crc)) & 0xf);
        parse_descriptor_simple ("memory", addrLoad, v, &dout);
        result = region_copy (&dout, d, regionCopySpinner);
        if (result)
          return result;
        parse_descriptor_simple ("memory", addrLoad, v, &dout);
        result = region_checksum (&dout,
                                  regionChecksumSpinner | regionChecksumLength,
                                  &crc_calc);
        if (d->driver->read (d, &crc, sizeof (crc)) != sizeof (crc))
          ERROR_RETURN (ERROR_IOFAILURE, "payload CRC missing");
        if (~crc != crc)
          ERROR_RETURN (ERROR_CRCFAILURE, "payload CRC error");
        while (cbPadding--) {
          if (d->driver->read (d, &crc, 1) != 1)
            ERROR_RETURN (ERROR_IOFAILURE, "payload padding missing");
        }
#if defined (CONFIG_ALIASES)
        if (type == typeLinuxKernel && addrEntry != ~0) {
          unsigned addr = lookup_alias_or_env_unsigned ("bootaddr", ~0);
          if (addr != addrEntry)
            alias_set_hex ("bootaddr", addrEntry);
        }
        if (type == typeLinuxInitrd) {
          unsigned size = lookup_alias_or_env_unsigned ("ramdisksize", ~0);
          if (size != v)
            alias_set_hex ("ramdisksize", v);
          if (addrLoad != ~0) {
            unsigned addr = lookup_alias_or_env_unsigned ("ramdiskaddr", ~0);
            if (addr != addrLoad)
              alias_set_hex ("ramdiskaddr", addrLoad);
          }
        }
#endif
        /* *** Set the bootaddr or initrd aliases if not already set */
      }
      type = 0;
      addrLoad = ~0;
      addrEntry = ~0;
      szPayload = NULL;
      break;
    case fieldPayloadLoadAddress:
      addrLoad = v;
      break;
    case fieldPayloadEntryPoint:
      addrEntry = v;
      break;
    case fieldPayloadType:
      type = v;
      break;
    case fieldPayloadDescription:
      szPayload = sz;
      break;
    case fieldLinuxKernelArchitectureID:
      printf ("Linux Kernel Arch ID:    %d (0x%x)\n", v, v);
      break;
    default:
      break;                    // It's OK to skip unknown tags
    }
  }

  return result;
}

int cmd_image (int argc, const char** argv)
{
  struct descriptor_d d;
  int result = 0;
  int op = opShow;

  if (argc < 2)
    return ERROR_PARAM;

  if (argc > 2) {
#if defined (CONFIG_CMD_IMAGE_SHOW)
    if      (PARTIAL_MATCH (argv[1], "s", "how") == 0)
      op = opShow;
#endif
    if (PARTIAL_MATCH (argv[1], "v", "erify") == 0)
      op = opVerify;
    if (PARTIAL_MATCH (argv[1], "l", "oad") == 0)
      op = opLoad;
    else
      ERROR_RETURN (ERROR_PARAM, "unrecognized OP");
    --argc;
    ++argv;
  }

  if ((result = parse_descriptor (argv[1], &d))) {
    printf ("Unable to open '%s'\n", argv[1]);
    goto fail;
  }

  if ((result = verify_apex_image (&d))) {
    goto fail;
    return result;
  }

  switch (op) {
  case opVerify:
    /* *** FIXME: we should check that the payload CRC is correct as well */
    break;
  case opShow:
    result = report_apex_image_header ();
    break;
  case opLoad:
    result = load_apex_image (&d);
    break;
  }

 fail:
  close_descriptor (&d);
  return result;
}

static __command struct command_d c_image = {
  .command = "image",
  .func = cmd_image,
  COMMAND_DESCRIPTION ("image handling")
  COMMAND_HELP(
"image [OP] REGION\n"
"  Operation on an image.\n"
"  Without an OP, the image command verifies the integrity of the image.\n"
"  Choices for OP are\n"
"    load     - load image payload or payloads into memory\n"
#if defined (CONFIG_CMD_IMAGE_SHOW)
"    show     - show the image metadata\n"
#endif
"    verify   - verify the integrity of the image\n"
"  In some cases, the region length may be omitted and the command\n"
"  will infer the\n"
"  length from the image header.\n"
"  e.g.  image show 0xc0000000\n"
"        image verify 0xc1000000\n"
"        image load 0xc0000000\n"
  )
};
