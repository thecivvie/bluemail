/*
 * blueMail offline mail reader
 * Hippo v2 driver

 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "hippo.h"
#include "../../common/auxil.h"
#include "../../common/error.h"
#include "../../common/mysystem.h"


/*
   Hippo v2 main driver
*/

hippo::hippo (bmail *bm)
{
  this->bm = bm;
  tool = new basic_tool(bm, this);

  areas = NULL;
  persIDX = NULL;
  persmsgs = 0;

  persArea = bm->resourceObject->isYes(PersonalArea);
  if (bm->getServiceType() == ST_REPLY) persArea = false;

  hasInfo = gotAddr = false;

  body = NULL;
  hd = NULL;
}


hippo::~hippo ()
{
  while (maxAreas--)
  {
    delete[] body[maxAreas];
    delete[] areas[maxAreas]->title;
    delete areas[maxAreas];
  }

  delete[] letterBody;
  delete[] body;
  delete[] persIDX;
  delete[] areas;
  delete tool;

  if (hd) fclose(hd);
}


const char *hippo::init ()
{
  const char *username = bm->resourceObject->get(UserName);
  bm->resourceObject->set(LoginName, username);
  bm->resourceObject->set(AliasName, username);
  bm->resourceObject->set(SysOpName, "");
  bm->resourceObject->set(BBSName, "");
  bm->resourceObject->set(hasLoginName, "N");

  const char *hdfile = bm->fileList->exists(".hd");

  if (!(hd = bm->fileList->ftryopen(hdfile, "rb")))
    return "Could not open data file.";

  int len = strrchr(hdfile, '.') - hdfile;
  len = (len > 8 ? 8 : len);
  strncpy(packetID, hdfile, len);
  packetID[len] = '\0';

  persmsgs = buildIndices();

  return NULL;
}


area_header *hippo::getNextArea ()
{
  area_header *ah;
  int flags;

  bool isPersArea = (persArea && (NAID == 0));

  if (isPersArea) flags = A_COLLECTION | A_READONLY | A_LIST;
  else flags = areas[NAID]->flags | (hasInfo ? 0 : A_SUBSCRIBED) |
                                    (isLatin1() ? A_CHRSLATIN1 : 0);

  ah = new area_header(bm, NAID + bm->driverList->getOffset(this),
                       (isPersArea ? "PERS" : ""),
                       (isPersArea ? "PERSONAL" : areas[NAID]->title),
                       (isPersArea ? "Letters addressed to you"
                                   : areas[NAID]->title),
                       (isPersArea ? "Hippo Personal" : "Hippo"), flags,
                       areas[NAID]->totmsgs, areas[NAID]->numpers, 71, 71);
  NAID++;

  return ah;
}


inline int hippo::getNoOfLetters ()
{
  return areas[currentArea]->totmsgs;
}


