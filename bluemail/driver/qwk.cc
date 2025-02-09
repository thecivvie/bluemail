/*
 * blueMail offline mail reader
 * QWK / QWKE packet driver

 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <ctype.h>
#include <string.h>
#include "qwk.h"
#include "../../common/auxil.h"
#include "../../common/error.h"
#include "../../common/mysystem.h"


/*
   QWK header (to handle the header in the MESSAGES.DAT file)
*/

void qwk_header::qstrncpy (char *dest, const char *source, size_t maxlen)
{
  char *p;

  strncpy(dest, source, maxlen);
  dest[maxlen] = '\0';
  for (p = strrchr(dest, '\0') - 1; p > dest && *p == ' '; p--) ;
  *++p = '\0';
}


bool qwk_header::read (FILE *msgFile)
{
  QWK_MSGHEADER qmh;
  char buf[9];

  if (fread(&qmh, sizeof(qmh), 1, msgFile) != 1) return false;

  privat = (qmh.status == '*' || qmh.status == '+');
  skipit = (qmh.status == '\0' || qmh.status == '\xFF');

  if (skipit) return true;

  qstrncpy(from, qmh.from, 25);
  qstrncpy(to, qmh.to, 25);
  qstrncpy(subject, qmh.subject, 25);

  qstrncpy(date, qmh.date, 8);
  //date[2] = '-';
  //date[5] = '-';                // to deal with some broken messages
  strcat(date, " ");
  strncat(date, qmh.time, 5);

  qstrncpy(buf, qmh.msgnum, 7);
  msgnum = atoi(buf);

  qstrncpy(buf, qmh.refnum, 8);
  refnum = atoi(buf);

  confnum = getLSBshort(qmh.confnum);

  qstrncpy(buf, qmh.chunks, 6);
  msglen = (atoi(buf) - 1) << 7;   // 1 record is the header itself

  return true;
}


bool qwk_header::write (FILE *msgFile)
{
  QWK_MSGHEADER qmh;
  char buf[9];
  int len;
  long chunks;

  memset(&qmh, ' ', sizeof(qmh));

  if (privat) qmh.status = '*';

  sprintf(buf, " %-6d", confnum);
  strncpy(qmh.msgnum, buf, 7);     // is confnum in replies

  strncpy(qmh.date, date, 8);
  strncpy(qmh.time, &date[9], 5);

  len = strlen(to);
  strncpy(qmh.to, to, (len < 25 ? len : 25));

  len = strlen(from);
  strncpy(qmh.from, from, (len < 25 ? len : 25));

  len = strlen(subject);
  strncpy(qmh.subject, subject, (len < 25 ? len : 25));

  if (refnum)
  {
    sprintf(buf, " %-7d", refnum);
    strncpy(qmh.refnum, buf, 8);
  }

  chunks = ((msglen + MSGDAT_RECLEN - 1) >> 7) + 1;     // 1 record for header
  sprintf(buf, "%-6ld", chunks);
  strncpy(qmh.chunks, buf, 6);

  qmh.alive = (char) 0xE1;     // message is active

  putLSBshort(qmh.confnum, confnum);

  return (fwrite(&qmh, sizeof(qmh), 1, msgFile) == 1);
}


/*
   QWK / QWKE main driver
*/

qwk::qwk (bmail *bm)
{
  this->bm = bm;
  tool = new basic_tool(bm, this);

  areas = NULL;

  const char *pa = bm->resourceObject->get(PersonalArea);
  bool pndx = (bm->fileList->exists("personal.ndx") != NULL);
  if (bm->getServiceType() == ST_REPLY) pa = "N";

  persArea = (pa && (toupper(*pa) == 'Y') ? true : (pa ? false : pndx));
  qwke = (bm->fileList->exists("toreader.ext") != NULL);

  body = NULL;
  msgFile = NULL;
}


qwk::~qwk ()
{
  while (maxAreas--)
  {
    delete[] body[maxAreas];
    delete[] areas[maxAreas].title;
  }

  delete[] bulletins;
  delete[] letterBody;
  delete[] body;
  delete[] areas;
  delete tool;

  if (msgFile) fclose(msgFile);
}


const char *qwk::init ()
{
  if (!readControl()) return error;

  body = new bodytype *[maxAreas];
  for (int i = 0; i < maxAreas; i++) body[i] = NULL;

  if (qwke)
  {
    offcfg = true;
    if (!readExtInformation()) return error;
  }
  else offcfg = readDoorId();

  if (!(msgFile = openFile("messages.dat"))) return error;

  readIndices();

  return NULL;
}


