/*
 * blueMail offline mail reader
 * Blue Wave packet driver

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "bwave.h"
#include "../../common/auxil.h"
#include "../../common/error.h"
#include "../../common/mysystem.h"


/*
   Blue Wave main driver
*/

bwave::bwave (bmail *bm)
{
  this->bm = bm;
  tool = new basic_tool(bm, this);

  areas = NULL;
  persIDX = NULL;
  persmsgs = 0;

  persArea = bm->resourceObject->isYes(PersonalArea);
  if (bm->getServiceType() == ST_REPLY) persArea = false;

  body = NULL;
  mixID = NULL;
  mixRecord = NULL;
  ftiFile = datFile = NULL;
}


bwave::~bwave ()
{
  while (maxAreas) delete[] body[--maxAreas];

  delete[] mixRecord;
  delete[] mixID;
  delete[] bulletins;
  delete[] letterBody;
  delete[] body;
  delete[] persIDX;
  delete[] areas;
  delete tool;

  if (datFile) fclose(datFile);
  if (ftiFile) fclose(ftiFile);
}


const char *bwave::init ()
{
  setPacketID();

  if (!readINF()) return error;

  body = new bodytype *[maxAreas];
  for (int i = 0; i < maxAreas; i++) body[i] = NULL;

  if (!(ftiFile = openFile("fti"))) return error;
  if (!readMIX()) return error;

  if (!(datFile = openFile("dat"))) return error;

  return NULL;
}


area_header *bwave::getNextArea ()
{
  area_header *ah;
  int index, totmsgs, numpers, bw_flags, bm_flags;

  bool isPersArea = (persArea && (NAID == 0));

  index = mixID[NAID];

  if(isPersArea) totmsgs = numpers = persmsgs;
  else if (index != -1)
  {
    totmsgs = getLSBshort(mixRecord[index].totmsgs);
    numpers = getLSBshort(mixRecord[index].numpers);
  }
  else totmsgs = numpers = 0;

  body[NAID] = new bodytype[totmsgs];

  bw_flags = getLSBshort(areas[NAID].area_flags);

  if (isPersArea) bm_flags = A_COLLECTION | A_READONLY | A_LIST;
  else bm_flags = (bw_flags & INF_SCANNING ? A_SUBSCRIBED : 0) |
                  (bw_flags & INF_NETMAIL ? A_NETMAIL : 0) |
                  (areas[NAID].network_type == INF_NET_INTERNET ? A_INTERNET |
                                                                  A_CHRSLATIN1
                                                                : 0) |
                  (bw_flags & INF_POST ? 0 : A_READONLY) |
                  (bw_flags & INF_ALIAS_NAME ? A_ALIAS : 0);

  ah = new area_header(bm, NAID + bm->driverList->getOffset(this),
                       (isPersArea ? "PERS" : (char *) areas[NAID].areanum),
                       (isPersArea ? "PERSONAL" : (char *) areas[NAID].echotag),
                       (isPersArea ? "Letters addressed to you"
                                   : (char *) areas[NAID].title),
                       (isPersArea ? "Blue Wave Personal" : "Blue Wave"),
                       bm_flags, totmsgs, numpers, maxfromto, maxsubject);
  NAID++;

  return ah;
}


int bwave::getNoOfLetters ()
{
  int index = mixID[currentArea];

  if (persArea && (currentArea == 0)) return persmsgs;
  else return (index != -1 ? getLSBshort(mixRecord[index].totmsgs) : 0);
}


letter_header *bwave::getNextLetter ()
{
  int area, letter;
  static FTI_REC fti;
  net_address na;
  unsigned int zone;

  if (persArea && (currentArea == 0))
  {
    area = persIDX[NLID].area;
    letter = persIDX[NLID].msgnum;
  }
  else
  {
    area = currentArea;
    letter = NLID;
  }

  fseek(ftiFile, getLSBlong(mixRecord[mixID[area]].msghptr) +
                 letter * ftiStructLen, SEEK_SET);

  if (fread(&fti, ftiStructLen, 1, ftiFile) != 1)
    fatalError("Could not get next letter.");

  body[area][letter].pointer = getLSBlong(fti.msgptr);
  body[area][letter].msglen = getLSBlong(fti.msglength);

  if ((zone = getLSBshort(fti.orig_zone)))
    na.set(zone, getLSBshort(fti.orig_net), getLSBshort(fti.orig_node), 0);
  // point only available through message (body)

  // due to Y2K problems of the doors the date field may not be 0-terminated
  static char date[21];
  strncpy(date, (char *) fti.date, 20);
  date[20] = '\0';

  NLID++;
  return new letter_header(bm, letter,
                           (char *) fti.from,
                           (char *) fti.to,
                           (char *) fti.subject,
                           date,
                           getLSBshort(fti.msgnum), getLSBshort(fti.replyto),
                           area,
                           getLSBshort(fti.flags),
                           (areas[area].network_type == INF_NET_INTERNET),
                           &na, NULL, NULL, NULL, this);
}


