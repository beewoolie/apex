/** @file apex-image.cc

    written by Marc Singer
    2 July 2008

    Copyright (C) 2007,2008 Marc Singer

    @brief Program to create or modify APEX format image files.

    NOTES
    -----

    o CRC should be calculated using an iostream filter.

    o Creation time.  It should be possible to override using the
      current time for the creation time.  We could either be able to
      use the timestamp on another file, set it by a string, or
      suppress the field altogether.

*/

//#define TALK 1

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>

#include <sys/types.h>
#include <regex.h>
#include <argp.h>

#include <list>
#include <vector>

#include "exception.h"
#include "talk.h"

const char* argp_program_version = "apex-image 0.1";

uint8_t signature[] = { 0x41, 0x69, 0x30, 0xb9 };

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

inline uint32_t long swabl (uint32_t v)
{
  return 0
    | ((v & (uint32_t) (0xff <<  0)) << 24)
    | ((v & (uint32_t) (0xff <<  8)) <<  8)
    | ((v & (uint32_t) (0xff << 16)) >>  8)
    | ((v & (uint32_t) (0xff << 24)) >> 24);
}

uint32_t compute_crc32 (uint32_t crc, const void* pv, int cb)
{
#define IPOLY		(0xedb88320)
#define POLY		(0x04c11db7)

#if 1
  // This one works, but I should make sure I know that I cannot use
  // the more efficient algorithm.  More importantly, it would be good
  // to use the same poly that is alredy in APEX.

  const unsigned char* pb = (const unsigned char *) pv;

  while (cb--) {
    int i;
    crc ^= *pb++ << 24;

    for (i = 8; i--; ) {
      if (crc & (1<<31))
        crc = (crc << 1) ^ POLY;
      else
        crc <<= 1;
    }
  }

  return crc;
#endif

#if 0
  unsigned char* pb = (unsigned char*) pv;
  uint32_t poly = POLY;	// Hack to get a register allocated

  crc = crc ^ 0xffffffff;	// Invert because we're continuing

#define DO_CRC\
  if (crc & 1) { crc >>= 1; crc ^= poly; }\
  else	       { crc >>= 1; }

  while (cb--) {
    crc ^= *pb++;

    DO_CRC;
    DO_CRC;
    DO_CRC;
    DO_CRC;
    DO_CRC;
    DO_CRC;
    DO_CRC;
    DO_CRC;
  }

  return crc ^ 0xffffffff;
#endif
}

struct file {

  const char* szPath;
  int fh;
  size_t cb;
  void* pv;
  struct stat stat;

  bool is_open (void) {
    return fh != -1; }
  void release_this (void) {
    if (pv != NULL) {
      munmap (pv, cb);
      pv = NULL;
    }
    if (fh != -1) {
      ::close (fh); }
    fh = -1;
  }

  file () : szPath (NULL), fh (-1), cb (0), pv (NULL) {}
  file (const char* _szPath) : szPath (_szPath), fh (-1), cb (0), pv (NULL) {}
  ~file () {
    release_this (); }

  void set_path (const char* _szPath) {
    release_this ();
    szPath = _szPath; }
  void open (const char* _szPath = NULL) {
    if (_szPath)
      szPath = _szPath;
    fh = ::open (szPath, O_RDONLY);
    if (fh == -1)
      return;
    ::fstat (fh, &stat);
    cb = stat.st_size;
    pv = ::mmap (0, cb, PROT_READ, MAP_FILE | MAP_SHARED, fh, 0);
    DBG (2, "file @%d -> %d b\n", fh, cb);
  }
};


/** The Payload object holds both the mmap'd source file and the
    attributes for the payload in the image. */

struct Payload : public file {

  int type;
  uint32_t load_address;
  uint32_t entry_point;
  const char* description;
  bool compressed;

  size_t header_size (void) {
    return 5                    // Mandatory length
      + (type ? 2 : 0)
      + (load_address != ~0 ? 5 : 0)
      + (entry_point != ~0 ? 5 : 0)
      + (description ? 1 + 1 + strlen (description) + 1 : 0)
      + (compressed ? 1 : 0); }