area_header *qwk::getNextArea ()
{
  area_header *ah;

  bool isPersArea = (persArea && (NAID == 0));

  int maxfromto = (qwke ? QWKE_MAXFROMTO : 25);
  int maxsubject = (qwke ? QWKE_MAXSUBJECT : 25);

  int flags = areas[NAID].flags;
  if (!qwke) flags |= (areas[NAID].totmsgs ? A_SUBSCRIBED : 0);

  ah = new area_header(bm, NAID + bm->driverList->getOffset(this),
                       (isPersArea ? "PERS" : areas[NAID].areanum),
                       (isPersArea ? "PERSONAL" : areas[NAID].title),
                       (isPersArea ? "Letters addressed to you"
                                   : areas[NAID].title),
                       (isPersArea ? (qwke ? "QWKE Personal" : "QWK Personal")
                                   : (qwke ? "QWKE" :"QWK")),
                       (isPersArea ? A_COLLECTION | A_READONLY | A_LIST
                                   : flags),
                       areas[NAID].totmsgs, 0, maxfromto, maxsubject);
  NAID++;

  return ah;
}


inline int qwk::getNoOfLetters ()
{
  return areas[currentArea].totmsgs;
}


letter_header *qwk::getNextLetter ()
{
  int area, letter;
  qwk_header qh;

  fseek(msgFile, body[currentArea][NLID].pointer - MSGDAT_RECLEN, SEEK_SET);
  if (!qh.read(msgFile)) fatalError("Could not get next letter.");

  if (persArea && (currentArea == 0))
  {
    area = getAreaIndex(qh.confnum);
    letter = getLetterIndex(area, body[currentArea][NLID].pointer);
  }
  else
  {
    area = currentArea;
    letter = NLID;
  }

  body[area][letter].msglen = qh.msglen;

  NLID++;
  return new letter_header(bm, letter,
                           qh.from, qh.to, qh.subject, qh.date,
                           qh.msgnum, qh.refnum, area,
                           (qh.privat ? PRIVATE : 0),
                           ((areas[area].flags & A_CHRSLATIN1) != 0),
                           NULL, NULL, NULL, NULL, this);
}


const char *qwk::getBody (int area, int letter, letter_header *lh)
{
  int kar;
  bool noSoftCR = bm->resourceObject->isYes(StripSoftCR);

  delete[] letterBody;
  letterBody = new char[body[area][letter].msglen + 1];
  unsigned char *p = (unsigned char *) letterBody;
  fseek(msgFile, body[area][letter].pointer, SEEK_SET);

  for (long c = 0; c < body[area][letter].msglen; c++)
  {
    kar = fgetc(msgFile);

    // a message shouldn't (must not) contain a 0x00 character
    if (kar == 0) kar = ' ';

    // strip soft CR
    if (noSoftCR && (kar == SOFT_CR)) continue;

    // end of line?
    if (kar == 227) *p++ = '\n';
    else if (kar != '\r') *p++ = kar;
  }

  do p--;
  // strip blank lines
  while (p >= (unsigned char *) letterBody && (*p == ' ' || *p == '\n'));

  *++p = '\0';

  // determine charset
  bool latin1 = lh->isLatin1();
  tool->fsc0054(letterBody, '\1', &latin1);
  lh->setLatin1(latin1);

  // get netmail address from echomail (for addressbook entry etc. only)
  net_address &na = lh->getNetAddr();
  char *addr = tool->getFidoAddr(letterBody);
  if (addr) na.set(addr);

  // decode quoted-printable
  if (bm->driverList->allowQuotedPrintable() && tool->isQP(letterBody))
    tool->mimeDecodeBody(letterBody);

  // decode UTF-8
  if (tool->isUTF8(letterBody)) tool->utf8DecodeBody(letterBody);

  char *body = letterBody;

  if (qwke)
  {
    bool isEmail = false, QWKEKludge;
    char *p;
    char kline[QWKE_MAXSUBJECT + 1];

    // handle netmail

    if (areas[area].flags & A_NETMAIL)
    {
      if (areas[area].flags & A_INTERNET) isEmail = true;
      else if (*lh->getSubject() == '@') na.set(lh->getSubject() + 1);
    }

    // now analyse the body

    do
    {
      QWKEKludge = false;

      if ((p = isQWKEKludge("To:", body, kline, QWKE_MAXFROMTO)))
      {
        body = p;
        lh->setTo(kline);
        QWKEKludge = true;

        if (isEmail)
        {
          na.set(kline);
          lh->setTo("INTERNET");
        }
      }

      if ((p = isQWKEKludge("From:", body, kline, QWKE_MAXFROMTO)))
      {
        body = p;
        lh->setFrom(kline);
        QWKEKludge = true;
      }

      if ((p = isQWKEKludge("Subject:", body, kline, QWKE_MAXSUBJECT)))
      {
        body = p;
        lh->setSubject(kline);
        QWKEKludge = true;
      }
    }
    while (QWKEKludge);
  }

  if (body != letterBody && *body == '\n') body++;

  return body;
}


