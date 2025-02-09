/*
 * blueMail offline mail reader
 * OMEN driver

 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "omen.h"
#include "../../common/auxil.h"
#include "../../common/error.h"
#include "../../common/mysystem.h"


/*
   OMEN main driver
*/

omen::omen (bmail *bm)
{
  this->bm = bm;
  tool = new basic_tool(bm, this);

  areas = NULL;
  persIDX = NULL;
  persmsgs = 0;

  persArea = bm->resourceObject->isYes(PersonalArea);
  if (bm->getServiceType() == ST_REPLY) persArea = false;

  c_set_iso = select_on = false;

  body = NULL;
  newmsg = NULL;
}


omen::~omen ()
{
  while (maxAreas--)
  {
    delete[] body[maxAreas];
    delete[] areas[maxAreas].title;
  }

  delete[] bulletins;
  delete[] letterBody;
  delete[] body;
  delete[] persIDX;
  delete[] areas;
  delete tool;

  if (newmsg) fclose(newmsg);
}


const char *omen::init ()
{
  FILE *system;

  const char *username = bm->resourceObject->get(UserName);
  bm->resourceObject->set(LoginName, username);
  bm->resourceObject->set(AliasName, username);
  bm->resourceObject->set(SysOpName, "");
  bm->resourceObject->set(hasLoginName, "N");

  const char *sysfile = bm->fileList->exists("system*");

  bool areas_ok = ((system = bm->fileList->ftryopen(sysfile, "rb")) &&
                   getAreas(system));

  if (system) fclose(system);
  if (!areas_ok) return "Could not access system file.";

  strncpy(packetID, sysfile + 6, 2);
  packetID[2] = '\0';

  readInfo();

  if ((persmsgs = buildIndices()) == -1) return "Could not read newmsg file.";

  const char bulletinfile[2][13] = {"bullet*", "nfile*"};
  bulletins = tool->bulletin_list(bulletinfile, 2, BL_NOADD);

  return NULL;
}


area_header *omen::getNextArea ()
{
  area_header *ah;
  char areanum[12];
  int flags;

  bool isPersArea = (persArea && (NAID == 0));

  sprintf(areanum, "%d", areas[NAID].number);

  if (isPersArea) flags = A_COLLECTION | A_READONLY | A_LIST;
  else flags = (areas[NAID].flags & BrdStatus_SELECTED ? A_SUBSCRIBED : 0) |
               (areas[NAID].flags & BrdStatus_NETMAIL ? A_NETMAIL : 0) |
               (areas[NAID].flags & BrdStatus_WRITE ? 0 : A_READONLY) |
               (areas[NAID].flags & BrdStatus_ALIAS ? A_ALIAS : 0) |
               (areas[NAID].flags & BrdStatus_PRIVATE ? A_ALLOWPRIV : 0) |
               (isLatin1() ? A_CHRSLATIN1 : 0);

  ah = new area_header(bm, NAID + bm->driverList->getOffset(this),
                       (isPersArea ? "PERS" : areanum),
                       (isPersArea ? "PERSONAL" : areas[NAID].shortname),
                       (isPersArea ? "Letters addressed to you"
                                   : (areas[NAID].title ? areas[NAID].title
                                                        : areas[NAID].shortname)),
                       (isPersArea ? "OMEN Personal" : "OMEN"),
                       flags, areas[NAID].totmsgs, areas[NAID].numpers,
                       35, 72);
  NAID++;

  return ah;
}


inline int omen::getNoOfLetters ()
{
  return areas[currentArea].totmsgs;
}