letter_header *hippo::getNextLetter ()
{
  char *from = NULL, *to = NULL, *subject = NULL;
  char date[20];
  int flags = 0, msgnum = 0, replyto = 0;

  *date = '\0';

  int area, letter;

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

  fseek(hd, body[area][letter].pointer, SEEK_SET);
  body[area][letter].msglen = 0;

  while (fgetsnl(line, sizeof(line), hd))
  {
    mkstr(line);

    if (strncasecmp(line, "Lines: ", 7) == 0)
    {
      long lines = atol(line + 7);
      for (long i = 0; i < lines; i++)
        body[area][letter].msglen += fgetsnl(line, sizeof(line), hd);
      break;
    }

    else if (strncasecmp(line, "Status: ", 8) == 0)
    {
      flags |= (strcasestr(line + 8, "Private") ? PRIVATE : 0);
      flags |= (strcasestr(line + 8, "Read") ? RECEIVED : 0);
    }

    else if (strncasecmp(line, "Number: ", 8) == 0) msgnum = atoi(line + 8);

    else if (strncasecmp(line, "Reply: ", 7) == 0) replyto = atoi(line + 7);

    else if (strncasecmp(line, "From: ", 6) == 0) from = strdupplus(line + 6);

    else if (strncasecmp(line, "To: ", 4) == 0) to = strdupplus(line + 4);

    else if (strncasecmp(line, "Subject: ", 9) == 0)
      subject = strdupplus(line + 9);

    else if (strncasecmp(line, "Date: ", 6) == 0)
      sprintf(date, "%2.2s.%2.2s.%4.4s %2.2s:%2.2s:%2.2s", line + 12,
                    line + 10, line + 6, line + 14, line + 16, line + 18);
  }

  net_address na;
  if (areas[area]->flags & A_NETMAIL)
  {
    if (areas[area]->flags & A_INTERNET)
    {
      na.set(from);
      delete[] from;
      from = NULL;
    }
    else
    {
      tool->getBBBSAddr(subject, mainAddr.getZone(), na, flags);
      if ((flags & RECEIVED) == 0) na = mainAddr;
    }
  }

  letter_header *lh = new letter_header(bm, letter,
                                        (from ? from : ""), (to ? to : ""),
                                        (subject ? subject : ""), date,
                                        msgnum, replyto, area, flags,
                                        isLatin1(), &na, NULL, NULL, NULL,
                                        this);

  delete[] from;
  delete[] to;
  delete[] subject;

  NLID++;
  return lh;
}


const char *hippo::getBody (int area, int letter, letter_header *lh)
{
  (void) lh;

  int kar;

  delete[] letterBody;
  letterBody = new char[body[area][letter].msglen + 1];
  unsigned char *p = (unsigned char *) letterBody;

  fseek(hd, body[area][letter].pointer, SEEK_SET);
  // skip header
  while (fgetsnl(line, sizeof(line), hd))
    if (strncasecmp(line, "Lines: ", 7) == 0) break;

  for (long c = 0; c < body[area][letter].msglen; c++)
  {
    kar = fgetc(hd);

    // a message shouldn't (must not) contain a 0x00 character
    if (kar == 0) kar = ' ';

    if (kar != '\r') *p++ = kar;
  }

  do p--;
  // strip blank lines
  while (p >= (unsigned char *) letterBody && (*p == ' ' || *p == '\n'));

  *++p = '\0';

  return letterBody;
}


void hippo::initMSF (int **msgstat)
{
  tool->MSFinit(msgstat, persOffset(), persIDX, persmsgs);
}


int hippo::readMSF (int **msgstat)
{
  tool->XTIread(msgstat, persOffset());
  return (MSF_READ | MSF_REPLIED | MSF_PERSONAL | MSF_MARKED);
}


const char *hippo::saveMSF (int **msgstat)
{
  return tool->XTIsave(msgstat, persOffset(), packetID);
}


inline const char *hippo::getPacketID () const
{
  return packetID;
}


inline bool hippo::isLatin1 ()
{
  // this is either completely right or wrong
  return isIso(bm->resourceObject->get(ConsoleCharset));
}


inline bool hippo::offlineConfig ()
{
  return true;
}


