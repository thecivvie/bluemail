/*
 * blueMail offline mail reader
 * services to make mail available to driver

 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "bmail.h"
#include "service.h"
#include "../common/auxil.h"
#include "../common/error.h"
#include "../common/mysystem.h"


services::services (resource *ro)
{
  this->ro = ro;
  setPacktype(PT_UNDEF);
  *oldname = '\0';
}


void services::setPacktype (packtype pt, archtype at)
{
  packType = pt;
  archType = at;     // last (uncompress) archive type for compress routine
}


// clear todir and "unpack" archive into it
int services::unpack (const char *archive, const char *todir, bool setarchtype)
{
  int result;
  archtype previous = archType;

  clearDirectory(todir);

  switch (packType)
  {
    case PT_ARCHIVE:
      result = uncompress(archive);
      if (!setarchtype) archType = previous;
      return result;
      break;

    case PT_COPY:
      return fcopy(archive, fname(archive));
      break;

    case PT_COLLECT:
      return (uncollect(archive, 0, ".", false) ? 0 : 1);   // error(level)
      break;

    default:
      return 1;   // error(level)
  }
}


// "pack" addfile(s) to arcfile in arcdir
const char *services::pack (const char *arcdir, const char *arcfile,
                            const char *addfile)
{
  static char archive[MYMAXPATH];
  file_list *fl;

  mkfname(archive, arcdir, arcfile);

  switch (packType)
  {
    case PT_ARCHIVE:
      compress(archive, addfile);
      break;

    case PT_COPY:
      fl = new file_list(".");
      fl->gotoFile(0);
      fcopy(fl->getName(), archive);
      delete fl;
      break;

    case PT_COLLECT:
      collect(".", archive);
      break;

    default:
      ;
  }

  return archive;
}


// save all files in dir into fname
bool services::collect (const char *dir, const char *fname)
{
  FILE *dest, *src;
  int i;
  size_t len;
  bool success = false;

  file_list *fl = new file_list(dir);

  if ((dest = fopen(fname, "ab")))
  {
    int f = fl->getNoOfFiles();

    if (fprintf(dest, COLL_MAGIC "%d:", f) >= (int) strlen(COLL_MAGIC) + 2)
    {
      char *buffer = new char[BLOCKLEN];

      for (i = 0; i < f; i++)
      {
        fl->gotoFile(i);

        const char *name = fl->getName();
        int slen = strlen(name);
        off_t size = fl->getSize();

        if (fprintf(dest, "%s%c%ld:", name, 0, (long) size) < slen + 3) break;

        if ((src = fopen(name, "rb")))
        {
          while ((len = fread(buffer, sizeof(char), BLOCKLEN, src)))
            fwrite(buffer, sizeof(char), len, dest);
          fclose(src);
        }
        else break;
      }

      success = (i == f);
      delete[] buffer;
    }

    fclose(dest);
  }

  delete fl;

  return success;
}


// restore all files in fname into dir
bool services::
// commonly used routine
#include "../common/uncoll.cc"


archtype services::getArchiveType (const char *archive) const
{
  FILE *arc;
  archtype guess = ARC_UNKNOWN;
  unsigned int magic, c;

  if (!(arc = fopen(archive, "rb"))) return guess;

  magic = fgetc(arc) << 8;
  magic += fgetc(arc);

  switch (magic)
  {
    case 0x60EA:     // ARJ header id
      guess = ARC_ARJ;
      break;

    case 0x504B:     // "PK"
      // check for local header signature
      if (fgetc(arc) == 3 && fgetc(arc) == 4) guess = ARC_ZIP;
      break;

    default:
      // check for LHA method id
      if (fgetc(arc) == '-' && fgetc(arc) == 'l')
      {
        c = fgetc(arc);
        if (c == 'h' || c == 'z') guess = ARC_LHA;
      }
  }

  fclose(arc);
  return guess;
}


const char *services::getArchiveExt () const
{
  static const char *ext[] = {"", "arj", "zip", "lzh"};

  return ext[archType];
}


int services::uncompress (const char *archive)
{
  const char *cmd;

  archType = getArchiveType(archive);

  switch (archType)
  {
    case ARC_ARJ:
      cmd = ro->get(arjUncompressCommand);
      break;

    case ARC_ZIP:
      cmd = ro->get(zipUncompressCommand);
      break;

    case ARC_LHA:
      cmd = ro->get(lhaUncompressCommand);
      break;

    default:
      cmd = ro->get(unknownUncompressCommand);
  }

  return mysystem(makecmd(cmd, extent(canonize(archive))));
}


void services::compress (const char *archive, const char *addfile)
{
  const char *cmd;

  switch (archType)
  {
    case ARC_ARJ:
      cmd = ro->get(arjCompressCommand);
      break;

    case ARC_ZIP:
      cmd = ro->get(zipCompressCommand);
      break;

    case ARC_LHA:
      cmd = ro->get(lhaCompressCommand);
      break;

    default:
      cmd = ro->get(unknownCompressCommand);
  }

  char *arg = strdupplus(makecmd(extent(canonize(archive)), addfile));
  mysystem(makecmd(cmd, arg));

  delete[] arg;
}


void services::readInfFile (const char *name)
{
  if (!isSystemInfFile(name)) memset(&sysinf, 0, sizeof(sysinf));
}


bool services::isSystemInfFile (const char *name)
{
  FILE *file;
  bool success = false;

  size_t slen = strlen(name);

  if (slen >= 5 && strcasecmp(name + slen - 4 , ".inf") == 0)
  {
    mkfname(fname, ro->get(InfDir), name);

    if (strcmp(oldname, fname) != 0)
    {
      strcpy(oldname, fname);
      memset(&sysinf, 0, sizeof(sysinf));

      if ((file = fopen(fname, "rb")))
      {
        fread(&sysinf, sizeof(SYSTEM_INF_HEADER), 1, file);
        fclose(file);
      }
    }

    success = (strncmp(sysinf.magic, SYSINF_MAGIC, sizeof(sysinf.magic)) == 0);
  }

  return success;
}


time_t services::getSystemInfDate (const char *name)
{
  return (isSystemInfFile(name) ? fmtime(fname) : 0);
}


unsigned int services::getSystemInfHeaderSize (const char *name)
{
  readInfFile(name);
  return (unsigned int) sysinf.size << 7;
}


const char *services::getSystemName (const char *name)
{
  readInfFile(name);
  return sysinf.systemname;
}


const char *services::getReplyExtension (const char *name)
{
  readInfFile(name);
  return sysinf.extension;
}


const char *services::getReplyName (const char *name)
{
  readInfFile(name);
  return sysinf.replyname;
}


unsigned char services::getPacktype (const char *name)
{
  readInfFile(name);
  return sysinf.packtype;
}


unsigned char services::getArchtype (const char *name)
{
  readInfFile(name);
  return sysinf.archtype;
}


void services::createSystemInfFile (time_t mtime, const char *id,
                                                  const char *replyExt,
                                                  const char *replyName)
{
  FILE *file;

  if (mychdir(ro->get(InfWorkDir)) != 0)
    fatalError("Unable to access work subdir.");

  mkfname(fname, ro->get(InfDir), ext(id, "inf"));

  if (access(fname, R_OK | W_OK) != 0 || fmtime(fname) < mtime)
  {
    remove(fname);

    memset(&sysinf, 0, sizeof(sysinf));
    *oldname = '\0';

    strncpy(sysinf.magic, SYSINF_MAGIC, sizeof(sysinf.magic));
    sysinf.size = sizeof(SYSTEM_INF_HEADER) >> 7;
    sysinf.packtype = (unsigned char) packType;
    sysinf.archtype = (unsigned char) archType;
    strncpy(sysinf.extension, replyExt, sizeof(sysinf.extension) - 1);
    strncpy(sysinf.systemname, ro->get(BBSName), sizeof(sysinf.systemname) - 1);
    if (replyName)
      strncpy(sysinf.replyname, stem(replyName), sizeof(sysinf.replyname) - 1);

    if ((file = fopen(fname, "wb")))
    {
      if (fwrite(&sysinf, sizeof(sysinf), 1, file) == 1) strcpy(oldname, fname);
      fclose(file);
    }

    collect(".", fname);

    if (mtime) fmtime(fname, mtime);
  }

  clearDirectory(".");
}


void services::killSystemInfFile (const char *name)
{
  if (isSystemInfFile(name))
  {
    remove(fname);
    *oldname = '\0';
  }
}
