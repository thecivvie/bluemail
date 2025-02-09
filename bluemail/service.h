/*
 * blueMail offline mail reader
 * services to make mail available to driver

 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef SERVICE_H
#define SERVICE_H


#include <time.h>
#include "resource.h"
#include "../common/mysystem.h"


// service types
enum servicetype {ST_UNDEF, ST_PACKET, ST_FILEDB, ST_REPLY, ST_DEMO};

// pack types (add new entries only at the end!)
enum packtype {PT_UNDEF, PT_ARCHIVE, PT_COPY, PT_COLLECT};

// archive types (add new entries only at the end and default extension in
//                services::getArchiveExt()!)
enum archtype {ARC_UNKNOWN, ARC_ARJ, ARC_ZIP, ARC_LHA};

// service error codes
enum srvcerror {SRVC_DONE, SRVC_OK, SRVC_NO_PACKET, SRVC_NO_FILEDB,
                SRVC_NO_REPLY_INF, SRVC_ERROR};


// header of system information file for reply packet manager
typedef struct
{
  char magic[5];               // identifier
  unsigned char size;          // of header in blocks of 128 bytes
  unsigned char packtype;      // to use for replies
  unsigned char archtype;      // to use for replies
  char extension[4];           // of the reply packet
  char systemname[65];         // BBS name
  char replyname[9];           // (only if different from system inf file)
  char reserved[170];          // for future use
}
SYSTEM_INF_HEADER;

#define SYSINF_MAGIC "bminf"

// The body of a system information file for the reply packet manager
// consists of a "collection" of files. A collection of files starts
// with a header:
//
// "COLL"                       identifier
// char no_of_files[]           in ASCII, variable length
// char = ':'                   terminator
//
// followed by file data (repeated as often as given by no_of_files):
//
// char file_name[]             variable length
// char = 0                     filename terminator
// char file_len[]              in ASCII, variable length
// char = ':'                   terminator
// char file_content[]          variable length, as given by file_len

#define COLL_MAGIC "COLL"


class services
{
  private:
    resource *ro;
    packtype packType;
    archtype archType;
    char oldname[MYMAXPATH], fname[MYMAXPATH];
    SYSTEM_INF_HEADER sysinf;

    archtype getArchiveType(const char *) const;
    int uncompress(const char *);
    void compress(const char *, const char *);
    bool collect(const char *, const char *);
    void readInfFile(const char *);

  public:
    services(resource *);
    void setPacktype(packtype, archtype = ARC_UNKNOWN);
    int unpack(const char *, const char *, bool = true);
    const char *pack(const char *, const char *, const char *);
    bool uncollect(const char *, long, const char *, bool);
    const char *getArchiveExt() const;
    bool isSystemInfFile(const char *);
    time_t getSystemInfDate(const char *);
    unsigned int getSystemInfHeaderSize(const char *);
    const char *getSystemName(const char *);
    const char *getReplyExtension(const char *);
    const char *getReplyName(const char *);
    unsigned char getPacktype(const char *);
    unsigned char getArchtype(const char *);
    void createSystemInfFile(time_t, const char *, const char *,
                             const char * = NULL);
    void killSystemInfFile(const char *);
};


#endif