  uint32_t write_header (uint32_t crc, int fh) {
    unsigned char tag;
    if (type) {
      tag = fieldPayloadType;
      unsigned char _type = type;
      crc = compute_crc32 (crc, &tag, 1);
      crc = compute_crc32 (crc, &_type, 1);
      write (fh, &tag, 1);
      write (fh, &_type, 1);
    }
    if (load_address != ~0) {
      tag = fieldPayloadLoadAddress;
      uint32_t _load_address = swabl (load_address);
      crc = compute_crc32 (crc, &tag, 1);
      crc = compute_crc32 (crc, &_load_address, sizeof (_load_address));
      write (fh, &tag, 1);
      write (fh, &_load_address, sizeof (_load_address));
    }
    if (entry_point != ~0) {
      tag = fieldPayloadEntryPoint;
      uint32_t _entry_point = swabl (entry_point);
      crc = compute_crc32 (crc, &tag, 1);
      crc = compute_crc32 (crc, &_entry_point, sizeof (_entry_point));
      write (fh, &tag, 1);
      write (fh, &_entry_point, sizeof (_entry_point));
    }
    if (description) {
      if (strlen (description) + 1 > 127)
        throw Exception ("description field too long");
      tag = fieldPayloadDescription;
      uint8_t length = strlen (description) + 1;
      crc = compute_crc32 (crc, &tag, 1);
      crc = compute_crc32 (crc, &length, 1);
      crc = compute_crc32 (crc, description, length);
      write (fh, &tag, 1);
      write (fh, &length, 1);
      write (fh, description, length);
    }
    if (compressed) {
      tag = fieldPayloadCompression;
      crc = compute_crc32 (crc, &tag, 1);
      write (fh, &tag, 1);
    }
    // Length is always last
    {
      tag = fieldPayloadLength;
      uint32_t _cb = swabl (cb);
      crc = compute_crc32 (crc, &tag, 1);
      crc = compute_crc32 (crc, &_cb, sizeof (_cb));
      write (fh, &tag, 1);
      write (fh, &_cb, sizeof (_cb));
    }
    return crc;
  }

  Payload () : type (0),
               load_address (~0), entry_point (~0),
    	       description (NULL), compressed (false) {}
  ~Payload () { }

};

struct arguments {
  int architecture_id;

  Payload* m_payload;

  arguments () {
    bzero (this, sizeof (*this)); }

//  int cFiles;
  int mode;
  bool verbose;
  bool force;
  bool ignore_crc_errors;
  uint32_t crc_calculated;

  const char* image_name;

  Payload* payload (void) {
    if (!m_payload)
      m_payload = new Payload;
    return m_payload; }
};

struct Image : public std::list<Payload*>, public file
{
  const char* description;
  time_t timeCreation;
  int architecture_id;
  Payload* payload;             // Pointer to payload as it is constructed

  Image () : description (NULL), timeCreation (0), payload (NULL) {
    timeCreation = time (NULL); }

  void load (const char* szPath);

  size_t header_size (void) {
    return 0
      + (description ? 1 + 1 + strlen (description) + 1 : 0)
      + 5	                // creation time
      + (architecture_id ? 5 : 0);
  }
  uint32_t write_header (uint32_t crc, int fh) {
    unsigned char tag;
    {
      tag = fieldImageCreationDate;
      uint32_t _timeCreation = swabl (timeCreation);
      crc = compute_crc32 (crc, &tag, 1);
      crc = compute_crc32 (crc, &_timeCreation, sizeof (_timeCreation));
      write (fh, &tag, 1);
      write (fh, &_timeCreation, sizeof (_timeCreation));
    }
    if (description) {
      printf ("image has a description %s\n", description);
      if (strlen (description) + 1 > 127)
        throw Exception ("description field too long");
      tag = fieldImageDescription;
      uint8_t length = strlen (description) + 1;
      crc = compute_crc32 (crc, &tag, 1);
      crc = compute_crc32 (crc, &length, 1);
      crc = compute_crc32 (crc, description, length);
      write (fh, &tag, 1);
      write (fh, &length, 1);
      write (fh, description, length);
    }
    if (architecture_id) {
      tag = fieldLinuxKernelArchitectureID;
      uint32_t _id = swabl (architecture_id);
      crc = compute_crc32 (crc, &tag, 1);
      crc = compute_crc32 (crc, &_id, 1);
      write (fh, &tag, 1);
      write (fh, &_id, sizeof (_id));
    }
    return crc;
  }
};