void qwk::initMSF (int **msgstat)
{
  tool->MSFinit(msgstat, persOffset(), NULL, 0);
}


int qwk::readMSF (int **msgstat)
{
  tool->XTIread(msgstat, persOffset());
  return (MSF_READ | MSF_REPLIED | MSF_PERSONAL | MSF_MARKED);
}


const char *qwk::saveMSF (int **msgstat)
{
  return tool->XTIsave(msgstat, persOffset(), BBSid);
}


inline const char *qwk::getPacketID () const
{
  return BBSid;
}


inline bool qwk::isLatin1 ()
{
  return false;
}


inline bool qwk::offlineConfig ()
{
  return offcfg;
}


bool qwk::readControl ()
{
  char *l, *p;

  if (!(ctrlFile = openFile("control.dat"))) return false;

  bm->resourceObject->set(BBSName, nextLine());     // 1: BBS name
  nextLine();                                       // 2: BBS city and state
  nextLine();                                       // 3: BBS phone number

  l = nextLine();                                   // 4: BBS Sysop name
  int slen = strlen(l);

  if (slen > 6)   // common format is: name, Sysop
  {
    p = l + slen - 5;

    if (strcasecmp(p, "Sysop") == 0)
    {
      if (*--p == ' ') p--;
      if (*p == ',') *p = '\0';
    }
  }

  bm->resourceObject->set(SysOpName, l);

  l = nextLine();                                   // 5: registration #,BBSid
  p = strtokr(l, ',');
  strncpy(BBSid, p, 8);
  BBSid[8] = '\0';

  nextLine();                                       // 6: packet creation time
  l = nextLine();                                   // 7: user name
  bm->resourceObject->set(LoginName, l);
  bm->resourceObject->set(AliasName, l);
  bm->resourceObject->set(hasLoginName, "Y");

  nextLine();                                       // 8: blank
  nextLine();                                       // 9: zero
  nextLine();                                       // 10: total # of messages
                                                    //     (not reliable)
  maxAreas = atoi(nextLine()) + (persArea ? 2 : 1); // 11: total # of confer-
                                                    //     ences minus 1
  areas = new QWK_AREA_INFO[maxAreas];

  for (int a = persOffset(); a < maxAreas; a++)
  {
    areas[a].number = atoi(nextLine());             // 12: conference number
    sprintf(areas[a].areanum, "%d", areas[a].number);
    areas[a].title = strdupplus(nextLine());        // 13: conference name
    areas[a].flags = (qwke ? 0 : A_LIST | A_ALLOWPRIV);
  }

  if (persArea) areas[0].title = NULL;

  slen = sizeof(bulletinfiles[0]) - 1;

  for (int i = 0; i < 3; i++)
  {
    strncpy(bulletinfiles[i], nextLine(), slen);
    bulletinfiles[i][slen] = '\0';
  }

  bulletins = tool->bulletin_list(bulletinfiles, 3, BL_NEWFILES | BL_QWK);

  fclose(ctrlFile);
  return true;
}


FILE *qwk::openFile (const char *fname)
{
  FILE *file;

  if (!(file = bm->fileList->ftryopen(fname, "rb")))
    sprintf(error, "Could not open %s file.", fname);

  return file;
}


char *qwk::nextLine ()
{
  static char line[MYMAXLINE];

  if (!fgetsnl(line, sizeof(line), ctrlFile)) *line = '\0';

  return mkstr(line);
}


bool qwk::readExtInformation ()
{
  char *line;
  int num;

  if (!(ctrlFile = openFile("toreader.ext"))) return false;

  while (!feof(ctrlFile))
  {
    line = nextLine();

    if (strncasecmp(line, "AREA ", 5) == 0)
    {
      if (sscanf(line + 5, "%d %s", &num, line) == 2)
      {
        if (strchr(line, 'a') || strchr(line, 'p') || strchr(line, 'g'))
          areas[getAreaIndex(num)].flags |= A_SUBSCRIBED;

        if (strchr(line, 'F')) areas[getAreaIndex(num)].flags |= A_FORCED;

        if (strchr(line, 'N') ||
            strchr(line, 'I')) areas[getAreaIndex(num)].flags |= A_NETMAIL;

        if (strchr(line, 'I') ||
            strchr(line, 'U')) areas[getAreaIndex(num)].flags |= A_INTERNET |
                                                                 A_CHRSLATIN1;
        if (strchr(line, 'R')) areas[getAreaIndex(num)].flags |= A_READONLY;

        if (strchr(line, 'H')) areas[getAreaIndex(num)].flags |= A_ALIAS;

        if (strchr(line, 'P') ||
            strchr(line, 'X')) areas[getAreaIndex(num)].flags |= A_ALLOWPRIV;
      }
    }

    else if (strncasecmp(line, "ALIAS ", 6) == 0)
    {
      cropesp(line);
      bm->resourceObject->set(AliasName, line + 6);
    }
  }

  fclose(ctrlFile);
  return true;
}