const char *bwave::getBody (int area, int letter, letter_header *lh)
{
  int kar;
  bool noSoftCR = bm->resourceObject->isYes(StripSoftCR);

  delete[] letterBody;
  letterBody = new char[body[area][letter].msglen + 1];
  unsigned char *p = (unsigned char *) letterBody;
  fseek(datFile, body[area][letter].pointer, SEEK_SET);

  for (long c = 0; c < body[area][letter].msglen; c++)
  {
    kar = fgetc(datFile);

    // each valid message starts with a space character
    if (c == 0 && kar == ' ')
      if (++c < body[area][letter].msglen) kar = fgetc(datFile);

    // a message shouldn't (must not) contain a 0x00 character
    if (kar == 0) kar = ' ';

    // strip soft CR
    if (noSoftCR && (kar == SOFT_CR)) continue;

    // end of line?
    if (kar == '\r') *p++ = '\n';
    else if (kar != '\n') *p++ = kar;
  }

  do p--;
  // strip blank lines
  while (p >= (unsigned char *) letterBody && (*p == ' ' || *p == '\n'));

  *++p = '\0';

  // determine charset
  bool latin1 = lh->isLatin1();
  tool->fsc0054(letterBody, '\1', &latin1);
  lh->setLatin1(latin1);

  // get netmail address from echomail
  net_address &na = lh->getNetAddr();
  char *addr = tool->getFidoAddr(letterBody);
  if (addr) na.set(addr);

  // add point to netmail address
  unsigned int point;
  char *fmpt = strstr(letterBody, "\1FMPT ");
  if (fmpt && sscanf(fmpt, "\1FMPT %u\n", &point))
    na.set(na.getZone(), na.getNet(), na.getNode(), point);

  // decode quoted-printable
  if (bm->driverList->allowQuotedPrintable() && tool->isQP(letterBody))
    tool->mimeDecodeBody(letterBody);

  // decode UTF-8
  if (tool->isUTF8(letterBody)) tool->utf8DecodeBody(letterBody);

  return letterBody;
}


/* Blue Wave expects proper information in the XTI file if it exists.
   Unfortunately, it uses this information to show the number of
   personal messages instead of using the MIX file. */
void bwave::initMSF (int **msgstat)
{
  tool->MSFinit(msgstat, persOffset(), persIDX, persmsgs);
}


int bwave::readMSF (int **msgstat)
{
  tool->XTIread(msgstat, persOffset());
  return (MSF_READ | MSF_REPLIED | MSF_PERSONAL | MSF_MARKED);
}


const char *bwave::saveMSF (int **msgstat)
{
  return tool->XTIsave(msgstat, persOffset(), packetID);
}


inline const char *bwave::getPacketID () const
{
  return packetID;
}


inline bool bwave::isLatin1 ()
{
  return false;
}


inline bool bwave::offlineConfig ()
{
  return (ctrl_flags & INF_NO_CONFIG) == 0;
}


void bwave::setPacketID ()
{
  const char *fname = bm->fileList->exists(".inf");   // which DOES exist,
  // otherwise we wouldn't be here

  int len = strlen(fname) - 4;
  strncpy(packetID, fname, len);
  packetID[len] = '\0';
}


inline unsigned int bwave::getNet () const
{
  return net;
}


inline unsigned int bwave::getNode () const
{
  return node;
}


inline INF_HEADER &bwave::getInfHeader ()
{
  return infHeader;
}


FILE *bwave::openFile (const char *extent)
{
  FILE *file;
  char buf[13];

  sprintf(buf, "%s.%s", packetID, extent);

  if (!(file = bm->fileList->ftryopen(buf, "rb")))
    sprintf(error, "Could not open %s file.", extent);

  return file;
}