Image g_image;
//std::vector<struct file> g_files;

struct argp_option options[] = {
  { "show",		'i', 0, 0, "Mode to show an image header", 1 },
  { "update",		'u', 0, 0, "Mode to update image" },
  { "create",		'c', 0, 0, "Mode to create a new image" },

  { "load-address",	'l', "ADDR", 0, "Set load address for image", 2 },
  { "entry-point",	'e', "ADDR", 0, "Set entry point for image" },
  { "load-and-entry",	'L', "ADDR", 0,
				"Set load address and entry point for image" },
//  { "architecture",	'A', "ARCH", 0, "Set architecture [arm]" },
//  { "os",		'O', "OS", 0, "Set operating system [GNU/Linux]" },
  { "image-description", 'n', "LABEL", 0, "Set description of image" },
  { "type",		't', "TYPE", 0, "Set payload type [kernel]" },
  { "description",       'd', "LABEL", 0, "Set description of payload" },

  { "force",		'f', 0, 0, "Force overwrite of output file", 3 },
  { "verbose",		'v', 0, 0, "Verbose output, when available", 3 },
  { "ignore-crc-errors", 'C', 0, 0, "Ignore CRC errors", 3 },

  { 0 }
};

error_t arg_parser (int key, char* arg, struct argp_state* state);

struct argp argp = {
  options, arg_parser,
  "FILE...",
  "  Create, update, or view the header of a U-BOOT image file\n"
  "\v"
  "  In the absence of any mode options, the program will show the header\n"
  "of an existing image.\n"
  "  Update mode may be used to insert a header in a data file that doesn't\n"
  "already have a header.  Otherwise, update mode is useful for modifying\n"
  "header parameters for an existing image, the load address for example.\n"
  "  Images may be created from one or more input files.  If the file type\n"
  "is not Multi, the inputs are simply contenated form the payload of the\n"
  "image.  For Multi images, a 0 terminated sub-imagge size array \n"
  "immediately follows the header after which the the input files are\n"
  "contenated and written.\n"
  "\n"
  "  ADDR is a 32 bit number in decimal or hexadecimal if prefixed with 0x\n"
  "  ARCH is one of: arm, mips, ppc, sh, x86\n"
  "  OS   is one of:   gnu/linux, linux\n"
  "  TYPE is one of: kernel, initrd, ramdisk, multi\n"
  "  FILE is either a filename, or a padding operator.\n"
  "\n"
  "  Padding operators may be used only when creating new images.  They start\n"
  "with an asterisk '*' and are of four types.  '*@SIZE' pads the image to\n"
  "SIZE bytes.  '*_SIZE' pads to SIZE bytes, but includes for the U-BOOT\n"
  "headers when padding the image data.  For a single image type, this means\n"
  "that the whole image file will be SIZE bytes long.  '*@*SIZE' pads enough\n"
  "to round the image data to a multiple of SIZE bytes.  '*_*SIZE' does the\n"
  "same rounding, but it accounts for the U-BOOT headers.\n"
  "  Padding is always filled with 0xff to be compatible with flash memory.\n"
  "  Quoting may be necessary to avoid shell globbing.\n"
  "\n"
  "  e.g. apex-image uImage                    # Show image header\n"
  "       apex-image -u -L 0x8000 uImage       # Change the load/entry point\n"
  "                                            # Create 2MiB kernel uImage\n"
  "       apex-image -c -L 0x8000 -A arm -t kernel vmlinuz '*_2m' uImage\n"
};

enum {
  modeNone = 0,
  modeShow,
  modeUpdate,
  modeCreate,
};

uint32_t interpret_number (const char* sz)
{
  int base = 10;
  if (strlen (sz) > 2 && sz[0] == '0' && tolower (sz[1]) == 'x') {
    base = 16;
    sz += 2;
  }
  char* pb;
  uint32_t value = (uint32_t) strtoul (sz, &pb, base);
  if (tolower (*pb) == 'k')
    value *= 1024;
  if (tolower (*pb) == 'm')
    value *= 1024*1024;
  return value;
}