bool qwk::readDoorId ()
{
  char *line;
  bool add = false, drop = false;

  *ctrlname = '\0';

  if ((ctrlFile = bm->fileList->ftryopen("door.id", "rb")))
  {
    while (!feof(ctrlFile))
    {
      line = nextLine();

      if (strncasecmp(line, "CONTROLNAME = ", 14) == 0)
        sprintf(ctrlname, "%.25s", line + 14);
      else if (strcasecmp(line, "CONTROLTYPE = ADD") == 0) add = true;
      else if (strcasecmp(line, "CONTROLTYPE = DROP") == 0) drop = true;
    }

    fclose(ctrlFile);
  }

  return (add && drop && *ctrlname);
}


bool qwk::hasNdx ()
{
  int a, msgs;
  const char *fname;
  bool success;

  success = (bm->fileList->exists(".ndx") != NULL);

  if (success)   // a first index file exists
  {
    char aname[9];
    FILE *ndxFile;
    struct
    {
      unsigned char recnum[4];
      unsigned char confnum;
    } ndx_rec;

    fseek(msgFile, 0, SEEK_END);
    long maxpointer = ftell(msgFile);
    if (maxpointer > 128) maxpointer -= 128;

    bm->fileList->gotoFile(-1);

    while (success && (fname = bm->fileList->getNext(".ndx")))
    {
      msgs = 0;
      int len = strlen(fname) - 4;

      strncpy(aname, fname, len);
      aname[len] = '\0';
      a = atoi(aname);

      if (a == 0)
      {
        if (strcasecmp(aname, "personal") == 0) a = -1;
        else if (strcmp(aname, "000") != 0)   // invalid name
        {
          success = false;
          break;
        }
      }

      a = getAreaIndex(a);

      if ((ndxFile = bm->fileList->ftryopen(fname, "rb")))
      {
        msgs = bm->fileList->getSize() / NDX_RECLEN;
        body[a] = new bodytype[msgs];

        for (int l = 0; l < msgs; l++)
        {
          if (fread(&ndx_rec, NDX_RECLEN, 1, ndxFile) != 1)
          {
            success = false;
            break;
          }

          body[a][l].pointer = getMKSlong(ndx_rec.recnum) << 7;

          if (body[a][l].pointer < 256 || body[a][l].pointer > maxpointer)
          // index appears to be corrupt, so don't use the index files
          {
            success = false;
            break;
          }
        }

        fclose(ndxFile);
        areas[a].totmsgs = msgs;
      }
    }
  }

  if (!success)
    // clean up
    for (a = 0; a < maxAreas; a++)
    {
      delete[] body[a];
      body[a] = NULL;
      areas[a].totmsgs = 0;
    }

  return success;
}


void qwk::readIndices ()
{
  int a;

  for (a = 0; a < maxAreas; a++)
  {
    body[a] = NULL;
    areas[a].totmsgs = 0;
  }

  if (bm->resourceObject->isYes(IgnoreNDX) || !hasNdx())
  // build own index
  {
    MSG_NDX *anchor = new MSG_NDX, *ndx = anchor, *i, *next;
    qwk_header qh;

    const char *username = bm->resourceObject->get(LoginName);
    const char *aliasname = bm->resourceObject->get(AliasName);

    fseek(msgFile, MSGDAT_RECLEN, SEEK_SET);

    while (qh.read(msgFile))
    {
      if (!qh.skipit)
      {
        ndx->area = getAreaIndex(qh.confnum);
        ndx->pointer = ftell(msgFile);
        areas[ndx->area].totmsgs++;

        ndx->next = new MSG_NDX;
        ndx = ndx->next;

        if (persArea && (strcasecmp(qh.to, username) == 0 ||
                         strcasecmp(qh.to, aliasname) == 0))
        {
          ndx->area = 0;
          ndx->pointer = ftell(msgFile);
          areas[0].totmsgs++;

          ndx->next = new MSG_NDX;
          ndx = ndx->next;
        }

        fseek(msgFile, qh.msglen, SEEK_CUR);
      }
    }

    for (a = 0; a < maxAreas; a++)
    {
      body[a] = new bodytype[areas[a].totmsgs];
      areas[a].totmsgs = 0;
    }

    i = anchor;

    while (i != ndx)
    {
      next = i->next;

      a = i->area;
      body[a][areas[a].totmsgs++].pointer = i->pointer;

      delete i;
      i = next;
    }

    delete ndx;
  }
}