letter_header *omen::getNextLetter ()
{
  int area, letter;
  char *p, *q, *date, *faddr, subject[73];
  unsigned long prev = 0, next;
  net_address na;

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

  fseek(newmsg, body[area][letter].pointer, SEEK_SET);
  fgetsnl(line, sizeof(line), newmsg);

  p = strchr(line, ' ');
  *p = '\0';
  int msgnum = (int) atol(line + 2);     // we're somewhat limited here

  p = strstr(p + 1, "  ");
  q = strchr(p + 1, '(');
  *(q - 2) = '\0';
  date = strdupplus(p + 2);

  sscanf(q, "(%lu/%lu)", &prev, &next);
  int replyto = (int) prev;            // we're somewhat limited here

  p = strchr(q + 1, '(');

  int flags = (strchr(p, 'P') ? PRIVATE : 0) |
              (strchr(p, 'R') ? RECEIVED : 0);

  fgetsnl(line, sizeof(line), newmsg);

  p = strstr(mkstr(line), " => ");
  *p = '\0';
  q = p + 4;

  if ((faddr = strcasestr(q, " From: (")))
  {
    na.set(faddr + 8);
    *faddr = '\0';
  }

  // skip "Subj: "
  fread(subject, 1, 6, newmsg);

  int c;
  unsigned int i = 0;

  do
  {
    c = fgetc(newmsg);
    subject[i++] = c;
  }
  while (c != 2 && i < sizeof(subject));

  subject[i - 1] = '\0';

  NLID++;
  letter_header *lh = new letter_header(bm, letter, line, q, subject, date,
                                        msgnum, replyto, area, flags,
                                        isLatin1(), &na, NULL, NULL, NULL,
                                        this);
  delete[] date;

  return lh;
}