int interpret_image_type (const char* sz)
{
  int value = typeNULL;

  if (strcasecmp (sz, "ramdisk") == 0)
    value = typeLinuxInitrd;
  if (strcasecmp (sz, "initrd") == 0)
    value = typeLinuxInitrd;
  if (strcasecmp (sz, "kernel") == 0)
    value = typeLinuxKernel;

  return value;
}

const char* interpret_size (uint32_t cb)
{
  static char sz[128];

  float s = cb/1024.0;
  char unit = 'K';

  if (s > 1024) {
    s /= 1024.0;
    unit = 'M';
  }
  snprintf (sz, sizeof (sz), "%d bytes (%d.%02d %ciB)",
            cb, (int) s, (int (s*100.0))%100, unit);
  return sz;
}

error_t arg_parser (int key, char* arg, struct argp_state* state)
{
  struct arguments& args = *(struct arguments*) state->input;

  switch (key) {
  case 'i':
    if (args.mode)
      argp_error (state, "only one mode (-i, -u, -c) may be set");
    args.mode = modeShow;
    break;
  case 'u':
    if (args.mode)
      argp_error (state, "only one mode (-i, -u, -c) may be set");
    args.mode = modeUpdate;
    break;
  case 'c':
    if (args.mode)
      argp_error (state, "only one mode (-i, -u, -c) may be set");
    args.mode = modeCreate;
    break;

  case 'l':
    args.payload ()->load_address = interpret_number (arg);
    break;

  case 'e':
    args.payload ()->entry_point = interpret_number (arg);
    break;

  case 'L':
    args.payload ()->load_address = args.payload ()->entry_point
      = interpret_number (arg);
    break;

//  case 'A':
//    args.architecture = interpret_architecture (arg);
//    if (args.architecture == 0)
//      argp_error (state, "unrecognized architecture '%s'", arg);
//    break;

//  case 'O':
//    args.os = interpret_os (arg);
//    if (args.os == 0)
//      argp_error (state, "unrecognized operating system '%s'", arg);
//    break;

  case 'n':
    g_image.description = arg;
    break;

  case 'd':
    args.payload ()->description = arg;
    break;

  case 't':
    args.payload ()->type = interpret_image_type (arg);
    if (args.payload ()->type == 0)
      argp_error (state, "unrecognized image type '%s'", arg);
    break;

  case 'f':
    args.force = true;
    break;

  case 'v':
    args.verbose = true;
    break;

  case 'C':
    args.ignore_crc_errors = true;
    break;

  case ARGP_KEY_ARG:
    args.payload ()->set_path (arg);
    g_image.push_back (args.payload ());
    args.m_payload = NULL;
    break;

  case ARGP_KEY_END:

    if (!g_image.size ())
      argp_usage (state);

    switch (args.mode) {
    default:
      args.mode = modeShow;
      // -- fall through
    case modeShow:
      if (g_image.size () != 1)
        argp_error (state, "show mode requires one and only one file");
      break;
    case modeUpdate:
      if (g_image.size () != 1)
        argp_error (state, "update mode requires one and only one file");
      break;
    case modeCreate:
      if (g_image.size () < 2)
        argp_error (state,
           "create mode requires at least two files, and input and an output");
      break;
    }
    break;

  default:
    return ARGP_ERR_UNKNOWN;
  }
  return 0;
}

const char* describe_image_type (unsigned long v)
{
  switch (v) {
  default:
  case typeNULL:	return "undefined-image-type";		break;
  case typeLinuxKernel:	return "Kernel";			break;
  case typeLinuxInitrd:	return "Initrd";			break;
  }
};


void Image::load (const char* szPath)
{
  open (szPath);                // Open the source file

  // *** Parse the file header and generate the Payloads
}


/** Show and possibly verify an image header.  The pv/cb is the memory
    to scan for the header which must be the first data in the
    region. */