int qwk::getAreaIndex (int area) const
{
  int a;

  // -1 is an internal pseudo-number for the personal area
  if (area == -1) return 0;

  for (a = persOffset(); a < maxAreas; a++)
    if (areas[a].number == area) break;

  return (a == maxAreas ? -1 : a);
}


int qwk::getLetterIndex (int area, long pos) const
{
  int l;

  for (l = 0; l < areas[area].totmsgs; l++)
    if (body[area][l].pointer == pos) break;

  return l;
}


bool qwk::isQWKE () const
{
  return qwke;
}


char *qwk::isQWKEKludge (const char *kludge, char *source,
                         char *dest, int destlen) const
{
  int len = strlen(kludge), i;

  if (strncmp(source, kludge, len) == 0)
  {
    source += len;
    while (*source == ' ') source++;

    for (i = 0; *source && (*source != '\n') && (i < destlen); i++)
      dest[i] = *source++;
    dest[i] = '\0';

    while (*source && (*source != '\n')) source++;

    return (*source ? ++source : source);
  }
  else return NULL;
}


inline const char *qwk::getCtrlname () const
{
  return ctrlname;
}


/*
   QWK / QWKE reply driver
*/

qwk_reply::qwk_reply (bmail *bm, main_driver *mainDriver)
{
  this->bm = bm;
  this->mainDriver = (qwk *) mainDriver;
  firstReply = currentReply = NULL;
}


qwk_reply::~qwk_reply ()
{
  REPLY *next, *r = firstReply;

  while (r)
  {
    next = r->next;
    if (r->filename) remove(r->filename);
    delete[] r->filename;
    delete[] r->address;
    delete r;
    r = next;
  }

  delete[] replyBody;
}


const char *qwk_reply::init ()
{
  sprintf(replyPacketName, "%.8s.rep", mainDriver->getPacketID());

  if (bm->getServiceType() != ST_REPLY) replyInf();

  return (getReplies() ? NULL : error);
}


inline area_header *qwk_reply::getNextArea ()
{
  return getReplyArea(mainDriver->isQWKE() ? "QWKE Replies" : "QWK Replies");
}


letter_header *qwk_reply::getNextLetter ()
{
  int area = mainDriver->getAreaIndex(currentReply->qh.confnum);
  area += (area == -1 ? 1 : bm->driverList->getOffset(mainDriver));

  // this is for QWKE only
  net_address na;
  if (currentReply->address) na.set(currentReply->address);

  letter_header *newLetter = new letter_header(bm, NLID,
                                               currentReply->qh.from,
                                               currentReply->qh.to,
                                               currentReply->qh.subject,
                                               currentReply->qh.date,
                                               NLID,
                                               currentReply->qh.refnum,
                                               area,
                                               (currentReply->qh.privat ? PRIVATE : 0),
                                               bm->areaList->isLatin1(area),
                                               (currentReply->address ? &na : NULL),
                                               NULL, NULL, NULL, this);

  NLID++;
  currentReply = currentReply->next;

  return newLetter;
}


const char *qwk_reply::getBody (int area, int letter, letter_header *lh)
{
  FILE *replyFile;

  (void) area;
  delete[] replyBody;

  REPLY *r = firstReply;
  for (int l = 1; l < letter; l++) r = r->next;

  if ((replyFile = fopen(r->filename, "rt")))
  {
    long msglen = r->qh.msglen;
    replyBody = new char[msglen + 1];
    msglen = fread(replyBody, sizeof(char), msglen, replyFile);
    if (msglen && (replyBody[msglen - 1] == '\n')) msglen--;
    replyBody[msglen] = '\0';

    // will anyone need this, i.e. use a charset kludge in replies?
    bool latin1 = lh->isLatin1();
    mainDriver->tool->fsc0054(replyBody, '\1', &latin1);
    lh->setLatin1(latin1);

    fclose(replyFile);
  }
  else replyBody = strdupplus("");

  return replyBody;
}


void qwk_reply::resetLetters ()
{
  NLID = 1;
  currentReply = firstReply;
}


inline bool qwk_reply::isLatin1 ()
{
  return mainDriver->isLatin1();
}


void qwk_reply::enterLetter (letter_header *newLetter,
                             const char *newLetterFileName, long msglen)
{
  static char fmt[] = "%m-%d-%y %H:%M";     // to avoid warning for %y

  REPLY *newReply = new REPLY;
  memset(newReply, 0, sizeof(REPLY));

  strncpy(newReply->qh.from, newLetter->getFrom(), QWKE_MAXFROMTO);
  strncpy(newReply->qh.to, newLetter->getTo(), QWKE_MAXFROMTO);
  strncpy(newReply->qh.subject, newLetter->getSubject(), QWKE_MAXSUBJECT);

  time_t now = time(NULL);
  strftime(newReply->qh.date, sizeof(newReply->qh.date), fmt, localtime(&now));

  newReply->qh.refnum = newLetter->getReplyTo();
  newReply->qh.confnum = atoi(bm->areaList->getNumber());
  newReply->qh.msglen = msglen;
  newReply->qh.privat = newLetter->isPrivate();

  newReply->address =
    (bm->areaList->isNetmail() ? strdupplus(newLetter->getNetAddr().get(bm->areaList->isInternet()))
                               : NULL);

  newReply->filename = strdupplus(newLetterFileName);
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


void qwk_reply::killLetter (int letter)
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
  delete[] killReply->address;
  delete killReply;

  noOfLetters--;
  resetLetters();
}


