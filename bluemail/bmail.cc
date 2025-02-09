/*
 * blueMail offline mail reader
 * bmail class

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>,
                    Robert Vukovic <vrobert@uns.ns.ac.yu>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "bmail.h"
#include "../common/auxil.h"
#include "../common/mysystem.h"


bmail::bmail ()
{
  *select_error = '\0';
  serviceType = ST_UNDEF;
  lastFileName = NULL;
  letterList = NULL;

  resourceObject = new resource();
  service = new services(resourceObject);

  if (resourceObject->isYes(OmitSystem))
    sprintf(bm_version, BM_TOP_N " " BM_TOP_V, BM_NAME, BM_MAJOR, BM_MINOR);
  else
    sprintf(bm_version, BM_TOP_N "/%.20s " BM_TOP_V, BM_NAME, sysname(),
                                                     BM_MAJOR, BM_MINOR);
}


bmail::~bmail ()
{
  delete service;
  delete resourceObject;
}


bool bmail::loadDriver (const char *dir)
{
  *select_error = '\0';

  fileList = new file_list(dir);

  if (fileList->getNoOfFiles() == 0)
  {
    if (serviceType == ST_PACKET)
      strcpy(select_error, "Could not uncompress packet");
    delete fileList;
    return false;
  }

  driverList = new driver_list(this);
  const char *driverInitError = driverList->init(this);

  if (driverInitError)
  {
    strncpy(select_error, driverInitError, sizeof(select_error) - 1);
    select_error[sizeof(select_error) - 1] = '\0';
  }

  if (!driverList->getNoOfDrivers() || driverInitError)
  {
    delete driverList;
    delete fileList;
    return false;
  }

  areaList = new area_list(this);

  if (driverList->canReply()) driverList->getReplyDriver()->getConfig();

  mychdir(fileList->getDir());
  return true;
}


void bmail::Delete ()
{
  serviceType = ST_UNDEF;
  service->setPacktype(PT_UNDEF);
  resourceObject->set(PacketName, NULL);
  resourceObject->set(InfName, NULL);

  delete[] lastFileName;
  lastFileName = NULL;

  delete areaList;
  delete driverList;
  delete fileList;
}


bool bmail::selectPacket (const char *packetName)
{
  char fname[MYMAXPATH];
  const char *pktworkdir = resourceObject->get(PacketWorkDir);

  serviceType = ST_PACKET;
  service->setPacktype(PT_ARCHIVE);
  resourceObject->set(PacketName, packetName);
  mkfname(fname, resourceObject->get(PacketDir), packetName);
  lastFileID = crc32(packetName);
  lastFileMtime = fmtime(fname);

  if (service->unpack(fname, pktworkdir) != 0)
  {
    strcpy(select_error, "Could not uncompress packet");
    return false;
  }

  if (loadDriver(pktworkdir)) return true;
  else
  {
    if (!*select_error) strcpy(select_error, "Packet type not recognized");
    return false;
  }
}


bool bmail::selectFileDB (const char *path, bool isFile)
{
  const char *dir = (isFile ? fdir(path) : path);

  serviceType = ST_FILEDB;
  service->setPacktype(PT_COLLECT);

  if (resourceObject->get(mboxFile))
    lastFileName = strdupplus(resourceObject->get(mboxFile));

  if (loadDriver(dir)) return true;
  else
  {
    if (!*select_error) strcpy(select_error, "Could not open file/database");
    return false;
  }
}


bool bmail::selectMbox (const char *file)
{
  char fname[MYMAXPATH];

  serviceType = ST_FILEDB;
  service->setPacktype(PT_COLLECT);

  mkfname(fname, resourceObject->get(MboxesDir), file);
  lastFileName = strdupplus(fname);
  lastFileID = crc32(file);
  lastFileMtime = fmtime(fname);

  if (loadDriver(resourceObject->get(MboxesDir))) return true;
  else
  {
    if (!*select_error) strcpy(select_error, "Could not open file");
    return false;
  }
}


bool bmail::selectReply (const char *infFile)
{
  char fname[MYMAXPATH];
  const char *pktworkdir = resourceObject->get(PacketWorkDir);

  serviceType = ST_REPLY;
  service->setPacktype((packtype) service->getPacktype(infFile),
                       (archtype) service->getArchtype(infFile));
  resourceObject->set(InfName, infFile);
  mkfname(fname, resourceObject->get(InfDir), infFile);

  if (!service->uncollect(fname, service->getSystemInfHeaderSize(infFile),
                          pktworkdir, false))
  {
    strcpy(select_error, "Could not open reply information");
    return false;
  }

  time_t mtime = service->getSystemInfDate(infFile);
  file_list *fl = new file_list(".");

  for (int i = 0; i < fl->getNoOfFiles(); i++)
  {
    fl->gotoFile(i);
    fmtime(fl->getName(), mtime);
  }

  delete fl;

  lastFileName = strdupplus(getLastFileName(false));

  if (loadDriver(pktworkdir)) return true;
  else
  {
    if (!*select_error) strcpy(select_error, "Reply information is corrupt");
    return false;
  }
}


bool bmail::selectDemo ()
{
  const char *pktworkdir = resourceObject->get(PacketWorkDir);

  serviceType = ST_DEMO;
  service->setPacktype(PT_COPY);
  clearDirectory(pktworkdir);
  fcreate(pktworkdir, "demo.pkt");

  if (loadDriver(pktworkdir)) return true;
  else
  {
    if (!*select_error) strcpy(select_error, "Error in service");
    return false;
  }
}


servicetype bmail::getServiceType () const
{
  return serviceType;
}


lrtype bmail::saveLastread ()
{
  const char *lrpfile =
    driverList->getReadObject(driverList->getMainDriver())->saveMSF();

  if (lrpfile && *lrpfile && (*lrpfile != '!'))
  {
    const char *packet = service->pack(resourceObject->get(PacketDir),
                                       resourceObject->get(PacketName),
                                       lrpfile);
    // preserve packet's mtime
    fmtime(packet, lastFileMtime);
  }

  return (lrpfile ? (*lrpfile == '!' ? LR_NORETRY : LR_OK) : LR_ERROR);
}


uint32_t bmail::getLastFileID () const
{
  return lastFileID;
}


time_t bmail::getLastFileMtime () const
{
  return lastFileMtime;
}


const char *bmail::getLastFileName (bool real) const
{
  return (real ? lastFileName : "$$mbox$$");
}


const char *bmail::version () const
{
  return bm_version;
}


bool bmail::isLatin1 () const
{
  return driverList->getMainDriver()->isLatin1();
}


const char **bmail::getBulletins () const
{
  return driverList->getMainDriver()->getBulletins();
}


const char *bmail::getSelectError () const
{
  return select_error;
}


const char *bmail::getExtraArea (int &count_or_state)
{
  if (driverList->canReply() && driverList->hasExtraAreas())
    return driverList->getReplyDriver()->getExtraArea(count_or_state);
  else
  {
    count_or_state = 0;
    return NULL;
  }
}


void bmail::setExtraArea (int extra, bool subscribe, const char *title)
{
  if (driverList->canReply() && driverList->hasExtraAreas())
    driverList->getReplyDriver()->setExtraArea(extra, subscribe, title);
}


const char *bmail::getAreaMarksFile (const char *name)
{
  return ext(name ? name : driverList->getMainDriver()->getPacketID(), "mrk");
}


bool bmail::saveAreaMarks ()
{
  bool success = true;
  char fname[MYMAXPATH], sname[MYMAXPATH];
  FILE *f;

  mkfname(fname, resourceObject->get(InfDir), getAreaMarksFile());

  if (areaList->isAnyMarked())
  {
    // rename old file, so don't trash old marks if saving fails
    strcpy(sname, fname);
    sname[strlen(sname) - 1] = '$';
    if (access(fname, R_OK | W_OK) == 0 && rename(fname, sname) == -1)
      return false;

    success = ((f = fopen(fname, "wt")) != NULL);

    for (int a = 0; success && (a < areaList->getNoOfAreas()); a++)
    {
      areaList->gotoArea(a);

      if (areaList->isMarked())
      {
        const char *title = areaList->getTitle();

        if (fprintf(f, "%s\n", title) != (int) strlen(title) + 1)
          success = false;
      }
    }

    if (f) fclose(f);

    if (success) remove(sname);
    else rename(sname, fname);
  }
  else remove(fname);   // may exist, or not

  return success;
}