void report_header (struct arguments& args, void* pv, size_t cb)
{
  if (cb < 16)
    throw Exception ("invalid header");

  if (memcmp (pv, signature, sizeof (signature)) != 0)
    throw Exception ("no image signature found");

  size_t cbHeader = ((uint8_t*) pv)[4]*16;
  if (cbHeader > cb)
    throw Exception ("header_size invalid");

  uint32_t crc = compute_crc32 (0, pv, cbHeader);
  if (crc != 0 && !args.ignore_crc_errors)
    throw Exception ("incorrect header CRC");
  crc = compute_crc32 (0, pv, cbHeader - 4);

  // At this point, we have what we believe is a bona fide image
  // header.  We skip the signature and header_size.  The loop on
  // fields proceeds until the last byte before the CRC.

  if (args.verbose) {
    printf ("Signature:            0x%02x 0x%02x 0x%02x 0x%02x\n",
            ((uint8_t*)pv)[0], ((uint8_t*)pv)[1],
            ((uint8_t*)pv)[2], ((uint8_t*)pv)[3]);
    printf ("Header Size:          %d (%d bytes)\n",
            ((uint8_t*)pv)[4], ((uint8_t*)pv)[4]*16);
  }

  pv = (char*) pv + 5;
  cbHeader -= 5;

  while (cbHeader > 4) {
    uint8_t tag = *(uint8_t*) pv;
    size_t cbField = 1;
    size_t cbData = 0;
//    printf ("tag %x  size %x\n", tag, tag & 3);
    switch (tag & 3) {
    case sizeZero:		cbData = 0; break;
    case sizeOne:		cbData = 1; break;
    case sizeFour:		cbData = 4; break;
    case sizeVariable:
//      printf ("variable size\n");
      cbData = ((uint8_t*)pv)[1];
      if (cbData > 127)
        throw Exception ("invalid field length");
      ++cbField;
      break;
    }
    cbField += cbData;

    uint32_t v;
    memcpy (&v, &((uint8_t*)pv)[1], 4);
    v = swabl (v);

//    printf ("tag %x %d %d\n", tag, cbField, cbData);
    switch (tag) {
    case fieldImageDescription:
//      printf ("image description\n");
      printf ("Image Description:    %s\n", &((char*)pv)[2]);
      break;
    case fieldImageCreationDate:
      {
        time_t t = v;
        printf ("Image Creation Date:  %s", asctime (localtime (&t)));
      }
      break;
    case fieldPayloadLength:
      printf ("Payload Length:       %s\n", interpret_size (v));
      break;
    case fieldPayloadLoadAddress:
      printf ("Payload Load Address: 0x%08lx\n", v);
      break;
    case fieldPayloadEntryPoint:
      printf ("Payload Entry Point:  0x%08lx\n", v);
      break;
    case fieldPayloadType:
      printf ("Payload Type:         %s\n",
              describe_image_type (((uint8_t*)pv)[1]));
      break;
    case fieldPayloadDescription:
      printf ("Payload Description:  \n", &((char*)pv)[2]);
      break;
    case fieldPayloadCompression:
      printf ("Payload Compression:  gzip\n");
      break;
    case fieldLinuxKernelArchitectureID:
      printf ("Linux Kernel Architecture ID: %d\n", v);
      break;
    case fieldNOP:
//      printf ("NOP:\n");
      break;
    default:
      break;                    // It's OK to skip unknown tags
    }

    if (cbField > cbHeader)
      throw Exception ("field size exceeds header length");
    cbHeader -= cbField;
    pv = (char*) pv + cbField;
  }

  if (crc != 0 || args.verbose || args.ignore_crc_errors) {
    uint32_t header_crc;
    memcpy (&header_crc, pv, 4);
    header_crc = swabl (header_crc);
    bool ok = crc == header_crc;

    printf ("Header CRC:           0x%08x %s 0x%08x %s\n",
            header_crc, ok ? "==" : "!=", crc, ok ? "" : "ERROR");
  }
}

#if 0

/** Show and verify the contents of an image header.  The args
    parameter is used to access user options such as 'verbose'.  The
    pv and cb are the payload of the image file and may be used to
    pull the sizes of sub-components in a multi-image.
*/