const char *qwk_reply::newLetterBody (int letter, long size)
{
  REPLY *r = firstReply;
  for (int l = 1; l < letter; l++) r = r->next;

  if (size >= 0) r->qh.msglen = size;
  return r->filename;
}


void qwk_reply::replyInf ()
{
  char fname[MYMAXPATH];
  const char *dir = bm->resourceObject->get(InfWorkDir);
  const char *ctrl = "control.dat";
  const char *toreader = "toreader.ext";
  const char *doorid = bm->fileList->exists("door.id");

  mkfname(fname, dir, ctrl);
  fcopy(ctrl, fname);

  if (mainDriver->isQWKE())
  {
    mkfname(fname, dir, toreader);
    fcopy(toreader, fname);
  }

  if (doorid)
  {
    mkfname(fname, dir, doorid);
    fcopy(doorid, fname);
  }

  fcreate(dir, "messages.dat");

  bm->service->createSystemInfFile(fmtime(ctrl),
                                   mainDriver->getPacketID(),
                                   strrchr(replyPacketName, '.') + 1);
}


bool qwk_reply::putReplies ()
{
  char msgFile[13];
  FILE *msg;
  char buf[MSGDAT_RECLEN];
  bool success = true;

  if (!mainDriver->isQWKE() || bm->areaList->areaConfig())
    if (!putConfig()) return false;

  sprintf(msgFile, "%.8s.msg", mainDriver->getPacketID());

  if (!(msg = fopen(msgFile, "wb"))) return false;

  sprintf(buf, "%-128s", mainDriver->getPacketID());
  if (fwrite(buf, MSGDAT_RECLEN, 1, msg) != 1)
  {
    fclose(msg);
    return false;
  }

  REPLY *r = firstReply;

  for (int l = 0; l < noOfLetters; l++)
  {
    if (!put1Reply(msg, r))
    {
      success = false;
      break;
    }
    r = r->next;
  }

  fclose(msg);

  return success;
}


bool qwk_reply::put1Reply (FILE *msg, REPLY *r)
{
  bool isFido = false;
  net_address na;
  char *QWKEKludge = NULL;
  long extralen = 0, chunks;
  FILE *reply;
  char *replyText, *p, *oldsubj = NULL;
  bool success;

  if (mainDriver->isQWKE())
  {
    // netmail
    if (r->address)
    {
      na.set(r->address);
      isFido = (na.get() != NULL);

      if (!isFido)
      {
        strncpy(r->qh.to, na.get(true), QWKE_MAXFROMTO);
        r->qh.to[QWKE_MAXFROMTO] = '\0';
      }
    }

    long tolen = strlen(r->qh.to);
    bool tolong = ((tolen > 25) || (r->address && !isFido));

    long fromlen = strlen(r->qh.from);
    bool fromlong = (fromlen > 25);

    long subjlen = strlen(r->qh.subject);
    bool subjlong = ((subjlen > 25) || (r->address && isFido));

    if (tolong) extralen += 4 + tolen + 1;
    if (fromlong) extralen += 6 + fromlen + 1;
    if (subjlong) extralen += 9 + subjlen + 1;

    if (extralen > 0)
    {
      extralen++;
      QWKEKludge = new char[extralen + 1];
      *QWKEKludge = '\0';

      if (tolong)
      {
        strcat(QWKEKludge, "To: ");
        strcat(QWKEKludge, r->qh.to);
        strcat(QWKEKludge, "\n");
      }

      if (fromlong)
      {
        strcat(QWKEKludge, "From: ");
        strcat(QWKEKludge, r->qh.from);
        strcat(QWKEKludge, "\n");
      }

      if (subjlong)
      {
        strcat(QWKEKludge, "Subject: ");
        strcat(QWKEKludge, r->qh.subject);
        strcat(QWKEKludge, "\n");
      }

      strcat(QWKEKludge, "\n");
    }
  }

  if (r->address)
  {
    if (isFido)
    {
      oldsubj = strdupplus(r->qh.subject);
      sprintf(r->qh.subject, "@%s", na.get());
    }
    else strcpy(r->qh.to, "INTERNET");
  }

  r->qh.msglen += extralen;
  success = r->qh.write(msg);
  r->qh.msglen -= extralen;

  if (!success)
  {
    if (oldsubj) strcpy(r->qh.subject, oldsubj);
    delete[] oldsubj;
    return false;
  }

  if ((chunks = (r->qh.msglen + extralen + MSGDAT_RECLEN - 1) >> 7) == 0)
    return true;

  if (!(reply = fopen(r->filename, "rt"))) return false;

  replyText = new char[(chunks << 7) + 1];
  memset(&replyText[(chunks - 1) << 7], ' ', MSGDAT_RECLEN);

  if (QWKEKludge) strcpy(replyText, QWKEKludge);

  success = (fread(replyText + extralen, sizeof(char), r->qh.msglen, reply) ==
                                                       (size_t) r->qh.msglen);
  fclose(reply);

  if (success)
  {
    replyText[r->qh.msglen + extralen] = '\0';

    for (p = replyText; *p; p++)
      if (*p == '\n') *p = (char) 227;

    p--;
    replyText[r->qh.msglen + extralen] = (*p == (char) 227 ? ' ' : (char) 227);

    success = (fwrite(replyText, MSGDAT_RECLEN, chunks, msg) ==
                                                             (size_t) chunks);
  }

  delete[] oldsubj;
  delete[] QWKEKludge;
  delete[] replyText;
  return success;
}