// open the .INF file, fill the infHeader structure, set
// the record lengths and read misc data from the header
bool bwave::readINF ()
{
  FILE *infFile;

  if (!(infFile = openFile("inf"))) return false;

  if (fread(&infHeader, sizeof(INF_HEADER), 1, infFile) != 1)
  {
    strcpy(error, "Could not read inf file.");
    fclose(infFile);
    return false;
  }

  // test and verify the validity of the structure lengths

  infHeaderLen = getLSBshort(infHeader.inf_header_len);
  if (infHeaderLen < ORIGINAL_INF_HEADER_LEN)
    infHeaderLen = ORIGINAL_INF_HEADER_LEN;

  infAreainfoLen = getLSBshort(infHeader.inf_areainfo_len);
  if (infAreainfoLen < ORIGINAL_INF_AREA_LEN)
    infAreainfoLen = ORIGINAL_INF_AREA_LEN;

  mixStructLen = getLSBshort(infHeader.mix_structlen);
  if (mixStructLen < ORIGINAL_MIX_STRUCT_LEN)
    mixStructLen = ORIGINAL_MIX_STRUCT_LEN;

  ftiStructLen = getLSBshort(infHeader.fti_structlen);
  if (ftiStructLen < ORIGINAL_FTI_STRUCT_LEN)
    ftiStructLen = ORIGINAL_FTI_STRUCT_LEN;

  // read some data

  bm->resourceObject->set(LoginName, (char *) infHeader.loginname);
  bm->resourceObject->set(AliasName, (char *) infHeader.aliasname);
  bm->resourceObject->set(SysOpName, (char *) infHeader.sysop);
  bm->resourceObject->set(BBSName, (char *) infHeader.systemname);
  bm->resourceObject->set(hasLoginName, "Y");

  net = getLSBshort(infHeader.net);
  node = getLSBshort(infHeader.node);

  //usesUPL = (infHeader.uses_upl_file != 0 || infHeader.ver >= 3);

  ctrl_flags = getLSBshort(infHeader.ctrl_flags);

  maxfromto = infHeader.from_to_len;
  if (maxfromto == 0 || maxfromto > 35) maxfromto = 35;

  maxsubject = infHeader.subject_len;
  if (maxsubject == 0 || maxsubject > 71) maxsubject = 71;

  if (*infHeader.packet_id) strcpy(packetID, (char *) infHeader.packet_id);

  // read area data

  maxAreas = (bm->fileList->getSize() - infHeaderLen) / infAreainfoLen +
             persOffset();

  areas = new INF_AREA_INFO[maxAreas];
  mixID = new int[maxAreas];

  for (int a = 0; a < maxAreas; a++)
  {
    int offset = a - persOffset();

    if (offset >= 0)
    {
      fseek(infFile, (long) infHeaderLen + offset * infAreainfoLen, SEEK_SET);

      if (fread(&areas[a], infAreainfoLen, 1, infFile) != 1)
      {
        strcpy(error, "Could not read area information.");
        fclose(infFile);
        maxAreas = 0;
        return false;
      }
    }

    mixID[a] = -1;
  }

  bulletins = tool->bulletin_list((const char (*)[13]) infHeader.readerfiles,
                                                       5, BL_NEWFILES);

  fclose(infFile);
  return true;
}


bool bwave::readMIX ()
{
  FILE *mixFile;
  static FTI_REC fti;

  if (!(mixFile = openFile("mix"))) return false;

  noOfMixRecs = (int) (bm->fileList->getSize() / mixStructLen);
  mixRecord = new MIX_REC[noOfMixRecs];

  if ((int) fread(mixRecord, mixStructLen, noOfMixRecs, mixFile) != noOfMixRecs)
  {
    strcpy(error, "Could not read mix file.");
    fclose(mixFile);
    return false;
  }

  fclose(mixFile);

  for (int r = 0; r < noOfMixRecs; r++)
    for (int a = persOffset(); a < maxAreas; a++)
      if (strncasecmp((char *) mixRecord[r].areanum,
                      (char *) areas[a].areanum, 6) == 0)
      {
        mixID[a] = r;
        persmsgs += getLSBshort(mixRecord[r].numpers);   // numpers of MIX_REC
        break;                                           // may be wrong...
      }

  // build personal index
  persIDX = new PERSIDX[persmsgs];
  int pm = 0;                                            // ...count it anew

  const char *username = bm->resourceObject->get(LoginName);
  const char *aliasname = bm->resourceObject->get(AliasName);

  for (int a = persOffset(); a < maxAreas; a++)
    if (mixID[a] != -1)
    {
      fseek(ftiFile, getLSBlong(mixRecord[mixID[a]].msghptr), SEEK_SET);

      int p = 0;
      for (unsigned int l = 0; l < getLSBshort(mixRecord[mixID[a]].totmsgs); l++)
      {
        if (fread(&fti, ftiStructLen, 1, ftiFile) != 1)
        {
          strcpy(error, "Could not read fti file.");
          return false;
        }

        char *to = cropesp((char *) fti.to);

        if (strcasecmp(username, to) == 0 || strcasecmp(aliasname, to) == 0)
        {
          p++;
          if (pm >= persmsgs)   // well, numpers of MIX_REC *was* wrong...
          {
            persmsgs++;
            PERSIDX *tmpIDX = new PERSIDX[persmsgs];   // ...hence resize
            for (int i = 0; i < persmsgs - 1; i++) tmpIDX[i] = persIDX[i];
            delete[] persIDX;
            persIDX = tmpIDX;
          }
          persIDX[pm].area = a;
          persIDX[pm].msgnum = l;
          pm++;
        }
      }

      putLSBshort(mixRecord[mixID[a]].numpers, p);
    }

  return true;
}


/*
   Blue Wave reply driver
*/

bwave_reply::bwave_reply (bmail *bm, main_driver *mainDriver)
{
  this->bm = bm;
  this->mainDriver = (bwave *) mainDriver;
  firstReply = currentReply = NULL;
}


bwave_reply::~bwave_reply ()
{
  REPLY *next, *r = firstReply;

  while (r)
  {
    next = r->next;
    if (r->filename) remove(r->filename);
    delete[] r->filename;
    delete r;
    r = next;
  }

  delete[] replyBody;
}