void report_header (struct arguments& args, struct header* header,
                    void* pv, size_t cb)
{
  if (header->magic != swabl (MAGIC)) {
    printf ("file is not a U-BOOT image\n");
    return;
  }

  time_t time = swabl (header->time);
  float s = swabl (header->size)/1024.0;
  char unit = 'K';

  if (s > 1024) {
    s /= 1024.0;
    unit = 'M';
  }

  printf ("Image Name:    %-32.32s\n",
	  *header->image_name ? header->image_name : "<null>");
  printf ("Creation Date: %s", asctime (localtime (&time)));
  printf ("Image Type:    %s %s %s (%s)\n",
	  describe_architecture (header->architecture),
	  describe_os (header->os),
	  describe_image_type (header->image_type),
	  describe_compression (header->compression));
  printf ("Data Size:     %s\n",
          interpret_size (swabl (header->size)));
  printf ("Load Address:  0x%08lx\n", swabl (header->load_address));
  printf ("Entry Point:   0x%08lx\n", swabl (header->entry_point));

  uint32_t header_crc = swabl (header->header_crc);
  header->header_crc = 0;
  uint32_t header_crc_verify = compute_crc32 (0, header, sizeof (*header));
  if (args.verbose || header_crc != header_crc_verify) {
    printf ("Header CRC:    0x%08lx", header_crc);
    if (header_crc == header_crc_verify)
      printf (" OK\n");
    else
      printf (" ERROR (!= 0x%08lx)\n", header_crc_verify);
  }
  header->header_crc = header_crc;

  if (args.verbose) {
    printf ("Magic:         0x%08lx\n", header->magic);
    printf ("Time:          0x%08lx\n", header->time);
  }
  if (args.verbose || args.crc_calculated != swabl (header->crc)) {
    printf ("CRC:           0x%08lx", swabl (header->crc));
    if (args.crc_calculated == swabl (header->crc))
      printf (" OK\n");
    else
      printf (" ERROR (!= 0x%08lx)\n", args.crc_calculated);
  }

  if (header->image_type == typeMulti) {
    printf ("Multi-Image Contents:\n");
    if (pv == NULL || cb == 0)
      printf ("  No payload to interpret\n");
    else {
      int c = 0;
      while (cb >= sizeof (uint32_t) && c < 32) {
        uint32_t cbData = swabl (*(uint32_t*) pv);
        if (cbData == 0)
          break;
        printf ("  Sub-image %d: %s\n", c, interpret_size (cbData));
        pv = (char*) pv + sizeof (uint32_t);
        cb -= 4;
        ++c;
      }
    }
  }
}
#endif