bool qwk_reply::getReplies ()
{
  file_list *replies;
  const char *msgFile = NULL;
  FILE *msg = NULL;
  bool success = true;

  if (!(replies = unpackReplies())) return true;

  if (!(msgFile = replies->exists(".msg")) ||
      !(msg = replies->ftryopen(msgFile, "rb")))
  {
    strcpy(error, "Could not open reply file.");
    success = false;
  }
  else
  {
    off_t filesize = fsize(msgFile);
    fseek(msg, MSGDAT_RECLEN, SEEK_SET);     // skip header

    REPLY seed, *r = &seed;
    seed.next = NULL;

    while (ftell(msg) < filesize)
    {
      r->next = new REPLY;
      r = r->next;
      r->filename = NULL;
      r->address = NULL;
      r->next = NULL;

      if (!get1Reply(msg, r))
      {
        strcpy(error, "Could not get reply.");
        success = false;
        break;
      }

      noOfLetters++;
    }

    firstReply = seed.next;
  }

  if (msg) fclose(msg);
  if (msgFile) remove(msgFile);
  delete replies;
  if (!success) clearDirectory(".");     // clean up

  return success;
}


bool qwk_reply::get1Reply (FILE *msg, REPLY *r)
{
  FILE *reply;
  char filename[13], filepath[MYMAXPATH];
  static int fnr = 1;
  char *replyText, *p;
  bool success;

  if (!r->qh.read(msg)) return false;

  sprintf(filename, "%d.%03d", r->qh.confnum, fnr++);
  mkfname(filepath, bm->resourceObject->get(WorkDir), filename);

  if (!(reply = fopen(filepath, "wt"))) return false;

  replyText = new char[r->qh.msglen + 1];
  success = (fread(replyText, sizeof(char), r->qh.msglen, msg) ==
                                                       (size_t) r->qh.msglen);

  if (success)
  {
    p = &replyText[r->qh.msglen - 1];
    while (p > replyText && (*p == ' ' || *p == (char) 227)) p--;
    *++p = '\0';

    for (p = replyText; *p; p++)
      if (*p == (char) 227) *p = '\n';

    *p = '\n';
    r->qh.msglen = p - replyText + 1;

    char *text = replyText;

    if (mainDriver->isQWKE())
    {
      char hfield[QWKE_MAXSUBJECT + 1];
      bool QWKEKludge;
      char *p;

      // try to figure out whether it may be netmail

      strcpy(hfield, r->qh.to);
      cropesp(hfield);
      bool isEmail = (strcasecmp(hfield, "INTERNET") == 0);

      strcpy(hfield, r->qh.subject);
      cropesp(hfield);
      if (*hfield == '@') r->address = strdupplus(hfield + 1);

      // now analyse the body

      do
      {
        QWKEKludge = false;

        if ((p = mainDriver->isQWKEKludge("To:", text, r->qh.to,
                                                       QWKE_MAXFROMTO)))
        {
          r->qh.msglen -= p - text;
          text = p;
          QWKEKludge = true;

          if (isEmail)
          {
            r->address = strdupplus(r->qh.to);
            strcpy(r->qh.to, "INTERNET");
          }
        }

        if ((p = mainDriver->isQWKEKludge("From:", text, r->qh.from,
                                                         QWKE_MAXFROMTO)))
        {
          r->qh.msglen -= p - text;
          text = p;
          QWKEKludge = true;
        }

        if ((p = mainDriver->isQWKEKludge("Subject:", text, r->qh.subject,
                                                            QWKE_MAXSUBJECT)))
        {
          r->qh.msglen -= p - text;
          text = p;
          QWKEKludge = true;
        }
      }
      while (QWKEKludge);
    }

    if (text != replyText && *text == '\n')
    {
      text++;
      r->qh.msglen--;
    }

    success = (fwrite(text, sizeof(char), r->qh.msglen, reply) ==
                                                       (size_t) r->qh.msglen);
    r->filename = strdupplus(filepath);
  }

  fclose(reply);
  delete[] replyText;
  return success;
}