const char *bwave_reply::init ()
{
  sprintf(replyPacketName, "%.8s.new", mainDriver->getPacketID());

  if (bm->getServiceType() != ST_REPLY) replyInf();

  return (getReplies() ? NULL : error);
}


inline area_header *bwave_reply::getNextArea ()
{
  return getReplyArea("Blue Wave Replies");
}


letter_header *bwave_reply::getNextLetter ()
{
  char date[20];
  net_address na;

  mainDriver->tool->fidodate(date, sizeof(date),
                             (time_t) getLSBlong(currentReply->upl.unix_date));

  int replyto = getLSBlong(currentReply->upl.replyto);

  int area = getAreaIndex((char *) currentReply->upl.echotag);

  int msg_attr = getLSBshort(currentReply->upl.msg_attr);

  int flags = getLSBshort(currentReply->upl.netmail_attr) |
              (msg_attr & UPL_PRIVATE ? PRIVATE : 0);

  na.set(getLSBshort(currentReply->upl.destzone),
         getLSBshort(currentReply->upl.destnet),
         getLSBshort(currentReply->upl.destnode),
         getLSBshort(currentReply->upl.destpoint));

  char *net_dest = (char *) currentReply->upl.net_dest;
  if (*net_dest) na.set(net_dest);

  letter_header *newLetter = new letter_header(bm, NLID,
                                               (char *) currentReply->upl.from,
                                               (char *) currentReply->upl.to,
                                               (char *) currentReply->upl.subj,
                                               date, NLID, replyto, area,
                                               flags,
                                               (currentReply->upl.network_type == UPL_NET_INTERNET),
                                               (msg_attr & UPL_NETMAIL ? &na : NULL),
                                               NULL, NULL, NULL, this);

  NLID++;
  currentReply = currentReply->next;

  return newLetter;
}


const char *bwave_reply::getBody (int area, int letter, letter_header *lh)
{
  FILE *replyFile;

  (void) area;
  delete[] replyBody;

  REPLY *r = firstReply;
  for (int l = 1; l < letter; l++) r = r->next;

  if ((replyFile = fopen(r->filename, "rt")))
  {
    long msglen = r->length;
    replyBody = new char[msglen + 1];
    msglen = fread(replyBody, sizeof(char), msglen, replyFile);
    if (msglen && (replyBody[msglen - 1] == '\n')) msglen--;
    replyBody[msglen] = '\0';

    // hopefully, nobody will notice or complain
    bool latin1 = lh->isLatin1();
    mainDriver->tool->fsc0054(replyBody, '\2', &latin1);
    lh->setLatin1(latin1);

    fclose(replyFile);
  }
  else replyBody = strdupplus("");

  return replyBody;
}


void bwave_reply::resetLetters ()
{
  NLID = 1;
  currentReply = firstReply;
}


inline bool bwave_reply::isLatin1 ()
{
  return mainDriver->isLatin1();
}


void bwave_reply::enterLetter (letter_header *newLetter,
                               const char *newLetterFileName, long msglen)
{
  int msg_attr = 0, netmail_attr = 0;

  REPLY *newReply = new REPLY;
  memset(newReply, 0, sizeof(REPLY));

  strncpy((char *) newReply->upl.from, newLetter->getFrom(), 35);
  strncpy((char *) newReply->upl.to, newLetter->getTo(), 35);
  strncpy((char *) newReply->upl.subj, newLetter->getSubject(), 71);

  putLSBlong(newReply->upl.unix_date, (unsigned long) time(NULL));

  unsigned long replyto = (unsigned long) newLetter->getReplyTo();
  putLSBlong(newReply->upl.replyto, replyto);
  if (replyto) msg_attr |= UPL_IS_REPLY;

  strcpy((char *) newReply->upl.echotag, bm->areaList->getShortName());

  if (bm->areaList->isNetmail())
  {
    msg_attr |= UPL_NETMAIL;
    net_address na = newLetter->getNetAddr();

    if (bm->areaList->isInternet())
      strncpy((char *) newReply->upl.net_dest, na.get(true), 99);
    else
    {
      putLSBshort(newReply->upl.destzone, na.getZone());
      putLSBshort(newReply->upl.destnet, na.getNet());
      putLSBshort(newReply->upl.destnode, na.getNode());
      putLSBshort(newReply->upl.destpoint, na.getPoint());
      netmail_attr = newLetter->getFlags() & (CRASH | FILEATTACHED | KILL |
                                              LOCAL | HOLD | IMMEDIATE |
                                              FILEREQUEST | DIRECT |
                                              FILEUPDATEREQ);
    }
  }

  if (newLetter->isPrivate()) msg_attr |= UPL_PRIVATE;

  putLSBshort(newReply->upl.msg_attr, msg_attr);
  putLSBshort(newReply->upl.netmail_attr, netmail_attr);

  newReply->upl.network_type = (bm->areaList->isInternet() ? INF_NET_INTERNET
                                                           : INF_NET_FIDONET);

  newReply->filename = strdupplus(newLetterFileName);
  newReply->length = msglen;
  newReply->next = NULL;

  if (!firstReply) firstReply = newReply;

  if (noOfLetters)
  {
    REPLY *r = firstReply;
    // find last reply
    for (int l = 1; l < noOfLetters; l++) r = r->next;
    r->next = newReply;
  }

  noOfLetters++;
}