void apeximage (struct arguments& args)
{
	// Open files and construct pads
  for (Image::iterator it = g_image.begin ();
       it != g_image.end (); ++it) {
    if (!(*it)->szPath)
      throw Exception ("file has no path?");
    (*it)->open ();
  }

  Payload& payloadIn = *(*g_image.begin ());
  Payload& payloadOut = *(*g_image.rbegin ());

  printf ("fileIn %s (%d B header)\n", payloadIn.szPath,
          payloadIn.header_size ());
  printf ("fileOut %s (%d B header)\n", payloadOut.szPath,
          payloadOut.header_size ());

  switch (args.mode) {
  case modeUpdate:
  case modeShow:
    if (payloadOut.fh == -1 || payloadOut.pv == NULL)
      throw Exception ("unable to read file '%s'", payloadOut.szPath);
#if 0
    if (payloadOut.pv != NULL
        && payloadOut.cb >= sizeof (header))
      memcpy (&header, payloadOut.pv, sizeof (header));
#endif

    break;
  case modeCreate:
    if (payloadOut.fh != -1 && !args.force)
      throw Exception ("unable to write file '%s', file exists",
                       payloadOut.szPath);
    break;
  }

  if (args.mode == modeShow) {
    report_header (args, payloadIn.pv, payloadIn.cb);
    return;
  }


  if (args.mode == modeCreate) {
		// Save the new image to a temporary filename
    size_t cbPathSave = strlen (payloadOut.szPath) + 32;
    char* szPathSave = (char*) alloca (cbPathSave);
    snprintf (szPathSave, cbPathSave, "%s~%d~", payloadOut.szPath, getpid ());
    int fhSave = open (szPathSave, O_WRONLY | O_TRUNC | O_CREAT, 0666);
    if (fhSave == -1)
      throw Exception ("unable to create output file '%s'", szPathSave);

    uint32_t crc = 0;
    crc = compute_crc32 (crc, &signature, sizeof (signature));
    write (fhSave, &signature, sizeof (signature));
    int header_size = sizeof (signature) + 1 + g_image.header_size ();
    printf ("base header is %d bytes\n", header_size);
    for (Image::iterator it = g_image.begin ();
         it != g_image.end (); ++it) {
      if ((*it) == &payloadOut)
        break;
      header_size += (*it)->header_size ();
    }
    header_size += 4;           // CRC
    printf ("total header with crc is %d bytes\n", header_size);
    unsigned char _header_size = (header_size + 15)/16;
    crc = compute_crc32 (crc, &_header_size, 1);
    write (fhSave, &_header_size, 1);
    printf ("crc of sig and size 0x%08x\n", crc);
    crc = g_image.write_header (crc, fhSave);
    printf ("crc of sig and size and image fields 0x%08x\n", crc);
    for (Image::iterator it = g_image.begin ();
         it != g_image.end (); ++it) {
      if ((*it) == &payloadOut)
        break;
      crc = (*it)->write_header (crc, fhSave);
    }

    for (header_size = _header_size*16 - header_size; header_size--; ) {
      unsigned char b = 0xff;
      crc = compute_crc32 (crc, &b, 1);
      write (fhSave, &b, 1);
    }
    crc = swabl (crc);
    write (fhSave, &crc, sizeof (crc));

    // Write the payloads
    for (Image::iterator it = g_image.begin ();
         it != g_image.end (); ++it) {
      if ((*it) == &payloadOut)
        break;
      crc = 0;
      crc = compute_crc32 (crc, (*it)->pv, (*it)->cb);
      write (fhSave, (*it)->pv, (*it)->cb);
      crc = swabl (crc);
      write (fhSave, &crc, sizeof (crc));

      printf ("%x length  %d->%d\n",
              swabl (crc), (*it)->cb, 16 - (((*it)->cb + 4) & 0xf));
      for (int cbPad = 16 - (((*it)->cb + 4) & 0xf); cbPad--; ) {
        unsigned char b = 0xff;
        write (fhSave, &b, 1);
      }
    }

    close (fhSave);

		// Release all files that could be the same as the output
    payloadIn.release_this ();
    payloadOut.release_this ();

    if (rename (szPathSave, payloadOut.szPath)) {
      // I'm not sure it is possible to get this error since we've
      // already been able to write a temporary file to the same directory.
      unlink (szPathSave);
      throw ResultException (errno, "unable to rename '%s' to '%s' (%s)",
                             szPathSave, payloadOut.szPath, strerror (errno));
    }
  }

#if 0

	// Adjust payload of first file in case we're updating; calc CRC
  void* pvIn = payloadIn.pv;
  size_t cbIn = payloadIn.cb;
  if (args.mode == modeUpdate || args.mode == modeShow) {
    if (header.magic == swabl (MAGIC)) {
      memcpy (&header, pvIn, sizeof (header));
      pvIn = (char*) pvIn + sizeof (header);
      cbIn -= sizeof (header);
    }
    if (args.compression == 0 && cbIn > 2
        && ((uint8_t*)pvIn)[0] == 0x1f
        && ((uint8_t*)pvIn)[1] == 0x8b)
      args.compression = compGZIP;
	// Compute CRC in case we're showing or being verbose
    args.crc_calculated = compute_crc32 (0, pvIn, cbIn);
  }

  if (args.mode == modeShow) {
    report_header (args, &header, pvIn, cbIn);
    return;
  }

  	// Compute array of sizes for multi-image and cbImageSize
  int cSizes =
    (args.mode == modeCreate && args.image_type == typeMulti)
    ? (args.cFiles - 1 + 1) : 0;
  uint32_t rgSizes[cSizes];
  bzero (rgSizes, sizeof (rgSizes));

	// Perform a fixup on the padding as we now know the true
	// length of the sizes array.  This only affects the first
	// pad and when that pad is absolute
  if (rgfile[1].is_pad_absolute ())
    rgfile[1].cb -= cSizes*sizeof (uint32_t);
  if (rgfile[1].is_pad_absolute_roundup ())
    rgfile[1].cb = (rgfile[1].cb - (rgfile[0].cb + sizeof (header)
                                    + sizeof (rgSizes))%rgfile[1].cb)
      %rgfile[1].cb;

  uint32_t cbImageSize = sizeof (rgSizes);
  {
    int iSize = -1;
    for (int i = 0; i < args.argc; ++i) {
      if (!rgfile[i].is_pad ())
        ++iSize;
      // We always use size of single file, if there is only one.
      // Otherwise, we use size of every file except the last one.
      // The size of the first file doesn't include the U-BOOT header
      // should one be present.  Whew.
      size_t cbImage = (i && i == args.argc - 1)
        ? 0 : (i ? rgfile[i].cb : cbIn);
      if (iSize < cSizes)
        rgSizes[iSize] = swabl (swabl (rgSizes[iSize]) + cbImage);
      DBG (2, "size #(%d->%d) %d b %d b\n",
           i, iSize, cSizes ? swabl (rgSizes[iSize]) : 0, cbImage);
      cbImageSize += cbImage;
    }
  }

	// Compute CRC, include sizes array if there is one
  uint32_t crc = compute_crc32 (0, &rgSizes, sizeof (rgSizes));
  crc = compute_crc32 (crc, pvIn, cbIn);
  for (int i = 1; i < args.argc - 1; ++i)
    crc = compute_crc32 (crc, rgfile[i].pv, rgfile[i].cb);

	// Fixup header
  header.magic	= swabl (MAGIC);
  header.time	= swabl (payloadIn.stat.st_mtime);
  header.size	= swabl (cbImageSize);

	// Update the header to reflect our new reality
  if (args.load_address != ~0)
    header.load_address		= swabl (args.load_address);
  if (args.entry_point  != ~0)
    header.entry_point		= swabl (args.entry_point);
  if (args.architecture != archNULL)
    header.architecture		= args.architecture;
  if (args.os           != osNULL)
    header.os			= args.os;
  if (args.image_type   != typeNULL)
    header.image_type		= args.image_type;
  header.compression		= args.compression;

  // Defaults in the absense of input from the user
  if (header.architecture == archNULL)
    header.architecture = archARM;
  if (header.os == osNULL)
    header.os = osGNULinux;
  if (header.image_type == typeNULL)
    header.image_type = typeKernel;

  header.crc			= swabl (crc);
  if (args.image_name) {
    header.image_name[0] = 0;
    strncat (header.image_name, args.image_name, sizeof (header.image_name));
  }
  header.header_crc		= 0;
  header.header_crc		= swabl (compute_crc32 (0, (void*) &header,
                                                        sizeof (header)));
  if (args.verbose)
    report_header (args, &header, NULL, 0);

		// Save the new image to a temporary filename
  size_t cbPathSave = strlen (payloadOut.szPath) + 32;
  char* szPathSave = (char*) alloca (cbPathSave);
  snprintf (szPathSave, cbPathSave, "%s~%d~", payloadOut.szPath, getpid ());
  int fhSave = open (szPathSave, O_WRONLY | O_TRUNC | O_CREAT, 0666);
  if (fhSave == -1)
    throw Exception ("unable to create output file '%s'", szPathSave);
  write (fhSave, &header, sizeof (header));

		// Write multi-image sub-image size array
  if (sizeof (rgSizes))
    write (fhSave, rgSizes, sizeof (rgSizes));

		// Write the first input file
  write (fhSave, pvIn, cbIn);

		// Write sub-images and padding, except for the first
  if (args.argc > 2)
    for (int i = 1; i < args.argc - 1; ++i)
      write (fhSave, rgfile[i].pv, rgfile[i].cb);

  close (fhSave);

		// Release all files that could be the same as the output
  payloadIn.release_this ();
  payloadOut.release_this ();

  if (rename (szPathSave, payloadOut.szPath)) {
    // I'm not sure it is possible to get this error since we've
    // already been able to write a temporary file to the same directory.
    unlink (szPathSave);
    throw ResultException (errno, "unable to rename '%s' to '%s' (%s)",
                           szPathSave, payloadOut.szPath, strerror (errno));
  }
#endif
}

int main (int argc, char** argv)
{
  struct arguments args;
  argp_parse (&argp, argc, argv, ARGP_IN_ORDER, 0, &args);

  try {
    apeximage (args);
  }
  catch (Exception& ex) {
    printf ("%s\n", ex.sz);
  }

  return 0;
}
