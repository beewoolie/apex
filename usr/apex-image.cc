/** @file apex-image.cc

    written by Marc Singer
    2 July 2008

    Copyright (C) 2007,2008 Marc Singer

    @brief Program to create or modify APEX format image files.

    TODO
    ----

    o Auto detect mode.  This was done, to some extent, in the
      ubootimage command.  It should be apparent from the command line
      whether or not we are updating, showing or creating a new image.
      o Much of this is done.  We need to think about update more
        because there are some possible edge cases with it.  What if
        the user specifies a single file, an image, and some metadata
        for the payload.  Should we require the '.' file for updating?
        There is no valid create mode with that.  Should we default to
        create if there is more than one file and none of them are
        update '.' files?  The use would then need to force or
        override.  Also, what happens if there are more input files
        for an update than were in the existing image?  Fewer input
        files?

    o Support update.  Especially, changes to the metadata.

    o Define how the output will look in normal, verbose, and quiet
      modes.

    o Fixup comments for doxygen.

    o Document the file format in this file, or refer to another
      file.  It should, at a minimum, be in this directory.

    o Header::Iterator should have some protection against running
      past the end of a header in case the data is corrupt.  Done but
      not tested.

    o There is some redundancy in that the report_header() code as well
      as the load() code needs to know about all of the tags.  There
      should be some protection against dropping unknown tags.
      Perhaps the update code will require a switch to ignore
      unrecognized tags.


    NOTES
    -----

    o CRC could/should be calculated using an iostream filter.

    o Creation time.  It should be possible to override the use of the
      current time for the creation time.  We could either be able to
      use the timestamp on another file, set it by a string, or
      suppress the field altogether.
      o If we do this, the modified() operator needs to do the right
        thing since we now ignore the creation time.

    o modeUpdate.  Up until now, the only thing we've been able to do
      when updating an image are changes to the metadata.  The command
      line needs to support this in a useful way.  For example, we
      could use a special marker for 'same file that is already in the
      image'.  However, it could be handy to allow replacement of one
      of the components of an image.  For example, revise the initrd
      and leave the kernel intact.  In fact, it could be inferred from
      the input.

      The syntax may look like this:

        apex-image -u zImage-new aImage    # Replace zImage in aImage
        apex-image -u . initrd-new aImage  # Replace initrd in aImage

      It would be wise to make sure that the distinction between an
      update the creation of a new image is clear.  Including a single
      '.' could be construed as requesting an update.  However, it
      isn't clear what to do if there are no metadata changes and the
      image has only one payload.

        apex-image zImage aImage	   # Update aImage or replace it

      We could assume that update has priority and that the only way
      to force creation is to use the -c/-f switches.  I don't like
      this because it seems like the user should be require to make
      his intention clear.  If there is no ambiguity, then the
      application can do the rigth thing.  There is no harm in
      creating an image if aImage doesn't exist.   Still, if the user
      has used this form in the past and then does so again, he may
      expect that we'll be creating a new image when, in fact, we fall
      back to updating.

      The only other way to manage this would be to report on the
      progress.  If we say 'creating' or 'updating' then we give the
      user sufficient feedback to make the process clear.  This would
      also mean that there is no need for an --update switch, only a
      --create switch.  Moreover, there is no need for --force since
      the the user would only specify --create when the output file
      already exists.

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
  // This version works and it is identical to the one in APEX.  I
  // should make sure I know that I cannot use a more efficient
  // algorithm like the one below, but we don't really need to care.

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

size_t verify_image_header (void* pv, size_t cb, bool ignore_crc_errors)
{
  if (cb < 16)
    throw Exception ("invalid header");

  if (memcmp (pv, signature, sizeof (signature)) != 0)
    throw Exception ("no image signature found");

  size_t cbHeader = ((uint8_t*) pv)[4]*16;
  if (cbHeader > cb)
    throw Exception ("header_size invalid");

  uint32_t crc = compute_crc32 (0, pv, cbHeader);
  if (crc != 0 && !ignore_crc_errors)
    throw Exception ("incorrect header CRC");

  return cbHeader;
}

struct File {

  const char* szPath;
  int fh;
  size_t cb;
  void* pv;
  struct stat stat;

  bool is_open (void) {
    return fh != -1; }
  bool is_update (void) {
    return strcasecmp (szPath, ".") == 0; }
  bool is_valid_image (void) {
    try {
      verify_image_header (pv, cb, false);
    }
    catch (...) {
      return false;
    }
    return true;
  }
  void release_this (void) {
    if (pv != NULL) {
      munmap (pv, cb);
      pv = NULL;
    }
    if (fh != -1) {
      ::close (fh); }
    fh = -1;
  }

  File () : szPath (NULL), fh (-1), cb (0), pv (NULL) {}
  File (const char* _szPath) : szPath (_szPath), fh (-1), cb (0), pv (NULL) {}
  ~File () {
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

struct Payload : public File {

  int type;
  uint32_t load_address;
  uint32_t entry_point;
  const char* description;
  bool compressed;

  bool modified (void) {
    return type
      || load_address != ~0
      || entry_point != ~0
      || description
      //      || compressed
      ; }
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

  Payload* m_payload;

  arguments () {
    bzero (this, sizeof (*this)); }

  int mode;
  bool verbose;
  bool quiet;
  bool force;
  bool ignore_crc_errors;
  uint32_t crc_calculated;

  const char* image_name;

  Payload* payload (void) {
    if (!m_payload)
      m_payload = new Payload;
    return m_payload; }
};

struct Image : public std::list<Payload*>, public File
{
  const char* description;
  time_t timeCreation;
  int architecture_id;

  Image () : description (NULL),
             timeCreation (time (NULL)),
             architecture_id (0) { }

  /** Return true if there is a change to the metadata for the image. */
  bool modified (void) {
    if (description || architecture_id)
      return true;
    return size () && front ()->modified ();
  }

  /** Pull metadata and payloads from the final payload object in
      preparation for updating an image. */
  void load (void);

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
//      printf ("image has a description %s\n", description);
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
      crc = compute_crc32 (crc, &_id, sizeof (_id));
      write (fh, &tag, 1);
      write (fh, &_id, sizeof (_id));
    }
    return crc;
  }
};