const char *omen::getBody (int area, int letter, letter_header *lh)
{
  int kar;

  delete[] letterBody;
  letterBody = new char[body[area][letter].msglen + 1];
  unsigned char *p = (unsigned char *) letterBody;

  fseek(newmsg, body[area][letter].pointer, SEEK_SET);
  // skip header
  while (!feof(newmsg) && (fgetc(newmsg) != 2)) ;

  for (long c = 0; c < body[area][letter].msglen; c++)
  {
    kar = fgetc(newmsg);

    // a message shouldn't (must not) contain a 0x00 character
    if (kar == 0) kar = ' ';

    if (kar != '\r') *p++ = kar;
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


void omen::initMSF (int **msgstat)
{
  tool->MSFinit(msgstat, persOffset(), persIDX, persmsgs);
}


int omen::readMSF (int **msgstat)
{
  tool->XTIread(msgstat, persOffset());
  return (MSF_READ | MSF_REPLIED | MSF_PERSONAL | MSF_MARKED);
}


const char *omen::saveMSF (int **msgstat)
{
  return tool->XTIsave(msgstat, persOffset(), packetID);
}


inline const char *omen::getPacketID () const
{
  return packetID;
}


inline bool omen::isLatin1 ()
{
  return c_set_iso;
}


inline bool omen::offlineConfig ()
{
  return select_on;
}


bool omen::getAreas (FILE *sysfile)
{
  struct
  {
    unsigned char BrdNum;
    unsigned char BrdStatus;
    unsigned char BrdHighNum;
    char BrdName[17];         // 1 byte length + 16 char (Pascal style string)
  } BrdRecord;

  // system name is 1 byte length + 40 char (Pascal style string)
  if (fread(line, 41, 1, sysfile) != 1) return false;
  line[*line + 1] = '\0';     // make it a C style string

  bm->resourceObject->set(BBSName, line + 1);

  maxAreas = (bm->fileList->getSize() - 41) / sizeof(BrdRecord) + persOffset();

  areas = new OMEN_AREA_INFO[maxAreas];

  if (persArea)
  {
    areas[0].title = NULL;
    areas[0].totmsgs = areas[0].numpers = 0;
  }

  FILE *bnames = bm->fileList->ftryopen("bnames*", "rt");

  int i = persOffset();

  while (fread(&BrdRecord, sizeof(BrdRecord), 1, sysfile) == 1)
  {
    areas[i].number = (BrdRecord.BrdHighNum << 8) + BrdRecord.BrdNum;
    strncpy(areas[i].shortname, BrdRecord.BrdName + 1, *BrdRecord.BrdName);
    areas[i].shortname[(unsigned char) *BrdRecord.BrdName] = '\0';
    areas[i].title = NULL;
    areas[i].flags = BrdRecord.BrdStatus;
    areas[i].totmsgs = 0;
    areas[i].numpers = 0;

    if (bnames && fgetsnl(line, sizeof(line), bnames))
      areas[i].title = strdupplus(strchr(mkstr(line), ':') + 1);

    i++;
  }

  if (bnames) fclose(bnames);

  return true;
}


void omen::readInfo ()
{
  FILE *info;

  if ((info = bm->fileList->ftryopen("info*", "rt")))
  {
    while (fgetsnl(line, sizeof(line), info))
    {
      mkstr(line);

      if (strncasecmp(line, "SYSOP:", 6) == 0)
        bm->resourceObject->set(SysOpName, line + 6);
      else if (strcasecmp(line, "C_SET:ISO") == 0) c_set_iso = true;
      else if (strcasecmp(line, "SELECT:ON") == 0) select_on = true;
    }

    fclose(info);
  }
}


int omen::buildIndices ()
{
  size_t readlen;
  char MsgNum[11];
  int BoardNum, pers = 0;
  char *separator = NULL, *faddr;
  pIDX pAnchor, *pidx = &pAnchor, *d;

  body = new bodytype *[maxAreas];
  for (int i = 0; i < maxAreas; i++) body[i] = NULL;

  if (!(newmsg = bm->fileList->ftryopen("newmsg*", "rb"))) return -1;

  struct MSG_IDX
  {
    int area;
    long pointer;
    long msglen;
    MSG_IDX *next;
  } *mAnchor = new MSG_IDX, *ndx = mAnchor, *i = NULL, *next;

  const char *username = bm->resourceObject->get(LoginName);

  while ((readlen = fgetsnl(line, sizeof(line), newmsg)))
  {
    if (sscanf(line, "\1#%s  %d:", MsgNum, &BoardNum) == 2)
    {
      int aix = getAreaIndex(BoardNum);

      if (aix != -1)
      {
        ndx->area = aix;
        ndx->pointer = ftell(newmsg) - readlen;
        ndx->msglen = 0;
        areas[aix].totmsgs++;

        i = ndx;

        ndx->next = new MSG_IDX;
        ndx = ndx->next;

        if (fgetsnl(line, sizeof(line), newmsg) &&
            (separator = strstr(mkstr(line), " => ")))
        {
          if ((faddr = strcasestr(separator + 4, " From: ("))) *faddr = '\0';
          if (strcasecmp(separator + 4, username) == 0)
              /* ||               AliasName */
          {
            pers++;
            areas[aix].numpers++;
            pidx->next = new pIDX;
            pidx = pidx->next;
            pidx->persidx.area = aix;
            pidx->persidx.msgnum = areas[aix].totmsgs - 1;
            pidx->next = NULL;
          }
        }
      }

      // skip header
      while (!feof(newmsg) && (fgetc(newmsg) != 2)) ;

      long pos = ftell(newmsg);

      // skip body
      while (!feof(newmsg) && (fgetc(newmsg) != 3)) ;

      if (aix != -1) i->msglen = ftell(newmsg) - pos - 1;
    }
  }

  for (int a = 0; a < maxAreas; a++)
  {
    body[a] = new bodytype[areas[a].totmsgs];
    areas[a].totmsgs = 0;
  }

  i = mAnchor;

  while (i != ndx)
  {
    next = i->next;

    int a = i->area;
    body[a][areas[a].totmsgs].pointer = i->pointer;
    body[a][areas[a].totmsgs].msglen = i->msglen;
    areas[a].totmsgs++;

    delete i;
    i = next;
  }

  delete ndx;

  if (pers)
  {
    persIDX = new PERSIDX[pers];
    pidx = pAnchor.next;
    int i = 0;

    while (pidx)
    {
      persIDX[i].area = pidx->persidx.area;
      persIDX[i].msgnum = pidx->persidx.msgnum;

      i++;
      d = pidx;
      pidx = pidx->next;
      delete d;
    }
  }

  if (persArea) areas[0].totmsgs = areas[0].numpers = pers;

  return pers;
}


int omen::getAreaIndex (int area) const
{
  int a;

  // -1 is an internal pseudo-number for the personal area
  if (area == -1) return 0;

  for (a = persOffset(); a < maxAreas; a++)
    if (areas[a].number == area) break;

  return (a == maxAreas ? -1 : a);
}


/*
   OMEN reply driver
*/

omen_reply::omen_reply (bmail *bm, main_driver *mainDriver)
{
  this->bm = bm;
  this->mainDriver = (omen *) mainDriver;
  firstReply = currentReply = NULL;
}


omen_reply::~omen_reply ()
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


const char *omen_reply::init ()
{
  char ext[4];

  sprintf(ext, "%.3s", bm->service->getArchiveExt());
  sprintf(replyPacketName, "RETURN%.2s.%s", mainDriver->getPacketID(),
                                            strupper(ext));

  if (bm->getServiceType() != ST_REPLY) replyInf();

  return (getReplies() ? NULL : error);
}


inline area_header *omen_reply::getNextArea ()
{
  return getReplyArea("OMEN Replies");
}


letter_header *omen_reply::getNextLetter ()
{
  char alias[21], to[36], subject[73];
  unsigned char len;
  int replyto, area, flags;
  unsigned long num;
  net_address na;

  const char *from = "";

  if (currentReply->header.Command & Command_ALIAS)
  {
    len = currentReply->header.alias_length;
    if (len > sizeof(currentReply->header.Alias))
      len = sizeof(currentReply->header.Alias);
    strncpy(alias, currentReply->header.Alias, len);
    alias[len] = '\0';
    from = alias;
  }

  len = currentReply->header.to_length;
  if (len > sizeof(currentReply->header.WhoTo))
    len = sizeof(currentReply->header.WhoTo);
  strncpy(to, currentReply->header.WhoTo, len);
  to[len] = '\0';

  len = currentReply->header.subj_length;
  if (len > sizeof(currentReply->header.Subject))
    len = sizeof(currentReply->header.Subject);
  strncpy(subject, currentReply->header.Subject, len);
  subject[len] = '\0';

  num = (getLSBshort(currentReply->header.MsgHighNumber) << 16) +
        getLSBshort(currentReply->header.MsgNumber);
  // we're somewhat limited here
  replyto = (int) num;

  area = mainDriver->getAreaIndex(getLSBshort(&currentReply->header.CurBoard));
  area += (area == -1 ? 1 : bm->driverList->getOffset(mainDriver));

  flags = (currentReply->header.Command & Command_PRIVATE ? PRIVATE : 0) |
          (currentReply->header.NetAttrib & NetAttrib_KILL ? KILL : 0) |
          (currentReply->header.NetAttrib & NetAttrib_CRASH ? CRASH : 0);

  na.set(getLSBshort(currentReply->header.DestZone),
         getLSBshort(currentReply->header.DestNet),
         getLSBshort(currentReply->header.DestNode),
         0);   // unfortunately, no DestPoint

  letter_header *newLetter = new letter_header(bm, NLID, from, to, subject,
                                               "", NLID, replyto, area, flags,
                                               isLatin1(), &na, NULL, NULL,
                                               NULL, this);

  NLID++;
  currentReply = currentReply->next;

  return newLetter;
}


const char *omen_reply::getBody (int area, int letter, letter_header *lh)
{
  FILE *replyFile;

  (void) area;
  (void) lh;
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
    fclose(replyFile);
  }
  else replyBody = strdupplus("");

  return replyBody;
}


void omen_reply::resetLetters ()
{
  NLID = 1;
  currentReply = firstReply;
}


inline bool omen_reply::isLatin1 ()
{
  return mainDriver->isLatin1();
}


void omen_reply::enterLetter (letter_header *newLetter,
                              const char *newLetterFileName, long msglen)
{
  REPLY *newReply = new REPLY;
  memset(newReply, 0, sizeof(REPLY));

  newReply->header.Command = Command_SAVE;

  if (bm->areaList->useAlias())
  {
    unsigned char len = strlen(newLetter->getFrom());
    if (len > sizeof(newReply->header.Alias))
      len = sizeof(newReply->header.Alias);
    strncpy(newReply->header.Alias, newLetter->getFrom(), len);
    newReply->header.alias_length = len;
    newReply->header.Command |= Command_ALIAS;
  }

  newReply->header.to_length = strlen(newLetter->getTo());
  strncpy(newReply->header.WhoTo, newLetter->getTo(), 35);

  newReply->header.subj_length = strlen(newLetter->getSubject());
  strncpy(newReply->header.Subject, newLetter->getSubject(), 72);

  unsigned char replyto[4];
  putLSBlong(replyto, (unsigned long) newLetter->getReplyTo());
  memcpy(newReply->header.MsgNumber, replyto, 2);
  memcpy(newReply->header.MsgHighNumber, replyto + 2, 2);

  putLSBshort(&newReply->header.CurBoard,
              (unsigned int) atoi(bm->areaList->getNumber()));

  if (newLetter->isPrivate()) newReply->header.Command |= Command_PRIVATE;

  int flags = newLetter->getFlags();
  newReply->header.NetAttrib = (flags & KILL ? NetAttrib_KILL : 0) |
                               (flags & CRASH ? NetAttrib_CRASH : 0);

  if (bm->areaList->isNetmail())
  {
    net_address na = newLetter->getNetAddr();
    putLSBshort(newReply->header.DestZone, na.getZone());
    putLSBshort(newReply->header.DestNet, na.getNet());
    putLSBshort(newReply->header.DestNode, na.getNode());
  }

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


void omen_reply::killLetter (int letter)
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


const char *omen_reply::newLetterBody (int letter, long size)
{
  REPLY *r = firstReply;
  for (int l = 1; l < letter; l++) r = r->next;

  if (size >= 0) r->length = size;
  return r->filename;
}


void omen_reply::replyInf ()
{
  char fname[MYMAXPATH];

  const char *sysfile = bm->fileList->exists("system*");
  const char *newmsg = bm->fileList->exists("newmsg*");
  const char *bnames = bm->fileList->exists("bnames*");
  const char *info = bm->fileList->exists("info*");
  const char *dir = bm->resourceObject->get(InfWorkDir);

  mkfname(fname, dir, sysfile);
  fcopy(sysfile, fname);

  fcreate(dir, newmsg);

  if (bnames)
  {
    mkfname(fname, dir, bnames);
    fcopy(bnames, fname);
  }

  if (info)
  {
    mkfname(fname, dir, info);
    fcopy(info, fname);
  }

  bm->service->createSystemInfFile(fmtime(sysfile),
                                   mainDriver->getPacketID(),
                                   strrchr(replyPacketName, '.') + 1,
                                   replyPacketName);
}


bool omen_reply::putReplies ()
{
  char hdrName[13];
  FILE *hdr;
  bool success = true;

  // OMEN allows a maximum of 100 replies per packet
  if (noOfLetters > 100) return false;

  sprintf(hdrName, "HEADER%.2s.BBS", mainDriver->getPacketID());

  if (!(hdr = fopen(hdrName, "wb"))) return false;

  REPLY *r = firstReply;

  for (int l = 0; l < noOfLetters; l++)
  {
    if (!put1Reply(hdr, r, l))
    {
      success = false;
      break;
    }
    r = r->next;
  }

  fclose(hdr);

  if (success && bm->areaList->areaConfig()) success = putConfig();

  return success;
}


bool omen_reply::put1Reply (FILE *hdr, REPLY *r, int letter)
{
  FILE *reply;
  char *replyText, msgName[13], *p;
  bool success;

  if (!(reply = fopen(r->filename, "rt"))) return false;

  replyText = new char[r->length + 1];
  success = (fread(replyText, sizeof(char), r->length, reply) ==
                                                          (size_t) r->length);
  fclose(reply);

  if (success)
  {
    replyText[r->length] = '\0';

    sprintf(msgName, "MSG%.2s%02d.TXT", mainDriver->getPacketID(), letter);

    success = ((reply = fopen(msgName, "wb")) != NULL);

    if (success)
    {
      for (p = replyText; *p; p++)
      {
        if (*p == '\n') fputc('\r', reply);
        fputc(*p, reply);
      }
      fclose(reply);
    }

    if (success)
      success = (fwrite(&r->header, sizeof(OMEN_REPLY_HEADER), 1, hdr) == 1);
  }

  delete[] replyText;
  return success;
}


bool omen_reply::getReplies ()
{
  file_list *replies;
  const char *hdrFile = NULL;
  FILE *hdr = NULL;
  char filename[13];
  bool success = true;

  if (!(replies = unpackReplies())) return true;

  if (!(hdrFile = replies->exists("header*")) ||
      !(hdr = replies->ftryopen(hdrFile, "rb")))
  {
    strcpy(error, "Could not open reply file.");
    success = false;
  }
  else
  {
    noOfLetters = replies->getSize() / sizeof(OMEN_REPLY_HEADER);

    REPLY seed, *r = &seed;
    seed.next = NULL;

    for (int l = 0; l < noOfLetters; l++)
    {
      r->next = new REPLY;
      r = r->next;
      r->filename = NULL;
      r->next = NULL;

      sprintf(filename, "msg%.2s%02d.txt", mainDriver->getPacketID(), l);
      const char *msg = replies->exists(filename);

      if (!msg || !get1Reply(hdr, r, msg))
      {
        strcpy(error, "Could not get reply.");
        success = false;
        break;
      }
    }

    firstReply = seed.next;
  }

  if (hdr) fclose(hdr);
  if (hdrFile) remove(hdrFile);
  delete replies;
  if (!success) clearDirectory(".");     // clean up

  return success;
}


bool omen_reply::get1Reply (FILE *hdr, REPLY *r, const char *msg)
{
  FILE *reply;
  char filename[13], filepath[MYMAXPATH], *replyText, *p;
  static int fnr = 1;
  bool success;

  if (fread(&r->header, sizeof(OMEN_REPLY_HEADER), 1, hdr) != 1) return false;

  // we can't handle the other commands
  if (r->header.Command & ~(Command_SAVE | Command_PRIVATE | Command_ALIAS))
    return false;

  sprintf(filename, "%d.%03d", getLSBshort(&r->header.CurBoard), fnr++);
  mkfname(filepath, bm->resourceObject->get(WorkDir), filename);

  if (!(reply = fopen(msg, "rb"))) return false;

  long msglen = fsize(msg);
  replyText = new char[msglen + 1];
  msglen = fread(replyText, sizeof(char), msglen, reply);
  replyText[msglen] = '\0';
  fclose(reply);

  success = ((reply = fopen(filepath, "wt")) != NULL);

  if (success)
  {
    r->length = 0;

    for (p = replyText; *p; p++)
      if (*p != '\r')
      {
        fputc(*p, reply);
        r->length++;
      }

    r->filename = strdupplus(filepath);
    fclose(reply);
  }

  delete[] replyText;
  if (success) success = (remove(msg) == 0);

  return success;
}


void omen_reply::getConfig ()
{
  FILE *cnf;
  char buffer[MYMAXLINE];

  file_list *replies = new file_list(bm->resourceObject->get(ReplyWorkDir));

  const char *cnfFile = replies->exists(".cnf");

  if ((cnf = replies->ftryopen(cnfFile, "rt")))
  {
    int offset = bm->driverList->getOffset(mainDriver) +
                 mainDriver->persOffset();

    for (int a = offset; a < bm->areaList->getNoOfAreas(); a++)
    {
      bm->areaList->gotoArea(a);
      if (bm->areaList->isSubscribed()) bm->areaList->setDropped();
    }

    int index;

    while (fgetsnl(buffer, sizeof(buffer), cnf))
    {
      if ((index = mainDriver->getAreaIndex(atoi(mkstr(buffer)))) != -1)
      {
        bm->areaList->gotoArea(index + bm->driverList->getOffset(mainDriver));
        bm->areaList->setAdded();
      }
    }

    fclose(cnf);
  }

  if (cnfFile) remove(cnfFile);

  delete replies;
}


bool omen_reply::putConfig ()
{
  char cnfName[13];
  FILE *cnf;
  int offset;
  bool success = true;

  sprintf(cnfName, "SELECT%.2s.CNF", mainDriver->getPacketID());

  if ((cnf = fopen(cnfName, "wb")))
  {
    offset = bm->driverList->getOffset(mainDriver) +
             mainDriver->persOffset();

    for (int a = offset; a < bm->areaList->getNoOfAreas(); a++)
    {
      bm->areaList->gotoArea(a);
      const char *areanum = bm->areaList->getNumber();

      if (bm->areaList->isAdded() ||
          (bm->areaList->isSubscribed() && !bm->areaList->isDropped()))
      {
        if (fprintf(cnf, "%s\r\n", areanum) != (int) strlen(areanum) + 2)
        {
          success = false;
          break;
        }
      }
    }

    fclose(cnf);
  }

  if (!success) remove(cnfName);

  return success;
}