void bwave_reply::killLetter (int letter)
{
  REPLY *killReply, *r;

  if (letter == 1)
  {
    killReply = firstReply;
    firstReply = firstReply->next;
  }
  else
  {
    r = firstReply;
    for (int l = 1; l < letter - 1; l++) r = r->next;
    killReply = r->next;
    r->next = (letter == noOfLetters ? NULL : r->next->next);
  }

  remove(killReply->filename);
  delete[] killReply->filename;
  delete killReply;

  noOfLetters--;
  resetLetters();
}


const char *bwave_reply::newLetterBody (int letter, long size)
{
  REPLY *r = firstReply;
  for (int l = 1; l < letter; l++) r = r->next;

  if (size >= 0) r->length = size;
  return r->filename;
}


int bwave_reply::getAreaIndex (const char *shortname) const
{
  int area = 0;

  for (int a = 0; a < bm->areaList->getNoOfAreas(); a++)
    if (strcasecmp(bm->areaList->getShortName(a), shortname) == 0)
    {
      area = a;
      break;
    }

  return area;
}


void bwave_reply::replyInf ()
{
  char file[13], fname[MYMAXPATH];
  const char *pid = mainDriver->getPacketID();
  const char *dir = bm->resourceObject->get(InfWorkDir);
  const char *inf = bm->fileList->exists(".inf");

  sprintf(file, "%.8s.inf", pid);
  mkfname(fname, dir, file);
  fcopy(inf, fname);

  sprintf(file, "%.8s.mix", pid);
  fcreate(dir, file);

  sprintf(file, "%.8s.fti", pid);
  fcreate(dir, file);

  sprintf(file, "%.8s.dat", pid);
  fcreate(dir, file);

  bm->service->createSystemInfFile(fmtime(inf), pid,
                                   strrchr(replyPacketName, '.') + 1);
}


bool bwave_reply::putReplies ()
{
  char uplName[13], upiName[13], netName[13];
  FILE *upl = NULL, *upi = NULL, *net = NULL;
  bool success = true;

  const char *pid = mainDriver->getPacketID();

  sprintf(uplName, "%.8s.upl", pid);
  sprintf(upiName, "%.8s.upi", pid);
  sprintf(netName, "%.8s.net", pid);

  // UPL header

  UPL_HEADER upl_header;
  memset(&upl_header, 0, sizeof(upl_header));

  sprintf((char *) upl_header.vernum, BM_TOP_V, BM_MAJOR, BM_MINOR);
  for (tBYTE *p = upl_header.vernum; *p; p++) *p -= 10;

  upl_header.reader_major = BM_MAJOR;
  upl_header.reader_minor = BM_MINOR;
  strcpy((char *) upl_header.reader_name, BM_NAME " " BM_IS_WHAT);

  putLSBshort(upl_header.upl_header_len, sizeof(UPL_HEADER));
  putLSBshort(upl_header.upl_rec_len, sizeof(UPL_REC));

  strcpy((char *) upl_header.loginname, bm->resourceObject->get(LoginName));
  strcpy((char *) upl_header.aliasname, bm->resourceObject->get(AliasName));

  strcpy((char *) upl_header.reader_tear, BM_NAME);

  // UPI header

  UPI_HEADER upi_header;
  memset(&upi_header, 0, sizeof(upi_header));

  strncpy((char *) upi_header.vernum, (char *) upl_header.vernum, 12);

  // write header

  if (!(upl = fopen(uplName, "wb")) ||
      !(upi = fopen(upiName, "wb")) || !(net = fopen(netName, "wb")) ||
      (fwrite(&upl_header, sizeof(upl_header), 1, upl) != 1) ||
      (fwrite(&upi_header, sizeof(upi_header), 1, upi) != 1))
  {
    if (upl) fclose(upl);
    if (upi) fclose(upi);
    if (net) fclose(net);
    return false;
  }

  REPLY *r = firstReply;

  for (int l = 0; l < noOfLetters; l++)
  {
    if (!put1Reply(upl, upi, net, r))
    {
      success = false;
      break;
    }
    r = r->next;
  }

  fclose(upl);
  fclose(upi);
  fclose(net);

  if (fsize(netName) == 0) remove(netName);

  if (success && bm->areaList->areaConfig()) success = putConfig();

  return success;
}