Image g_image;

/** APEX image header class used to support a metadata iterator.
 */

class Header {
  void* m_pv;
  size_t m_cb;

public:
  Header (void* pv, size_t cb) : m_pv (pv), m_cb (cb) {}

  class Iterator {
    void* m_pv;
    void* m_pvEnd;

    uint8_t _tag (void) {
      return *(uint8_t*) m_pv; }

    void lengths (size_t& cbTag, size_t& cbData) {
      uint8_t tag = this->_tag ();
      cbTag = 1;
      cbData = 0;

//      printf ("tag %x  size %x", tag, tag & 3);
      switch (tag & 3) {
      case sizeZero:		cbData = 0; break;
      case sizeOne:		cbData = 1; break;
      case sizeFour:		cbData = 4; break;
      case sizeVariable:
//        printf (" (variable)");
        cbData = ((uint8_t*)m_pv)[1];
        if (cbData > 127)
          throw Exception ("invalid field length");
        ++cbTag;
        break;
      }
//      printf ("  cbTag %d cbData %d\n", cbTag, cbData);
    }

  public:
    Iterator (void* pv, void* pvEnd) : m_pv (pv), m_pvEnd (pvEnd) {}

    uint8_t tag (void) {
      return this->_tag (); }
    void* data (void) {
      return (char*) m_pv + tag_length (); }

    operator uint32_t () {
      uint32_t v;
      memcpy (&v, data(), 4);
      return swabl (v); }

    operator uint8_t () {
      return *(uint8_t*)data (); }

    size_t field_length (void) {
      size_t cbTag;
      size_t cbData;
      lengths (cbTag, cbData);
      return cbTag + cbData; }

    size_t tag_length (void) {
      size_t cbTag;
      size_t cbData;
      lengths (cbTag, cbData);
      return cbTag; }

    size_t data_length (void) {
      size_t cbTag;
      size_t cbData;
      lengths (cbTag, cbData);
      return cbData; }

    Iterator& operator++ () {
      if (m_pv >= m_pvEnd)
        throw Exception ("attempt to increment metdata iterator beyond end");
      m_pv = (char*) m_pv + field_length (); };

    bool operator!= (const Iterator& it) {
      return m_pv != it.m_pv; }
  };

  Iterator begin (void) {
    return Iterator ((char*) m_pv + 5,        (char*) m_pv + m_cb - 4); }
  Iterator end (void) {
    return Iterator ((char*) m_pv + m_cb - 4, (char*) m_pv + m_cb - 4); }

};


struct argp_option options[] = {
  { "show",		'i', 0, 0, "Mode to show an image header", 1 },
  { "update",		'u', 0, 0, "Mode to update image" },
  { "create",		'c', 0, 0, "Mode to create a new image" },