void qwk_reply::getConfig ()
{
  FILE *cfg;
  char buffer[MYMAXLINE];
  int num, index;

  file_list *replies = new file_list(bm->resourceObject->get(ReplyWorkDir));

  if (mainDriver->isQWKE())
  {
    const char *cfgFile = replies->exists("todoor.ext");

    if ((cfg = replies->ftryopen(cfgFile, "rt")))
    {
      while (fgetsnl(buffer, sizeof(buffer), cfg))
      {
        if (sscanf(mkstr(buffer), "AREA %d", &num) == 1)
        {
          if ((index = mainDriver->getAreaIndex(num)) != -1)
          {
            bm->areaList->gotoArea(index +
                                   bm->driverList->getOffset(mainDriver));
            if (strchr(buffer + 6, 'D')) bm->areaList->setDropped(true);
            else bm->areaList->setAdded(true);
          }
        }
      }

      fclose(cfg);
      remove(cfgFile);
    }
  }
  else
  {
    REPLY *next, *r = firstReply;
    int index = -1;

    while (r)
    {
      next = r->next;

      if (strcasecmp(r->qh.to, mainDriver->getCtrlname()) == 0 &&
          (index = mainDriver->getAreaIndex(r->qh.confnum)) != -1)
      {
        bm->areaList->gotoArea(index +
                               bm->driverList->getOffset(mainDriver));
        if (strcasecmp(r->qh.subject, "ADD") == 0)
          bm->areaList->setAdded(true);
        else if (strcasecmp(r->qh.subject, "DROP") == 0)
          bm->areaList->setDropped(true);
      }

      r = next;
    }
  }

  delete replies;
}


bool qwk_reply::putConfig ()
{
  FILE *ext = NULL;
  static char extName[] = "TODOOR.EXT";
  const char *username = NULL, *ctrlname = NULL;
  int index;
  letter_header *ctrlMsg;
  bool success = true;

  bool qwke = mainDriver->isQWKE();

  if (qwke)
  {
    if (!(ext = fopen(extName, "wb"))) return false;
  }
  else
  {
    username = bm->resourceObject->get(LoginName);
    ctrlname = mainDriver->getCtrlname();

    bool ctrlMsgs = true;

    while (ctrlMsgs)
    {
      REPLY *next, *r = firstReply;
      int letter = 0;

      while (r)
      {
        letter++;
        next = r->next;

        if (strcasecmp(r->qh.from, username) == 0 &&
            strcasecmp(r->qh.to, ctrlname) == 0 &&
            (strcasecmp(r->qh.subject, "ADD") == 0 ||
             strcasecmp(r->qh.subject, "DROP") == 0))
        {
          killLetter(letter);
          break;
        }

        r = next;
      }

      ctrlMsgs = (r != NULL);
    }
  }

  int offset = bm->driverList->getOffset(mainDriver) +
               mainDriver->persOffset();

  for (int a = offset; a < bm->areaList->getNoOfAreas(); a++)
  {
    bm->areaList->gotoArea(a);

    bool add = (!bm->areaList->isSubscribed() && bm->areaList->isAdded());
    bool drop = (bm->areaList->isSubscribed() && bm->areaList->isDropped());

    if (add || drop)
    {
      if (qwke)
      {
        const char *areaNo = bm->areaList->getNumber();

        if (fprintf(ext, "AREA %s %c\r\n", areaNo, (drop ? 'D' : 'a')) !=
            (int) strlen(areaNo) + 9)
        {
          success = false;
          break;
        }
      }
      else
      {
        if ((index = mainDriver->getAreaIndex(a)) != -1)
        {
          ctrlMsg = new letter_header(bm, 0, username, ctrlname,
                                      (add ? "ADD" : "DROP"), "", 0, 0,
                                      index +
                                      bm->driverList->getOffset(mainDriver),
                                      0, isLatin1(), NULL, NULL, NULL, NULL,
                                      this);
          enterLetter(ctrlMsg, "", 0);
          delete ctrlMsg;
        }
      }
    }
  }

  if (ext) fclose(ext);
  if (!qwke) bm->areaList->refreshReplyArea();
  if (!success) remove(extName);

  return success;
}