bool bwave_reply::put1Reply (FILE *upl, FILE *upi, FILE *net, REPLY *r)
{
  FILE *reply;
  char *replyText, *p;
  bool success;
  const char *file = fname(r->filename);

  if (!(reply = fopen(r->filename, "rt"))) return false;

  replyText = new char[r->length + 1];
  success = (fread(replyText, sizeof(char), r->length, reply) ==
                                                          (size_t) r->length);
  fclose(reply);

  if (success)
  {
    replyText[r->length] = '\0';

    if ((reply = fopen(file, "wb")))
    {
      for (p = replyText; *p; p++)
      {
        if (*p == '\n') fputc('\r', reply);
        fputc(*p, reply);
        // Actually, only a \r and not \r\n should terminate lines in reply
        // files according to the Blue Wave documentation. But although Blue
        // Wave readers would display \r-terminated lines correctly, they
        // pass the files as they are to the editor. As (most of?) the Blue
        // Wave readers run under DOS, DOS type line termination ensures
        // interchangeability.
      }
      fclose(reply);

      // UPL
      strcpy((char *) r->upl.filename, file);
      if (success) success = (fwrite(&r->upl, sizeof(UPL_REC), 1, upl) == 1);

      // NET
      if (getLSBshort(r->upl.msg_attr) & UPL_NETMAIL)
      {
        NET_REC net_rec;
        memset(&net_rec, 0, sizeof(NET_REC));

        memcpy(net_rec.msg.from, r->upl.from, 36);
        memcpy(net_rec.msg.to, r->upl.to, 36);
        memcpy(net_rec.msg.subj, r->upl.subj, 72);
        mainDriver->tool->fidodate((char *) net_rec.msg.date,
                                   sizeof(net_rec.msg.date),
                                   (time_t) getLSBlong(r->upl.unix_date));
        memcpy(&net_rec.msg.dest, &r->upl.destnode, 2);
        putLSBshort(net_rec.msg.orig, mainDriver->getNode());
        putLSBshort(net_rec.msg.orig_net, mainDriver->getNet());
        memcpy(&net_rec.msg.destnet, &r->upl.destnet, 2);
        putLSBshort(net_rec.msg.reply, (unsigned int) getLSBlong(r->upl.replyto));
        int attr = getLSBshort(r->upl.netmail_attr) |
                   (getLSBshort(r->upl.msg_attr) & UPL_PRIVATE ? MSG_NET_PRIVATE
                                                               : 0);
        putLSBshort(net_rec.msg.attr, attr);
        memcpy(net_rec.fname, r->upl.filename, 13);
        memcpy(net_rec.echotag, r->upl.echotag, 21);
        memcpy(&net_rec.zone, &r->upl.destzone, 2);
        memcpy(&net_rec.point, &r->upl.destpoint, 2);
        memcpy(&net_rec.unix_date, &r->upl.unix_date, 4);

        if (success) success = (fwrite(&net_rec, sizeof(NET_REC), 1, net) == 1);
      }
      // UPI
      else
      {
        UPI_REC upi_rec;
        memset(&upi_rec, 0, sizeof(UPI_REC));

        memcpy(upi_rec.from, r->upl.from, 36);
        memcpy(upi_rec.to, r->upl.to, 36);
        memcpy(upi_rec.subj, r->upl.subj, 72);
        memcpy(&upi_rec.unix_date, &r->upl.unix_date, 4);
        memcpy(upi_rec.fname, r->upl.filename, 13);
        memcpy(upi_rec.echotag, r->upl.echotag, 21);
        if (getLSBshort(r->upl.msg_attr) & UPL_PRIVATE)
          upi_rec.flags = (tBYTE) UPI_PRIVATE;

        if (success) success = (fwrite(&upi_rec, sizeof(UPI_REC), 1, upi) == 1);
      }
    }
    else success = false;
  }

  delete[] replyText;
  return success;
}


bool bwave_reply::getReplies ()
{
  file_list *replies;
  const char *uplFile, *upiFile, *netFile;
  FILE *upl = NULL, *upi = NULL, *net = NULL;
  bool hasUpl;
  UPL_HEADER upl_header;
  unsigned int upl_headerlen, upl_reclen = 0;
  REPLY seed, *r = &seed;
  bool success = true;

  if ((replies = unpackReplies()))
  {
    uplFile = replies->exists(".upl");
    upiFile = replies->exists(".upi");
    netFile = replies->exists(".net");

    hasUpl = (uplFile && (upl = replies->ftryopen(uplFile, "rb")) &&
              (fread(&upl_header, sizeof(UPL_HEADER), 1, upl) == 1));

    // UPL
    if (hasUpl)
    {
      upl_headerlen = getLSBshort(upl_header.upl_header_len);
      upl_reclen = getLSBshort(upl_header.upl_rec_len);
      fseek(upl, upl_headerlen, SEEK_SET);
      noOfLetters = (replies->getSize() - upl_headerlen) / upl_reclen;
    }
    // UPI/NET
    else
    {
      if (!upiFile || !(upi = replies->ftryopen(upiFile, "rb")))
      {
        strcpy(error, "Could not open reply file.");
        success = false;
        goto GETREPLEND;
      }

      fseek(upi, sizeof(UPI_HEADER), SEEK_SET);
      noOfLetters = (replies->getSize() - sizeof(UPI_HEADER)) / sizeof(UPI_REC);

      if (netFile && (net = replies->ftryopen(netFile, "rb")))
        noOfLetters += replies->getSize() / sizeof(NET_REC);
    }

    seed.next = NULL;

    for (int l = 0; l < noOfLetters; l++)
    {
      r->next = new REPLY;
      r = r->next;
      r->filename = NULL;
      r->next = NULL;

      if (!get1Reply(upl, upi, net, r, hasUpl, upl_reclen))
      {
        strcpy(error, "Could not get reply.");
        success = false;
        break;
      }
    }

    firstReply = seed.next;

GETREPLEND:
    if (upl) fclose(upl);
    if (upi) fclose(upi);
    if (net) fclose(net);
    if (uplFile) remove(uplFile);
    if (upiFile) remove(upiFile);
    if (netFile) remove(netFile);
    delete replies;
    if (!success) clearDirectory(".");     // clean up
  }

  return success;
}