  { "load-address",	'l', "ADDR", 0, "Set load address for image", 2 },
  { "entry-point",	'e', "ADDR", 0, "Set entry point for image" },
  { "load-and-entry",	'L', "ADDR", 0,
				"Set load address and entry point for image" },
  { "image-description", 'n', "LABEL", 0, "Set description of image" },
  { "type",		't', "TYPE", 0, "Set payload type [kernel]" },
  { "description",       'd', "LABEL", 0, "Set description of payload" },
  { "architecture-id",  'A', "NUMBER", 0,
    "Set a value to override the architecture ID" },

  { "force",		'f', 0, 0, "Force overwrite of output file", 3 },
  { "verbose",		'v', 0, 0, "Verbose output, when available", 3 },
  { "quiet",		'q', 0, 0, "Suppress output", 3 },
  { "ignore-crc-errors", 'C', 0, 0, "Ignore CRC errors", 3 },

  { 0 }
};

error_t arg_parser (int key, char* arg, struct argp_state* state);

struct argp argp = {
  options, arg_parser,
  "FILE...",
  "  Create, update, or view an APEX image file\n"
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
  "  TYPE is one of: kernel, initrd\n"
  "  FILE is either a filename, or possibly '.' when updating an image.\n"
  "\n"
  "  e.g. apex-image aImage                    # Show image metadata\n"
//  "       apex-image -u -L 0x8000 aImage       # Change the load/entry point\n"
//  "                                            # Create 2MiB kernel uImage\n"
//  "       apex-image -c -L 0x8000 -A arm -t kernel vmlinuz '*_2m' uImage\n"
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

  case 'A':
    g_image.architecture_id = interpret_number (arg);
    break;

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
    args.quiet = false;
    break;

  case 'q':
    args.quiet = true;
    args.verbose = false;
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