int hippo::buildIndices ()
{
  int pers = 0, aix = 0;
  HIPPO_AREA_INFO anchor, *idx = &anchor, *ai = NULL;
  MSG_NDX *mAnchor = new MSG_NDX, *ndx = mAnchor, *i, *next;
  pIDX pAnchor, *pidx = &pAnchor, *d;
  enum {nothing, Message, Info} in = nothing;
  long cmdpos = 0;

  anchor.next = NULL;

  const char *username = bm->resourceObject->get(LoginName);

  if (persArea) idx = newArea(idx);

  while (fgetsnl(line, sizeof(line), hd))
  {
    mkstr(line);

    if (strncasecmp(line, "Command: ", 9) == 0)
    {
      cmdpos = ftell(hd);

      if (strcasecmp(line + 9, "Message") == 0) in = Message;
      else if (strcasecmp(line + 9, "Info") == 0) in = Info;
      // no support for NewFiles, Bulletin, File
      else in = nothing;
    }

    else if (strncasecmp(line, "Lines: ", 7) == 0)
    {
      long lines = atol(line + 7);
      for (long j = 0; j < lines; j++) fgetsnl(line, sizeof(line), hd);
      in = nothing;
    }

    else switch (in)
    {
      case Message:
        if (strncasecmp(line, "Area: ", 6) == 0)
        {
          aix = getAreaIndex(&anchor, line + 6);

          if (aix == -1)
          {
            idx = newArea(idx);
            idx->title = strdupplus(line + 6);
            aix = maxAreas - 1;
          }

          ndx->area = aix;
          ndx->pointer = cmdpos;

          ai = getArea(&anchor, aix);
          ai->totmsgs++;

          ndx->next = new MSG_NDX;
          ndx = ndx->next;
        }
        else if (strncasecmp(line, "To: ", 4) == 0)
        {
          if (strcasecmp(line + 4, username) == 0)
              /* ||                AliasName */
          {
            pers++;
            ai->numpers++;
            pidx->next = new pIDX;
            pidx = pidx->next;
            pidx->persidx.area = aix;
            pidx->persidx.msgnum = ai->totmsgs - 1;
            pidx->next = NULL;
          }
        }
        break;

      case Info:
        if (strncasecmp(line, "System: ", 8) == 0)
          bm->resourceObject->set(BBSName, line + 8);
        else if (strncasecmp(line, "SysOp: ", 7) == 0)
          bm->resourceObject->set(SysOpName, line + 7);
        else if (!gotAddr && (strncasecmp(line, "Address: ", 9) == 0))
        {
          mainAddr.set(strtok(line + 9, " "));
          // we consider the first one as main address
          gotAddr = true;
        }
        else if (strncasecmp(line, "Area: ", 6) == 0)
        {
          aix = getAreaIndex(&anchor, line + 6);

          if (aix == -1)
          {
            idx = newArea(idx);
            idx->title = strdupplus(line + 6);
            aix = maxAreas - 1;
          }

          int flags = 0;
          size_t readlen;

          while ((readlen = fgetsnl(line, sizeof(line), hd)))
          {
            if (strncasecmp(line, "Status:", 7) == 0)   // see "else" below
            {
              bool net = strcasestr(line + 7, "net");   // Net or inet
              bool mail = strcasestr(line + 7, "Mail");
              flags |= (strcasestr(line + 7, "Must") ? A_FORCED : 0);
              flags |= (strcasestr(line + 7, "inet") ? A_INTERNET : 0);
              flags |= (mail || strcasestr(line + 7, "Private") ? A_ALLOWPRIV
                                                                : 0);
              flags |= (net && mail ? A_NETMAIL : 0);
            }
            else if (strncasecmp(line, "Access:", 7) == 0) // see "else" below
            {
              flags |= (strcasestr(line + 7, "Member") ? A_SUBSCRIBED : 0);
              flags |= (strcasestr(line + 7, "Write") ? 0 : A_READONLY);
            }
            else
            // Because we break here, we used "Status:" and "Access:" rather
            // than "Status: " and "Access: ". If these lines are empty, we
            // must not break and thus leave this loop.
            {
              fseek(hd, ftell(hd) - readlen, SEEK_SET);
              break;
            }
          }

          ai = getArea(&anchor, aix);
          ai->flags = flags;
        }
        hasInfo = true;
        break;

      default:
        ;
    }
  }

  if (maxAreas)
  {
    areas = new HIPPO_AREA_INFO *[maxAreas];
    idx = anchor.next;
    int j = 0;

    while (idx)
    {
      areas[j++] = idx;
      idx = idx->next;
    }
  }

  body = new bodytype *[maxAreas];

  for (int a = 0; a < maxAreas; a++)
  {
    body[a] = new bodytype[areas[a]->totmsgs];
    areas[a]->totmsgs = 0;
  }

  i = mAnchor;

  while (i != ndx)
  {
    next = i->next;

    int a = i->area;
    body[a][areas[a]->totmsgs++].pointer = i->pointer;

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

  if (persArea) areas[0]->totmsgs = areas[0]->numpers = pers;

  return pers;
}


hippo::HIPPO_AREA_INFO *hippo::newArea (HIPPO_AREA_INFO *idx)
{
  maxAreas++;

  idx->next = new HIPPO_AREA_INFO;
  idx = idx->next;
  idx->title = NULL;
  idx->flags = 0;
  idx->totmsgs = 0;
  idx->numpers = 0;
  idx->next = NULL;

  return idx;
}


int hippo::getAreaIndex (HIPPO_AREA_INFO *anchor, const char *title) const
{
  HIPPO_AREA_INFO *ai = anchor->next;

  // NULL is an internal pseudo-title for the personal area
  if (title == NULL) return 0;

  int a = 0;

  while (ai)
  {
    if (ai->title && (strcasecmp(ai->title, title) == 0)) break;
    ai = ai -> next;
    a++;
  }

  return (ai ? a : -1);
}


int hippo::getAreaIndex (const char *title) const
{
  int a;

  for (a = persOffset(); a < maxAreas; a++)
    if (strcasecmp(areas[a]->title, title) == 0) break;

  return (a == maxAreas ? -1 : a);
}


hippo::HIPPO_AREA_INFO *hippo::getArea (HIPPO_AREA_INFO *anchor, int num) const
{
  HIPPO_AREA_INFO *ai = anchor->next;

  while (ai)
  {
    if (num-- == 0) break;
    ai = ai -> next;
  }

  return ai;
}


const char *hippo::getAreaInfo (int *area) const
{
  int a = *area;

  *area = areas[a]->flags | (hasInfo ? 0 : A_SUBSCRIBED);
  return areas[a]->title;
}


/*
   Hippo v2 reply driver
*/

hippo_reply::hippo_reply (bmail *bm, main_driver *mainDriver)
{
  this->bm = bm;
  this->mainDriver = (hippo *) mainDriver;
  firstReply = currentReply = NULL;
}


hippo_reply::~hippo_reply ()
{
  REPLY *next, *r = firstReply;

  while (r)
  {
    next = r->next;

    delete[] r->header.to;
    delete[] r->header.subject;
    delete[] r->header.areatag;
    delete[] r->header.netAddr;
    if (r->filename) remove(r->filename);
    delete[] r->filename;

    delete r;
    r = next;
  }

  delete[] replyBody;
}


const char *hippo_reply::init ()
{
  sprintf(replyPacketName, "%.8s.hra", mainDriver->getPacketID());

  if (bm->getServiceType() != ST_REPLY) replyInf();

  return (getReplies() ? NULL : error);
}


inline area_header *hippo_reply::getNextArea ()
{
  return getReplyArea("Hippo Replies");
}


letter_header *hippo_reply::getNextLetter ()
{
  int area = mainDriver->getAreaIndex(currentReply->header.areatag);
  area += (area == -1 ? 1 : bm->driverList->getOffset(mainDriver));

  net_address na;
  na.set(currentReply->header.netAddr);

  letter_header *newLetter = new letter_header(bm, NLID, "",
                                               currentReply->header.to,
                                               currentReply->header.subject,
                                               "", NLID,
                                               currentReply->header.replyto,
                                               area,
                                               (currentReply->header.isPrivate ? PRIVATE : 0),
                                               isLatin1(), &na, NULL, NULL,
                                               NULL, this);

  NLID++;
  currentReply = currentReply->next;

  return newLetter;
}


const char *hippo_reply::getBody (int area, int letter, letter_header *lh)
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


void hippo_reply::resetLetters ()
{
  NLID = 1;
  currentReply = firstReply;
}


inline bool hippo_reply::isLatin1 ()
{
  return mainDriver->isLatin1();
}


void hippo_reply::enterLetter (letter_header *newLetter,
                               const char *newLetterFileName, long msglen)
{
  REPLY *newReply = new REPLY;

  newReply->header.to = strdupplus(newLetter->getTo());
  newReply->header.subject = strdupplus(newLetter->getSubject());
  newReply->header.replyto = newLetter->getReplyTo();
  newReply->header.areatag = strdupplus(bm->areaList->getTitle());
  newReply->header.isPrivate = newLetter->isPrivate();

  if (bm->areaList->isNetmail())
  {
    net_address na = newLetter->getNetAddr();
    newReply->header.netAddr = strdupplus(na.get(bm->areaList->isInternet()));
  }
  else newReply->header.netAddr = NULL;

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


void hippo_reply::killLetter (int letter)
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


const char *hippo_reply::newLetterBody (int letter, long size)
{
  REPLY *r = firstReply;
  for (int l = 1; l < letter; l++) r = r->next;

  if (size >= 0) r->length = size;
  return r->filename;
}


void hippo_reply::replyInf ()
{
  FILE *hd;
  char fname[MYMAXPATH];
  const char *hdfile = bm->fileList->exists(".hd");

  mkfname(fname, bm->resourceObject->get(InfWorkDir), hdfile);

  if ((hd = fopen(fname, "wt")))
  {
    fprintf(hd, "Command: Info\n");
    fprintf(hd, "System: %s\n", bm->resourceObject->get(BBSName));
    fprintf(hd, "SysOp: %s\n", bm->resourceObject->get(SysOpName));

    for (int a = mainDriver->persOffset(); a < mainDriver->getNoOfAreas(); a++)
    {
      int af = a;

      fprintf(hd, "Area: %s\n", mainDriver->getAreaInfo(&af));

      if (af & (A_FORCED | A_INTERNET | A_NETMAIL | A_ALLOWPRIV))
      {
        bool separate = false;

        bool inet = ((af & A_INTERNET) != 0);
        bool netmail = ((af & A_NETMAIL) != 0);

        fprintf(hd, "Status: ");

        if (af & A_FORCED)
        {
          fprintf(hd, "Must");
          separate = true;
        }

        if (inet)
        {
          fprintf(hd, "%sinet", (separate ? "," : ""));
          separate = true;
        }

        if (netmail && !inet)
        {
          fprintf(hd, "%sNet", (separate ? "," : ""));
          separate = true;
        }

        if (af & A_ALLOWPRIV) fprintf(hd, (netmail || inet ? "%sMail"
                                                           : "%sPrivate"),
                                          (separate ? "," : ""));

        fprintf(hd, "\n");
      }

      fprintf(hd, "Access: Read");
      if ((af & A_READONLY) == 0) fprintf(hd, ",Write");
      if (af & A_SUBSCRIBED) fprintf(hd, ",Member");
      fprintf(hd, "\n");
    }

    fprintf(hd, "Lines: 0\n");
    fclose(hd);

    bm->service->createSystemInfFile(fmtime(hdfile),
                                     mainDriver->getPacketID(),
                                     strrchr(replyPacketName, '.') + 1);
  }
}


bool hippo_reply::putReplies ()
{
  FILE *hr;
  bool success = true;

  if (!(hr = fopen(ext(replyPacketName, "hr"), "wt"))) return false;

  if (bm->areaList->areaConfig()) success = putConfig(hr);

  if (success)
  {
    REPLY *r = firstReply;

    for (int l = 0; l < noOfLetters; l++)
    {
      if (!put1Reply(hr, r))
      {
        success = false;
        break;
      }
      r = r->next;
    }
  }

  fclose(hr);
  return success;
}


bool hippo_reply::put1Reply (FILE *hr, REPLY *r)
{
  FILE *reply;
  char *replyText, *p;
  bool success;

  if (!(reply = fopen(r->filename, "rt"))) return false;

  replyText = new char[r->length + 2];   // we might need to add '\n'
  success = (fread(replyText, sizeof(char), r->length, reply) ==
                                                          (size_t) r->length);
  fclose(reply);

  if (success)
  {
    replyText[r->length] = '\0';

    if (r->length > 0 && replyText[r->length - 1] != '\n')
      strcat(replyText, "\n");

    long lines = 0;
    p = replyText;
    while (*p)
      if (*p++ == '\n') lines++;

    fprintf(hr, "Command: Post\nArea: %s\n", r->header.areatag);
    if (r->header.isPrivate) fprintf(hr, "Status: Private\n");
    if (r->header.replyto) fprintf(hr, "Reply: %d\n", r->header.replyto);
    fprintf(hr, "Subject: %s\nTo: %s\n", r->header.subject, r->header.to);
    if (r->header.netAddr) fprintf(hr, "Destination: %s\n", r->header.netAddr);
    fprintf(hr, "Lines: %ld\n", lines);
    if (fprintf(hr, "%s", replyText) < r->length) success = false;
  }

  delete[] replyText;
  return success;
}


bool hippo_reply::getReplies ()
{
  file_list *replies;
  const char *fname;
  FILE *hr;
  bool success = true;

  // Unfortunately, we cannot wait until getConfig() is called to determine
  // the joined and resigned areas, because the information is within the
  // *.hr files we are going to read now. Hence, we will store these areas
  // (i.e. the indices needed for a later bm->areaList->gotoArea() call)
  // in confArea, an integer array. A positive number is an index of a
  // joined area, a negative number a (negative) index of a resigned area.
  // The indices will be set in get1Config() and used in getConfig().
  confAreas = mainDriver->getNoOfAreas();
  confArea = new int[confAreas];
  for (int a = 0; a < confAreas; a++) confArea[a] = 0;

  if (!(replies = unpackReplies())) return true;

  REPLY seed, *r = &seed;
  seed.next = NULL;

  replies->gotoFile(-1);

  while ((fname = replies->getNext(".hr")))
  {
    if ((hr = fopen(fname, "rt")))
    {
      while (fgetsnl(line, sizeof(line), hr))
      {
        if (strncasecmp(mkstr(line), "Command: ", 9) == 0)
        {
          if (strcasecmp(line + 9, "Post") == 0)
          {
            if (!get1Reply(hr, r))
            {
              success = false;
              fclose(hr);
              goto GETREPEND;
            }
          }
          else get1Config(hr, line + 9);
        }
      }

      fclose(hr);

      if (remove(fname) != 0)
      {
        success = false;
        break;
      }
    }
    else
    {
      success = false;
      break;
    }
  }

GETREPEND:
  if (noOfLetters) firstReply = seed.next;

  delete replies;

  if (!success)
  {
    clearDirectory(".");     // clean up
    strcpy(error, "Could not get reply.");
  }

  return success;
}


bool hippo_reply::get1Reply (FILE *hr, REPLY *&r)
{
  FILE *reply;
  char filename[13], filepath[MYMAXPATH];
  static int fnr = 1;

  r->next = new REPLY;
  r = r->next;
  r->header.to = NULL;
  r->header.subject = NULL;
  r->header.replyto = 0;
  r->header.areatag = NULL;
  r->header.isPrivate = false;
  r->header.netAddr = NULL;
  r->filename = NULL;
  r->length = 0;
  r->next = NULL;

  noOfLetters++;
  bool gotLines = false;

  while (fgetsnl(line, sizeof(line), hr))
  {
    mkstr(line);

    if (strncasecmp(line, "Area: ", 6) == 0)
      r->header.areatag = strdupplus(line + 6);

    else if (strcasecmp(line, "Status: Private") == 0)
      r->header.isPrivate = true;

    else if (strncasecmp(line, "Reply: ", 7) == 0)
      r->header.replyto = atoi(line + 7);

    else if (strncasecmp(line, "Subject: ", 9) == 0)
      r->header.subject = strdupplus(line + 9);

    else if (strncasecmp(line, "To: ", 4) == 0)
      r->header.to = strdupplus(line + 4);

    else if (strncasecmp(line, "Destination: ", 13) == 0)
      r->header.netAddr = strdupplus(line + 13);

    else if (strncasecmp(line, "Lines: ", 7) == 0)
    {
      if (!r->header.subject) r->header.subject = strdupplus("<none>");
      if (!r->header.to) r->header.to = strdupplus("ALL");

      if (!r->header.areatag) return false;
      else
      {
        gotLines = true;
        break;
      }
    }
  }

  if (!gotLines) return false;

  sprintf(filename, "reply.%03d", fnr++);
  mkfname(filepath, bm->resourceObject->get(WorkDir), filename);

  if (!(reply = fopen(filepath, "wt"))) return false;

  long lines = atol(line + 7);

  while (lines)
  {
    // Must be fgets rather than fgetsnl!
    if (!fgets(line, sizeof(line), hr)) break;

    size_t len = strlen(line);
    r->length += len;

    fwrite(line, len, 1, reply);

    if (strchr(line, '\n')) lines--;
  }

  r->filename = strdupplus(filepath);
  fclose(reply);

  return true;
}


void hippo_reply::get1Config (FILE *hr, const char *cmd)
{
  int index;

  bool join = (strcasecmp(cmd, "Join") == 0);
  bool resign = (strcasecmp(cmd, "Resign") == 0);

  if (join || resign)
  {
    while (fgetsnl(line, sizeof(line), hr))
    {
      mkstr(line);

      if (strncasecmp(line, "Area: ", 6) == 0)
      {
        if ((index = mainDriver->getAreaIndex(line + 6)) != -1)
        {
          int area = index + bm->driverList->getOffset(mainDriver);

          // see comment in getReplies() above
          int a;
          for (a = 0; a < confAreas; a++)
            if (confArea[a] == 0) break;

          if (a < confAreas) confArea[a] = (join ? area : -area);
        }
      }
      else if (strncasecmp(line, "Lines: ", 7) == 0)
      {
        long lines = atol(line + 7);
        for (long j = 0; j < lines; j++) fgetsnl(line, sizeof(line), hr);
        return;
      }
    }
  }
}


void hippo_reply::getConfig ()
{
  for (int a = 0; a < confAreas; a++)
  {
    if (confArea[a] != 0)
    {
      bm->areaList->gotoArea(confArea[a] > 0 ? confArea[a] : -confArea[a]);

      if (confArea[a] > 0) bm->areaList->setAdded(true);
      else bm->areaList->setDropped(true);
    }
    else break;
  }

  delete[] confArea;
}


bool hippo_reply::putConfig (FILE *hr)
{
  bool success = true;

  int offset = bm->driverList->getOffset(mainDriver) +
               mainDriver->persOffset();

  for (int a = offset; a < bm->areaList->getNoOfAreas(); a++)
  {
    bm->areaList->gotoArea(a);

    bool join = (!bm->areaList->isSubscribed() && bm->areaList->isAdded());
    bool resign = (bm->areaList->isSubscribed() && bm->areaList->isDropped());

    if (join || resign)
    {
      const char *title = bm->areaList->getTitle();

      if (fprintf(hr, "Command: %s\nArea: %s\nLines: 0\n",
                                           join ? "Join" : "Resign", title) !=
          (int) strlen(title) + (join ? 30 : 32))
      {
        success = false;
        break;
      }
    }
  }

  return success;
}