bool bwave_reply::get1Reply (FILE *upl, FILE *upi, FILE *net, REPLY *r,
                             bool hasUpl, unsigned int upl_reclen)
{
  FILE *oldReply, *newReply;
  char *replyText, *p;
  char filepath[MYMAXPATH];
  bool success;
  NET_REC net_rec;
  UPI_REC upi_rec;

  memset(&r->upl, 0, sizeof(UPL_REC));

  // UPL
  if (hasUpl)
  {
    if (fread(&r->upl, upl_reclen, 1, upl) != 1) return false;
  }
  else
  {
    // NET
    if (net && fread(&net_rec, sizeof(NET_REC), 1, net) == 1)
    {
      memcpy(r->upl.from, net_rec.msg.from, 36);
      memcpy(r->upl.to, net_rec.msg.to, 36);
      memcpy(r->upl.subj, net_rec.msg.subj, 72);
      memcpy(&r->upl.destzone, &net_rec.zone, 2);
      memcpy(&r->upl.destnet, &net_rec.msg.destnet, 2);
      memcpy(&r->upl.destnode, &net_rec.msg.dest, 2);
      memcpy(&r->upl.destpoint, &net_rec.point, 2);
      int attr = getLSBshort(net_rec.msg.attr);
      unsigned int replyto = getLSBshort(net_rec.msg.reply);
      putLSBshort(r->upl.msg_attr, UPL_NETMAIL |
                  (attr & MSG_NET_PRIVATE ? UPL_PRIVATE : 0) |
                  (replyto ? UPL_IS_REPLY : 0));
      putLSBshort(r->upl.netmail_attr, attr & (CRASH | FILEATTACHED | KILL |
                                               LOCAL | HOLD | IMMEDIATE |
                                               FILEREQUEST | DIRECT |
                                               FILEUPDATEREQ));
      memcpy(&r->upl.unix_date, &net_rec.unix_date, 4);
      putLSBlong(r->upl.replyto, replyto);
      memcpy(r->upl.filename, net_rec.fname, 13);
      memcpy(r->upl.echotag, net_rec.echotag, 21);
    }
    // UPI
    else if (fread(&upi_rec, sizeof(UPI_REC), 1, upi) == 1)
    {
      memcpy(r->upl.from, upi_rec.from, 36);
      memcpy(r->upl.to, upi_rec.to, 36);
      memcpy(r->upl.subj, upi_rec.subj, 72);
      if (upi_rec.flags & UPI_PRIVATE)
        putLSBshort(r->upl.msg_attr, UPL_PRIVATE);
      memcpy(&r->upl.unix_date, &upi_rec.unix_date, 4);
      memcpy(r->upl.filename, upi_rec.fname, 13);
      memcpy(r->upl.echotag, upi_rec.echotag, 21);
    }
    else return false;
  }

  char *upl_filename = (char *) r->upl.filename;
  long msglen = fsize(upl_filename);

  if (!(oldReply = fopen(upl_filename, "rb"))) return false;

  replyText = new char[msglen + 1];
  msglen = fread(replyText, sizeof(char), msglen, oldReply);
  replyText[msglen] = '\0';
  fclose(oldReply);

  mkfname(filepath, bm->resourceObject->get(WorkDir), upl_filename);

  if ((success = ((newReply = fopen(filepath, "wt")) != NULL)))
  {
    r->length = 0;

    for (p = replyText; *p; p++)
      if (*p != '\n')
      {
        if (*p == '\r') *p = '\n';
        fputc(*p, newReply);
        r->length++;
      }

    r->filename = strdupplus(filepath);
    fclose(newReply);
  }

  delete[] replyText;
  if (success) success = (remove(upl_filename) == 0);

  return success;
}