    if (!args.mode) {
      if (g_image.size () == 1)
        args.mode = g_image.modified ()
          ? modeUpdate : modeShow;
      else
        args.mode = g_image.back ()->is_valid_image ()
          ? modeUpdate : modeCreate;
    }

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


/**

   Probably the best way to do this is to allocate a payload at the
   start and whenever we see a payload length tag.  When we're ready
   to apply the payload, we either merge it with the payload object
   already in the image, or we insert the new payload object.  We'll
   end up with an allocated payload at the end that we don't need, but
   that's OK.

 */

void Image::load (void)
{
  Payload& image = *back ();

  /* *** FIXME: this isn't really the right thing to do.  If the final
     *** paylaod isn't an image, we should be able to use it as a
     *** source payload. */

  if (!image.is_valid_image ())
    throw Exception ("unable to load from invalid image");

  /*
    What we do is scan the header for tags, updating the image
    metadata as well as the payload metadata.  If there are more
    payloads found in the metadata than the user specified, we add
    payload objects to the image.  If some of the payload objects have
    update '.' names, we need to setup the payload object to point to
    the data in the original image.  We're going to generate a new
    image when we update instead of attempting to update in place.

   */

  size_t cbHeader = verify_image_header (image.pv, image.cb, false);
  Header header (image.pv, cbHeader);
  Payload* payload = new Payload;
  for (Header::Iterator it = header.begin (); it != header.end (); ++it) {
    switch (it.tag ()) {
    case fieldImageDescription:
      if (!description)
        description = (char*) it.data ();
      break;
    case fieldPayloadLength:
      /* *** Need to always fixup the pv and cb to reflect where this
     *** data used to come from.  The merge process will take care
     *** of updating the data if the user has asked for it. */
      printf ("Payload Length:       %s\n", interpret_size (uint32_t (it)));
      break;
    case fieldPayloadLoadAddress:
      payload->load_address = uint32_t (it);
      break;
    case fieldPayloadEntryPoint:
      payload->entry_point = uint32_t (it);
      break;
    case fieldPayloadType:
      payload->type = uint8_t (it);
      break;
    case fieldPayloadDescription:
      payload->description = (char*) it.data ();
      break;
    case fieldPayloadCompression:
      printf ("Payload Compression:  gzip\n");
      break;
    case fieldLinuxKernelArchitectureID:
      if (!architecture_id)
        architecture_id = uint32_t (it);
      break;
    default:
      break;                    // It's OK to skip unknown tags
    }
  }

  // *** Parse the file header and generate the Payloads
}


/** Show and possibly verify an image header.  The pv/cb is the memory
    to scan for the header which must be the first data in the
    region. */

void report_header (struct arguments& args, void* pv, size_t cb)
{
                                // Verify will throw on error
  size_t cbHeader = verify_image_header (pv, cb, args.ignore_crc_errors);
  uint32_t crc = compute_crc32 (0, pv, cbHeader - 4);

  // At this point, we have what we believe is a bona fide image
  // header.  We skip the signature and header_size.  The loop on
  // fields proceeds until the last byte before the CRC.

  Header header (pv, cbHeader);

  if (args.verbose) {
    printf ("Signature:            0x%02x 0x%02x 0x%02x 0x%02x\n",
            ((uint8_t*)pv)[0], ((uint8_t*)pv)[1],
            ((uint8_t*)pv)[2], ((uint8_t*)pv)[3]);
    printf ("Header Size:          %d (%d bytes)\n",
            ((uint8_t*)pv)[4], ((uint8_t*)pv)[4]*16);
  }

  for (Header::Iterator it = header.begin (); it != header.end (); ++it) {
    switch (it.tag ()) {
    case fieldImageDescription:
      printf ("Image Description:    %s\n", it.data ());
      break;
    case fieldImageCreationDate:
      {
        time_t t = (uint32_t) it;
        printf ("Image Creation Date:  %s", asctime (localtime (&t)));
      }
      break;
    case fieldPayloadLength:
      printf ("Payload Length:       %s\n", interpret_size (uint32_t (it)));
      break;
    case fieldPayloadLoadAddress:
      printf ("Payload Load Address: 0x%08lx\n", uint32_t (it));
      break;
    case fieldPayloadEntryPoint:
      printf ("Payload Entry Point:  0x%08lx\n", uint32_t (it));
      break;
    case fieldPayloadType:
      printf ("Payload Type:         %s\n",
              describe_image_type (uint8_t (it)));
      break;
    case fieldPayloadDescription:
      printf ("Payload Description:  \n", it.data ());
      break;
    case fieldPayloadCompression:
      printf ("Payload Compression:  gzip\n");
      break;
    case fieldLinuxKernelArchitectureID:
      printf ("Linux Kernel Architecture ID: %d (0x%x)\n",
              uint32_t (it), uint32_t (it));
      break;
    case fieldNOP:
//      printf ("NOP:\n");
      break;
    default:
      break;                    // It's OK to skip unknown tags
    }
  }

  if (crc != 0 || args.verbose || args.ignore_crc_errors) {
    uint32_t header_crc;
    memcpy (&header_crc, (char*) pv + cbHeader - 4, 4);
    header_crc = swabl (header_crc);
    bool ok = crc == header_crc;

    printf ("Header CRC:           0x%08x %s 0x%08x %s\n",
            header_crc, ok ? "==" : "!=", crc, ok ? "" : "ERROR");
  }
}

void apeximage (struct arguments& args)
{
	// Open files and construct pads
  for (Image::iterator it = g_image.begin ();
       it != g_image.end (); ++it) {
    if (!(*it)->szPath)
      throw Exception ("file has no path?");
    (*it)->open ();
  }

  Payload& payloadIn  = *g_image.front ();
  Payload& payloadOut = *g_image.back ();

#if 0
  printf ("fileIn %s (%d B header)\n", payloadIn.szPath,
          payloadIn.header_size ());
  printf ("fileOut %s (%d B header)\n", payloadOut.szPath,
          payloadOut.header_size ());
#endif

  switch (args.mode) {
  case modeShow:
    break;
  case modeUpdate:
    // *** FIXME: this may not be a valid test if we want to be able
    // *** to 'update' a file that has no header as is available with
    // *** the ubootimage program.
    if (!payloadOut.is_valid_image ())
      throw Exception ("unable to read file '%s'", payloadOut.szPath);
    g_image.load ();
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

  if (args.mode == modeUpdate) {
    printf ("Update not supported\n");
    return;
  }

  if (args.mode == modeCreate) {
    printf ("Creating image %s.\n", payloadOut.szPath);
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
//    printf ("base header is %d bytes\n", header_size);
    for (Image::iterator it = g_image.begin ();
         it != g_image.end (); ++it) {
      if ((*it) == &payloadOut)
        break;
      header_size += (*it)->header_size ();
    }
    header_size += 4;           // CRC
//    printf ("total header with crc is %d bytes\n", header_size);
    unsigned char _header_size = (header_size + 15)/16;
    crc = compute_crc32 (crc, &_header_size, 1);
    write (fhSave, &_header_size, 1);
//    printf ("crc of sig and size 0x%08x\n", crc);
    crc = g_image.write_header (crc, fhSave);
//    printf ("crc of sig and size and image fields 0x%08x\n", crc);
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

//      printf ("%x length  %d->%d\n",
//              swabl (crc), (*it)->cb, 16 - (((*it)->cb + 4) & 0xf));
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