void bwave_reply::getConfig ()
{
  FILE *cfg;
  bool olc, area_changes = false;
  char buffer[MYMAXLINE], *keyword, *value;
  PDQ_HEADER pdqHead;
  PDQ_REC pdqRec;

  file_list *replies = new file_list(bm->resourceObject->get(ReplyWorkDir));

  const char *olcFile = replies->exists(".olc");
  const char *pdqFile = replies->exists(".pdq");

  cfg = replies->ftryopen(olcFile, "rt");

  if (cfg) olc = true;
  else
  {
    olc = false;
    cfg = replies->ftryopen(pdqFile, "rb");
  }

  if (cfg)
  {
    // OLC
    if (olc)
    {
      while (fgetsnl(buffer, sizeof(buffer), cfg))
      {
        parseOption(mkstr(buffer), '=', keyword, value);

        if (strcasecmp(keyword, "AreaChanges") == 0 &&
            (strcasecmp(value, "YES") == 0 ||
             strcasecmp(value, "ON") == 0 ||
             strcasecmp(value, "TRUE") == 0))
        {
          area_changes = true;
          break;
        }
      }
    }
    // PDQ
    else
    {
      if (fread(&pdqHead, sizeof(PDQ_HEADER), 1, cfg) == 1 &&
          (getLSBshort(pdqHead.flags) & PDQ_AREA_CHANGES) != 0)
        area_changes = true;
    }

    if (area_changes)
    {
      int offset = bm->driverList->getOffset(mainDriver) +
                   mainDriver->persOffset();

      for (int a = offset; a < bm->areaList->getNoOfAreas(); a++)
      {
        bm->areaList->gotoArea(a);
        if (bm->areaList->isSubscribed()) bm->areaList->setDropped();
      }

      int index;

      // OLC
      if (olc)
      {
        while (fgetsnl(buffer, sizeof(buffer), cfg))
        {
          if (*buffer == '[' && ((value = strrchr(buffer, ']'))))
          {
            *value = '\0';
            if ((index = getAreaIndex(buffer + 1)))
            {
              bm->areaList->gotoArea(index);
              bm->areaList->setAdded();
            }
          }
        }
      }
      // PDQ
      else
      {
        while (fread(&pdqRec, sizeof(PDQ_REC), 1, cfg) == 1)
        {
          if ((index = getAreaIndex((char *) pdqRec.echotag)))
          {
            bm->areaList->gotoArea(index);
            bm->areaList->setAdded();
          }
        }
      }
    }

    fclose(cfg);
  }

  if (olcFile) remove(olcFile);
  if (pdqFile) remove(pdqFile);

  delete replies;
}


bool bwave_reply::putConfig ()
{
  char olcName[13], pdqName[13];
  FILE *olc, *pdq = NULL;
  PDQ_HEADER pdqHead;
  PDQ_REC pdqRec;
  int offset;
  bool success = true;

  sprintf(olcName, "%.8s.olc", mainDriver->getPacketID());
  sprintf(pdqName, "%.8s.pdq", mainDriver->getPacketID());

  INF_HEADER &infHeader = mainDriver->getInfHeader();

  if ((olc = fopen(olcName, "wb")) && (pdq = fopen(pdqName, "wb")))
  {
    // OLC Header
    if (fprintf(olc, "[Global Mail Host Configuration]\r\n"
                     "AreaChanges = ON\r\n\r\n") != 54)
    {
      success = false;
      goto PUTCONFEND;
    }

    // PDQ HEADER
    memset(&pdqHead, 0, sizeof(PDQ_HEADER));

    memcpy(&pdqHead, infHeader.keywords, sizeof(infHeader.keywords) +
                                         sizeof(infHeader.filters) +
                                         sizeof(infHeader.macros));
    memcpy(pdqHead.password, infHeader.password, sizeof(infHeader.password) +
                                                 sizeof(infHeader.passtype));
    putLSBshort(pdqHead.flags,
                getLSBshort(infHeader.uflags) | PDQ_AREA_CHANGES);

    if (fwrite(&pdqHead, sizeof(PDQ_HEADER), 1, pdq) != 1)
    {
      success = false;
      goto PUTCONFEND;
    }

    // Data
    offset = bm->driverList->getOffset(mainDriver) +
             mainDriver->persOffset();

    for (int a = offset; a < bm->areaList->getNoOfAreas(); a++)
    {
      bm->areaList->gotoArea(a);
      const char *echotag = bm->areaList->getShortName();

      if (bm->areaList->isAdded() ||
          (bm->areaList->isSubscribed() && !bm->areaList->isDropped()))
      {
        // OLC
        if (fprintf(olc, "[%s]\r\nScan = ALL\r\n\r\n", echotag) !=
            (int) strlen(echotag) + 18)
        {
          success = false;
          break;
        }

        // PDQ
        memset(&pdqRec, 0, sizeof(PDQ_REC));
        strcpy((char *) pdqRec.echotag, echotag);
        if (fwrite(&pdqRec, sizeof(PDQ_REC), 1, pdq) != 1)
        {
          success = false;
          break;
        }
      }
    }

PUTCONFEND:
    fclose(olc);
    fclose(pdq);

    if (!success)
    {
      remove(olcName);
      remove(pdqName);
    }
  }

  return success;
}
