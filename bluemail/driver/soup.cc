/*
 * blueMail offline mail reader
 * SOUP driver

 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <ctype.h>
#include <string.h>
#include "soup.h"
#include "../../common/auxil.h"
#include "../../common/error.h"
#include "../../common/mysystem.h"


/*
   SOUP main driver
*/

soup::soup (bmail *bm)
{
  this->bm = bm;
  tool = new basic_tool(bm, this);

  areas = NULL;
  headerlines = NULL;
  persIDX = NULL;
  persmsgs = 0;

  persArea = bm->resourceObject->isYes(PersonalArea);
  if (bm->getServiceType() == ST_REPLY) persArea = false;

  body = NULL;
  msgFile = NULL;
}


soup::~soup ()
{
  while (maxAreas--)
  {
    delete[] body[maxAreas];
    delete[] headerlines[maxAreas];
    delete[] areas[maxAreas]->prefix;
    delete[] areas[maxAreas]->title;
    delete areas[maxAreas];
  }

  delete[] bulletins;
  delete[] letterBody;
  delete[] body;
  delete[] persIDX;
  delete[] headerlines;
  delete[] areas;
  delete tool;

  if (msgFile) fclose(msgFile);
}


const char *soup::init ()
{
  const char *username = bm->resourceObject->get(UserName);
  bm->resourceObject->set(LoginName, username);
  bm->resourceObject->set(AliasName, username);
  bm->resourceObject->set(SysOpName, "");
  bm->resourceObject->set(BBSName, "Usenet offline");
  bm->resourceObject->set(hasLoginName, "N");

  offcfg = readCommands();

  bool areas_ok = getAreas();

  body = new bodytype *[maxAreas];
  headerlines = new long *[maxAreas];
  for (int i = 0; i < maxAreas; i++)
  {
    body[i] = NULL;
    headerlines[i] = NULL;
  }

  if (!areas_ok || ((persmsgs = buildIndices()) == -1)) return error;

  if (bm->getServiceType() != ST_REPLY)
  {
    const char bulletinfile[2][13] = {"INFO", "LIST"};
    bulletins = tool->bulletin_list(bulletinfile, 2, BL_NOADD);
  }

  return NULL;
}


area_header *soup::getNextArea ()
{
  int flags;
  area_header *ah;

  bool isPersArea = (persArea && (NAID == 0));

  if (isPersArea) flags = A_COLLECTION | A_READONLY | A_LIST;
  else
  {
    flags = A_INTERNET | A_CHRSLATIN1 |
            (areas[NAID]->isEmail ? A_NETMAIL | A_FORCED : 0);

    if (areas[NAID]->number[0]) flags |= A_SUBSCRIBED;
    else flags |= A_COLLECTION;
  }

  ah = new area_header(bm, NAID + bm->driverList->getOffset(this),
                       (isPersArea ? "PERS" : areas[NAID]->number),
                       (isPersArea ? "PERSONAL" : areas[NAID]->title),
                       (isPersArea ? "Letters addressed to you"
                                   : areas[NAID]->title),
                       (isPersArea ? "SOUP Personal" : "SOUP"),
                       flags, areas[NAID]->totmsgs, areas[NAID]->numpers,
                       NEWSGROUPLIST, OTHERSUBJLEN);
  NAID++;

  return ah;
}


inline int soup::getNoOfLetters ()
{
  return areas[currentArea]->totmsgs;
}


letter_header *soup::getNextLetter ()
{
  int area, letter;
  net_address na;
  bool latin1 = isLatin1();
  char *from = NULL, *faddr = NULL, *raddr = NULL, *to = NULL, *subj = NULL,
       *date = NULL, *newsgrps = NULL, *refs = NULL,
       *msgid = NULL, *newrefs = NULL;
  int name, n_len, addr, a_len;

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

  bool c_news = (areas[area]->format == 'i');

  if (!openFile(areas[area])) fatalError(error);
  fseek(msgFile, body[area][letter].pointer, SEEK_SET);

  if (c_news)
  {
    char *cline = tool->fgetsline(msgFile);

    subj = strdupplus(strtokn(cline, '\t'));
    char *froml = strtokn(NULL, '\t');
    date = strdupplus(strtokn(NULL, '\t'));

    newsgrps = (char *) areas[area]->title;

    if (areas[area]->index == 'c')
    {
      char *token = strtokn(NULL, '\t');
      msgid = (*token ? strdupplus(token) : NULL);
      token = strtokn(NULL, '\t');
      refs = (*token ? strdupplus(token) : NULL);
    }

    parseAddress(froml, name, n_len, addr, a_len);

    if (n_len)
    {
      from = new char[n_len + 1];
      strncpy(from, froml + name, n_len);
      from[n_len] = '\0';
    }

    if (a_len)
    {
      faddr = new char[a_len + 1];
      strncpy(faddr, froml + addr, a_len);
      faddr[a_len] = '\0';
    }

    delete[] cline;
  }
  else
  {
    headerlines[area][letter] = tool->parseHeader(msgFile,
                                                  body[area][letter].pointer +
                                                  body[area][letter].msglen,
                                                  &latin1);

    from = tool->getFrom();
    faddr = tool->getFaddr();
    raddr = tool->getReplyToAddr();
    to = tool->getTo();
    subj = tool->getSubject();
    date = tool->getDate();

    newsgrps = tool->getFollowupTo();
    if (!newsgrps) newsgrps = tool->getNewsgroups();

    msgid = tool->getMessageID();
    refs = tool->getReferences();
  }

  if (!areas[area]->isEmail && !newsgrps) newsgrps = (char *) "";

  if (areas[area]->isEmail || !refs) refs = msgid;
  else if (msgid)
  {
    newrefs = new char[strlen(refs) + strlen(msgid) + 2];
    sprintf(newrefs, "%s %s", refs, msgid);
    refs = newrefs;
  }

  if (faddr) na.set(faddr);

  letter_header *lh = new letter_header(bm, letter,
                                        (from ? from : ""),
                                        (to ? to : ""),
                                        (subj ? subj : ""),
                                        (date ? date : ""),
                                        letter + 1, 0, area,
                                        (areas[area]->isEmail ? PRIVATE : 0),
                                        latin1, &na, raddr, newsgrps, refs,
                                        this);

  if (c_news)
  {
    delete[] from;
    delete[] faddr;
    delete[] subj;
    delete[] date;
  }

  delete[] newrefs;

  NLID++;
  return lh;
}


const char *soup::getBody (int area, int letter, letter_header *lh)
{
  (void) lh;
  int kar;

  // get number of headerlines plus separation line
  long hlines = headerlines[area][letter] + 1;

  if (areas[area]->format == 'i') return "(no body available)";

  delete[] letterBody;
  letterBody = new char[body[area][letter].msglen + hlines + 1];
  unsigned char *p = (unsigned char *) letterBody;

  if (!openFile(areas[area])) fatalError(error);
  fseek(msgFile, body[area][letter].pointer, SEEK_SET);

  bool startOfLine = true;

  for (long c = 0; c < body[area][letter].msglen; c++)
  {
    kar = fgetc(msgFile);

    // a message shouldn't (must not) contain a 0x00 character
    if (kar == 0) kar = ' ';

    // convert header lines into kludge lines
    if (startOfLine && hlines)     // to prevent letterBody from overflow
    {
      *p++ = '\1';
      hlines--;
    }

    *p++ = kar;

    startOfLine = (kar == '\n');
  }

  do p--;
  // strip blank lines
  while (p >= (unsigned char *) letterBody && (*p == ' ' || *p == '\n'));

  *++p = '\0';

  // decode quoted-printable
  if (bm->driverList->allowQuotedPrintable() && tool->isQP(letterBody))
    tool->mimeDecodeBody(letterBody);

  // decode UTF-8
  if (tool->isUTF8(letterBody)) tool->utf8DecodeBody(letterBody);

  return letterBody;
}


void soup::initMSF (int **msgstat)
{
  tool->MSFinit(msgstat, persOffset(), persIDX, persmsgs);
}


int soup::readMSF (int **msgstat)
{
  tool->XTIread(msgstat, persOffset(), "XTI");
  return (MSF_READ | MSF_REPLIED | MSF_MARKED) |
         (*bm->resourceObject->get(IsPersonal) ? MSF_PERSONAL : 0);
}


const char *soup::saveMSF (int **msgstat)
{
  return tool->XTIsave(msgstat, persOffset(), "XTI", false);
}


const char *soup::getPacketID () const
{
  if (bm->getServiceType() == ST_REPLY)
    return stem(bm->resourceObject->get(InfName));
  else
    return stem(bm->resourceObject->get(PacketName));
}


inline bool soup::isLatin1 ()
{
  return true;
}


inline bool soup::offlineConfig ()
{
  return offcfg;
}


bool soup::getAreas ()
{
  FILE *areasFile;
  char *prefix, *name, *encoding;
  SOUP_AREA_INFO anchor, *m = &anchor;
  bool success = true;

  if ((areasFile = bm->fileList->ftryopen("AREAS", "rt")))
  {
    int offset = 0;
    int totareas = 0;
    long lastpos = 0;

    if (persArea)
    {
      offset++;
      totareas++;
      m->next = new SOUP_AREA_INFO;
      m = m->next;
      m->number[0] = '\0';
      m->prefix = NULL;
      m->title = NULL;
      m->format = ' ';
      m->index = ' ';
      m->isEmail = false;
      m->totmsgs = 0;
      m->numpers = 0;
      m->offset = 0;
      m->next = NULL;
    }

    if (offcfg)
    {
      offset++;
      totareas++;
      m->next = new SOUP_AREA_INFO;
      m = m->next;
      m->number[0] = '\0';
      m->prefix = NULL;
      m->title = strdupplus(AREA_EXTRA);
      m->format = ' ';
      m->index = ' ';
      m->isEmail = false;
      m->totmsgs = 0;
      m->numpers = 0;
      m->offset = 0;
      m->next = NULL;
    }

    while (fgetsnl(line, sizeof(line), areasFile))
    {
      mkstr(line);
      prefix = strtok(line, "\t");
      name = strtok(NULL, "\t");
      encoding = strtok(NULL, "\t");

      if (prefix && name && encoding && (strlen(encoding) >= 2))
      {
        totareas++;
        m->next = new SOUP_AREA_INFO;
        m = m->next;
        sprintf(m->number, "%d", totareas - offset);
        m->prefix = strdupplus(prefix);
        m->title = strdupplus(name);
        m->format = encoding[0];
        m->index = encoding[1];
        m->isEmail = ((encoding[2] == 'm') ||
                      ((encoding[2] != 'n') && ((*encoding == 'm') ||
                                                (*encoding == 'M') ||
                                                (*encoding == 'b'))));
        m->totmsgs = 0;
        m->numpers = 0;
        m->offset = lastpos;
        m->next = NULL;

        if (!openFile(m))
        {
          success = false;
          break;
        }
      }

      lastpos = ftell(areasFile);
    }

    if (totareas)
    {
      areas = new SOUP_AREA_INFO *[totareas];
      m = anchor.next;
      int i = 0;

      while (m)
      {
        areas[i++] = m;
        m = m->next;
      }

      maxAreas = totareas;
    }

    fclose(areasFile);
  }
  else
  {
    strcpy(error, "Could not open areas file.");
    success = false;
  }

  return success;
}


// if there is no index file, we have to build the indices anyway, so it's
// (probably?) not worth using existing 'c', 'C' or 'i' format index files
int soup::buildIndices ()
{
  int totmsgs, pmsgs, pers = 0;
  bool binary, c_news;
  size_t readlen = 0;
  long msglen, pos, lpos;
  unsigned char bdata[4];
  bool success = true;

  struct MSG_IDX
  {
    long offset;
    long msglen;
    MSG_IDX *next;
  } anchor, *ndx, *d;

  pIDX pAnchor, *pidx = &pAnchor, *p;
  const char *isPersonal = bm->resourceObject->get(IsPersonal);

  for (int a = persOffset(); a < maxAreas; a++)
  {
    totmsgs = 0;
    pmsgs = 0;
    ndx = &anchor;
    binary = (toupper(areas[a]->format) == 'B');
    c_news = (toupper(areas[a]->index) == 'C');

    if (openFile(areas[a]))
    {
      while (binary ? (fread(bdata, sizeof(char), 4, msgFile) == 4)
                    : (readlen = fgetsnl(line, sizeof(line), msgFile)))
      {
        msglen = -1;
        pos = ftell(msgFile);

        switch (areas[a]->format)
        {
          case 'u':
            sscanf(line, "#! rnews %ld", &msglen);
            if (msglen == 0) msglen = -1;            // ignore it
            break;

          case 'm':
            if (strncmp(line, "From ", 5) == 0)
            {
              do lpos = ftell(msgFile);
              while (fgetsnl(line, sizeof(line), msgFile) &&
                     strncmp(line, "From ", 5) != 0);

              msglen = lpos - pos;
              fseek(msgFile, pos, SEEK_SET);     // back to behind "From "
            }
            break;

          case 'M':
            // we won't handle that format
            break;

          case 'b':
          case 'B':
            msglen = getMSBlong(bdata);
            break;

          case 'i':
            if (c_news)
            {
              pos -= readlen - strlen(strtokn(line, '\t')) - 1;
              msglen = 0;
            }
            break;
        }

        if (msglen != -1)
        {
          totmsgs++;
          ndx->next = new MSG_IDX;
          ndx = ndx->next;
          ndx->offset = pos;
          ndx->msglen = msglen;
          ndx->next = NULL;

          if (msglen && *isPersonal)
          {
            while (ftell(msgFile) < pos + msglen)
            {
              const char *line = tool->fgetsline(msgFile);

              if (areas[a]->isEmail ||
                  (strncasecmp(line, "From: ", 6) != 0 &&
                   strstr(line, isPersonal) != NULL))
              {
                pers++;
                pmsgs++;
                pidx->next = new pIDX;
                pidx = pidx->next;
                pidx->persidx.area = a;
                pidx->persidx.msgnum = totmsgs - 1;
                pidx->next = NULL;

                delete[] line;
                break;
              }

              delete[] line;
            }

            fseek(msgFile, pos + msglen, SEEK_SET);
          }
          else fseek(msgFile, msglen, SEEK_CUR);
        }
      }
    }
    else success = (areas[a]->number[0] == '\0');

    if (totmsgs)
    {
      body[a] = new bodytype[totmsgs];
      headerlines[a] = new long[totmsgs];
      ndx = anchor.next;
      int i = 0;

      while (ndx)
      {
        body[a][i].pointer = ndx->offset;
        body[a][i].msglen = ndx->msglen;
        headerlines[a][i] = 0;               // will be set in getNextLetter()
        i++;
        d = ndx;
        ndx = ndx->next;
        delete d;
      }

      areas[a]->totmsgs = totmsgs;
      areas[a]->numpers = pmsgs;
    }

    if (!success) break;
  }

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
      p = pidx;
      pidx = pidx->next;
      delete p;
    }
  }

  if (persArea) areas[0]->totmsgs = areas[0]->numpers = pers;

  return (success ? pers : -1);
}


bool soup::openFile (SOUP_AREA_INFO *area)
{
  static long lastOffset;

  char fname[MYMAXPATH];
  const char *ext = (area->format == 'i' ? "IDX" : "MSG");
  const char *access;
  bool success = true;

  if (!msgFile) lastOffset = -1;     // initialize static variable

  if (strcmp(area->title, AREA_EXTRA) == 0) return false;

  if (area->offset != lastOffset)
  {
    lastOffset = area->offset;

    if (msgFile) fclose(msgFile);
    msgFile = NULL;

    mkfnamext(fname, area->prefix, ext);

    if (bm->fileList->exists(fname))
    {
      access = "open";
      msgFile = bm->fileList->ftryopen(fname, "rb");
    }
    else access = "find";

    if (!msgFile)
    {
      sprintf(error, "Could not %s %.20s.%s file.", access, area->prefix, ext);
      success = false;
    }
  }

  return success;
}


bool soup::readCommands ()
{
  FILE *file;
  char *cmds, *cmd;
  bool subscribe = true, unsubscribe = true;

  if ((file = bm->fileList->ftryopen("COMMANDS", "rt")))
  {
    while (fgetsnl(line, sizeof(line), file))
    {
      mkstr(line);

      if (strncasecmp(line, "supported ", 10) == 0)
      {
        cmds = line + 10;
        subscribe = unsubscribe = false;

        while ((cmd = strtok(cmds, " ")))
        {
          if (strcasecmp(cmd, "subscribe") == 0) subscribe = true;
          else if (strcasecmp(cmd, "unsubscribe") == 0) unsubscribe = true;
          cmds = NULL;
        }
      }
      else if (strncasecmp(line, "hostname ", 9) == 0)
        bm->resourceObject->set(SysOpName, line + 9);
    }
    fclose(file);
  }

  return (subscribe && unsubscribe);
}


int soup::getAreaIndex (const char *name)
{
  int a;

  for (a = persOffset(); a < maxAreas; a++)
    if (strcasecmp(areas[a]->title, name) == 0) break;

  return (a == maxAreas ? -1 : a);
}


int soup::getAreaIndex (bool email) const
{
  int a;

  for (a = persOffset(); a < maxAreas; a++)
    if (areas[a]->isEmail == email) break;

  return (a == maxAreas ? -1 : a);
}


/*
   SOUP reply driver
*/

soup_reply::soup_reply (bmail *bm, main_driver *mainDriver)
{
  this->bm = bm;
  this->mainDriver = (soup *) mainDriver;
  firstReply = currentReply = NULL;
  extraAreas = NULL;
  extraAreasLIST = NULL;
  extraAreasCount = extraAreasLISTCount = 0;
  list = NULL;
}


soup_reply::~soup_reply ()
{
  REPLY *next, *r = firstReply;

  while (r)
  {
    next = r->next;

    delete[] r->header.from;
    delete[] r->header.to;
    delete[] r->header.subject;
    delete[] r->header.date;
    delete[] r->header.references;
    delete[] r->header.newsgroups;
    delete[] r->header.netAddr;
    if (r->filename) remove(r->filename);
    delete[] r->filename;

    delete r;
    r = next;
  }

  EXTRA_AREA *nxt, *ea = extraAreas;

  while (extraAreasCount--)
  {
    nxt = ea->next;
    delete[] ea->title;
    delete ea;
    ea = nxt;
  }

  free(extraAreasLIST);
  delete[] replyBody;

  if (list) fclose(list);
}


const char *soup_reply::init ()
{
  const char *ext = bm->resourceObject->get(ReplyExtension);

  sprintf(replyPacketName, "%.8s.%s", mainDriver->getPacketID(),
                                      ext ? ext : bm->service->getArchiveExt());

  if (bm->getServiceType() != ST_REPLY) replyInf();

  return (getReplies() ? NULL : error);
}


inline area_header *soup_reply::getNextArea ()
{
  return getReplyArea("SOUP Replies");
}


letter_header *soup_reply::getNextLetter ()
{
  int flag = 0;
  net_address na;

  char *newsgroups = strdupplus(currentReply->header.newsgroups);
  int area = (newsgroups ? mainDriver->getAreaIndex(strtok(newsgroups, ","))
                         : -1);
  delete[] newsgroups;

  if (area == -1)
    // get first area of same type (e-mail or newsgroup)
    area = mainDriver->getAreaIndex(currentReply->header.netAddr != NULL);

  area += (area == -1 ? 1 : bm->driverList->getOffset(mainDriver));

  if (currentReply->header.netAddr)
  {
    flag = PRIVATE;
    na.set(currentReply->header.netAddr);
  }

  letter_header *newLetter = new letter_header(bm, NLID,
                                               currentReply->header.from,
                                               currentReply->header.to,
                                               currentReply->header.subject,
                                               currentReply->header.date,
                                               NLID, 0, area, flag,
                                               currentReply->latin1,
                                               &na, NULL,
                                               currentReply->header.newsgroups,
                                               currentReply->header.references,
                                               this);

  NLID++;
  currentReply = currentReply->next;

  return newLetter;
}


const char *soup_reply::getBody (int area, int letter, letter_header *lh)
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


void soup_reply::resetLetters ()
{
  NLID = 1;
  currentReply = firstReply;
}


inline bool soup_reply::isLatin1 ()
{
  return mainDriver->isLatin1();
}


void soup_reply::enterLetter (letter_header *newLetter,
                              const char *newLetterFileName, long msglen)
{
  REPLY *newReply = new REPLY;

  newReply->header.from = strdupplus(newLetter->getFrom());
  newReply->header.subject = strdupplus(newLetter->getSubject());
  newReply->header.date = strdupplus(mainDriver->tool->rfc822_date());
  newReply->header.references = strdupplus(newLetter->getReplyID());

  if (bm->areaList->isNetmail())
  {
    newReply->header.to = strdupplus(newLetter->getTo());
    newReply->header.newsgroups = NULL;
    net_address na = newLetter->getNetAddr();
    newReply->header.netAddr = strdupplus(na.get(true));
  }
  else
  {
    newReply->header.to = strdupplus("");
    newReply->header.newsgroups = strdupplus(newLetter->getTo());
    newReply->header.netAddr = NULL;
  }

  newReply->latin1 = true;

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


void soup_reply::killLetter (int letter)
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


const char *soup_reply::newLetterBody (int letter, long size)
{
  REPLY *r = firstReply;
  for (int l = 1; l < letter; l++) r = r->next;

  if (size >= 0) r->length = size;
  return r->filename;
}


void soup_reply::replyInf ()
{
  char fname[MYMAXPATH], *ext;
  const char *areas = bm->fileList->exists("AREAS");
  const char *commands = bm->fileList->exists("COMMANDS");
  const char *list = bm->fileList->exists("LIST");
  const char *dir = bm->resourceObject->get(InfWorkDir);

  mkfname(fname, dir, "AREAS");
  fcopy(areas, fname);

  // to be independent from mainDriver,
  // we create the area files by using fileList
  for (int i = 0; i < bm->fileList->getNoOfFiles(); i++)
  {
    bm->fileList->gotoFile(i);
    ext = strrchr((char*)bm->fileList->getName(), '.');

    if (ext && (strcasecmp(ext, ".MSG") == 0 ||
                strcasecmp(ext, ".IDX") == 0))
      fcreate(dir, bm->fileList->getName());
  }

  if (commands)
  {
    mkfname(fname, dir, "COMMANDS");
    fcopy(commands, fname);
  }

  if (list)
  {
    mkfname(fname, dir, "LIST");
    fcopy(list, fname);
  }

  bm->service->createSystemInfFile(fmtime(areas),
                                   mainDriver->getPacketID(),
                                   strrchr(replyPacketName, '.') + 1);
}


bool soup_reply::putReplies ()
{
  FILE *rep = NULL;
  bool success = true;

  if (noOfLetters)
    if (!(rep = fopen("REPLIES", "wb"))) return false;

  REPLY *r = firstReply;

  for (int l = 0; l < noOfLetters; l++)
  {
    if (!put1Reply(rep, r, l))
    {
      success = false;
      break;
    }
    r = r->next;
  }

  if (rep) fclose(rep);

  if (success && bm->areaList->areaConfig()) success = putConfig();

  return success;
}


bool soup_reply::put1Reply (FILE *rep, REPLY *r, int letter)
{
  FILE *reply;
  char *replyText, repName[13];
  bool success;

  if (!(reply = fopen(r->filename, "rt"))) return false;
  replyText = new char[r->length + 1];
  success = (fread(replyText, sizeof(char), r->length, reply) ==
                                                          (size_t) r->length);
  fclose(reply);

  if (success)
  {
    replyText[r->length] = '\0';

    bool has8bit = false;
    char *p = replyText;

    while (*p)
      if (*p++ & 0x80)
      {
        has8bit = true;
        break;
      }

    bool isEmail = (r->header.netAddr != NULL);

    sprintf(repName, "R%07d.MSG", letter);
    fprintf(rep, "R%07d\t%sn\n", letter, (isEmail ? "mail\tb" : "news\tB"));

    success = ((reply = fopen(repName, "wb")) != NULL);

    if (success)
    {
      unsigned char bsize[4];

      putMSBlong(bsize, 0);
      fwrite(bsize, sizeof(bsize), 1, reply);

      mainDriver->tool->writeHeader(reply, isEmail, letter, r->header.from,
                                    (isEmail ? r->header.to
                                             : r->header.newsgroups),
                                    r->header.subject, r->header.date,
                                    r->header.references, r->header.netAddr,
                                    has8bit);

      if (has8bit && (toupper(*(bm->resourceObject->get(MIMEBody))) == 'Q'))
        success = mainDriver->tool->mimeEncodeBody(reply, replyText);
      else
        success = (fwrite(replyText, sizeof(char), r->length, reply) ==
                                                          (size_t) r->length);

      putMSBlong(bsize, ftell(reply) - sizeof(bsize));
      fseek(reply, 0, SEEK_SET);
      fwrite(bsize, sizeof(bsize), 1, reply);
      fseek(reply, 0, SEEK_END);
      fclose(reply);
    }
  }

  delete[] replyText;
  return success;
}


bool soup_reply::getReplies ()
{
  file_list *replies;
  const char *repFile = NULL;
  char *fname = NULL;
  FILE *rep = NULL, *msg;
  unsigned char bdata[4];
  long msglen, pos, lpos;
  bool isEmail, success = true;

  *error = '\0';

  if (!(replies = unpackReplies()) ||
      !(repFile = replies->exists("REPLIES"))) return true;

  REPLY seed, *r = &seed;
  seed.next = NULL;

  if (!(rep = replies->ftryopen(repFile, "rt")))
  {
    strcpy(error, "Could not open reply file.");
    success = false;
  }
  else
  {
    while (fgetsnl(line, sizeof(line), rep))
    {
      mkstr(line);

      const char *prefix = strtok(line, "\t");
      const char *kind = strtok(NULL, "\t");
      char format = *(strtok(NULL, "\t"));

      if (strcmp(kind, "mail") == 0) isEmail = true;
      else if (strcmp(kind, "news") == 0) isEmail = false;
      // we don't handle other kinds
      else continue;

      bool binary = (toupper(format) == 'B');

      if ((format != 'u') && (format != 'm') && !binary)
        // we don't handle other formats
        continue;

      delete[] fname;
      fname = new char[strlen(prefix) + 5];
      sprintf(fname, "%s.MSG", prefix);

      if ((msg = fopen(fname, "rb")))
      {
        // similar stuff like in soup::buildIndices()
        while (binary ? (fread(bdata, sizeof(char), 4, msg) == 4)
                      : fgetsnl(line, sizeof(line), msg))
        {
          msglen = -1;
          pos = ftell(msg);

          switch (format)
          {
            case 'u':
              sscanf(line, "#! rnews %ld", &msglen);
              if (msglen == 0) msglen = -1;            // ignore it
              break;

            case 'm':
              if (strncmp(line, "From ", 5) == 0)
              {
                do lpos = ftell(msg);
                while (fgetsnl(line, sizeof(line), msg) &&
                       strncmp(line, "From ", 5) != 0);

                msglen = lpos - pos;
                fseek(msg, pos, SEEK_SET);     // back to behind "From "
              }
              break;

            case 'b':
            case 'B':
              msglen = getMSBlong(bdata);
              break;
          }

          if (msglen != -1)
          {
            long end = pos + msglen;

            if (!get1Reply(msg, r, isEmail, end))
            {
              success = false;
              fclose(msg);
              goto GETREPEND;
            }
            else fseek(msg, end, SEEK_SET);
          }
        }

        fclose(msg);

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
  }

GETREPEND:
  if (noOfLetters) firstReply = seed.next;

  if (rep) fclose(rep);
  if (repFile) remove(repFile);
  delete[] fname;
  delete replies;

  if (!success)
  {
    clearDirectory(".");     // clean up
    if (!*error) strcpy(error, "Could not get reply.");
  }

  return success;
}


bool soup_reply::get1Reply (FILE *msg, REPLY *&r, bool isEmail, long end)
{
  FILE *reply;
  char filename[13], filepath[MYMAXPATH], *replyText = NULL;
  static int fnr = 1;
  bool success;

  r->next = new REPLY;
  r = r->next;
  r->header.from = NULL;
  r->header.to = NULL;
  r->header.subject = NULL;
  r->header.date = NULL;
  r->header.references = NULL;
  r->header.newsgroups = NULL;
  r->header.netAddr = NULL;
  r->latin1 = isLatin1();
  r->filename = NULL;
  r->length = 0;
  r->next = NULL;

  noOfLetters++;

  sprintf(filename, "%s.%03d", (isEmail ? "mail" : "news"), fnr++);
  mkfname(filepath, bm->resourceObject->get(WorkDir), filename);

  success = ((reply = fopen(filepath, "wt")) != NULL);

  if (success)
  {
    (void) mainDriver->tool->parseHeader(msg, end, &r->latin1);

    const char *from = mainDriver->tool->getFrom();
    const char *faddr = mainDriver->tool->getFaddr();
    r->header.from = new char[strlen(from) + slen(faddr) + 4];
    char *p = (char *) r->header.from;
    p += sprintf(p, "%s", from);
    if (faddr) sprintf(p, "%s<%s>", (*from ? " " : ""), faddr);

    r->header.subject = strdupplus(mainDriver->tool->getSubject());
    r->header.date = strdupplus(mainDriver->tool->getDate());

    if (isEmail)
    {
      r->header.to = strdupplus(mainDriver->tool->getTo());

      r->header.references = strdupplus(mainDriver->tool->getInReplyTo());
      if (!r->header.references)
        r->header.references = strdupplus(mainDriver->tool->getReferences());

      r->header.netAddr = strdupplus(mainDriver->tool->getTaddr());
    }
    else
    {
      r->header.to = strdupplus("");
      r->header.references = strdupplus(mainDriver->tool->getReferences());

      r->header.newsgroups = strdupplus(mainDriver->tool->getNewsgroups());
      if (!r->header.newsgroups) r->header.newsgroups = strdupplus("");
    }

    long msglen = end - ftell(msg);
    replyText = new char[msglen + 1];

    success = (fread(replyText, sizeof(char), msglen, msg) == (size_t) msglen);
    replyText[msglen] = '\0';

    if (mainDriver->tool->isQP()) mainDriver->tool->mimeDecodeBody(replyText);

    msglen = strlen(replyText);
    if (success) success = (fwrite(replyText, sizeof(char), msglen, reply) ==
                                                             (size_t) msglen);
    fclose(reply);

    r->filename = strdupplus(filepath);
    r->length = msglen;
  }

  delete[] replyText;

  return success;
}


void soup_reply::getConfig ()
{
  FILE *cmds;
  char buffer[MYMAXLINE];
  int index;

  file_list *replies = new file_list(bm->resourceObject->get(ReplyWorkDir));

  const char *cmdsFile = replies->exists("COMMANDS");

  if ((cmds = replies->ftryopen(cmdsFile, "rb")))
  {
    while (fgetsnl(buffer, sizeof(buffer), cmds))
    {
      mkstr(buffer);

      const char *area = (strtok(buffer, " ") ? strtok(NULL, "\t") : NULL);

      if (area)
      {
        bool subscribe = (strcasecmp(buffer, "subscribe") == 0);
        bool unsubscribe = (strcasecmp(buffer, "unsubscribe") == 0);

        if ((index = mainDriver->getAreaIndex(area)) != -1)
        {
          bm->areaList->gotoArea(index + bm->driverList->getOffset(mainDriver));

          if (subscribe) bm->areaList->setAdded(true);
          else if (unsubscribe) bm->areaList->setDropped(true);
        }
        else if (subscribe) setExtraArea(-1, true, area);
      }
    }

    fclose(cmds);
    remove(cmdsFile);
  }

  delete replies;
}


bool soup_reply::putConfig ()
{
  FILE *cmds;
  static char cmdsName[] = "COMMANDS";
  bool subscribe, unsubscribe;
  long pos;
  bool success = true;

  if (!(cmds = fopen(cmdsName, "wb"))) return false;

  int offset = bm->driverList->getOffset(mainDriver) +
               mainDriver->persOffset();

  // real areas

  for (int a = offset; a < bm->areaList->getNoOfAreas(); a++)
  {
    bm->areaList->gotoArea(a);

    subscribe = (!bm->areaList->isSubscribed() && bm->areaList->isAdded());
    unsubscribe = (bm->areaList->isSubscribed() && bm->areaList->isDropped());

    if (subscribe || unsubscribe)
    {
      const char *title = bm->areaList->getTitle();

      if (fprintf(cmds, "%ssubscribe %s\n", (subscribe ? "" : "un"), title) !=
          (int) strlen(title) + (subscribe ? 11 : 13))
      {
        success = false;
        break;
      }
    }
  }

  // extra areas

  if (success && extraAreasCount)
  {
    EXTRA_AREA *ea = extraAreas;

    do
    {
      if (ea->subscribed)
        if (fprintf(cmds, "subscribe %s\n", ea->title) !=
                                                 (int) strlen(ea->title) + 11)
        {
          success = false;
          break;
        }

      ea = ea->next;
    }
    while (ea);
  }

  if (success && extraAreasLISTCount)
  {
    for (int i = 0; i < extraAreasLISTCount; i++)
    {
      if ((pos = extraAreasLIST[i]) < 0)
      {
        fseek(list, -(pos + 1), SEEK_SET);
        fgetsnl(line, sizeof(line), list);
        const char *title = strtok(mkstr(line), "\t");

        if (fprintf(cmds, "subscribe %s\n", title) != (int) strlen(title) + 11)
        {
          success = false;
          break;
        }
      }
    }
  }

  if (cmds) fclose(cmds);
  if (!success) remove(cmdsName);

  return success;
}


// build index for LIST file
void soup_reply::initExtraAreaLIST ()
{
  int const ALLOCITEMS = 500, ALLOCSIZE = ALLOCITEMS * sizeof(long);
  static long alloclen, *eal;
  long pos = 0;

  if ((list = bm->fileList->ftryopen("LIST", "rt")))
  {
    while (fgetsnl(line, sizeof(line), list))
    {
      const char *title = strtok(mkstr(line), "\t");

      // is a real area?
      if (mainDriver->getAreaIndex(title) != -1)
      {
        pos = ftell(list);
        continue;
      }

      int i;
      EXTRA_AREA *ea = extraAreas;

      // is already subscribed?
      for (i = 0; i < extraAreasCount; i++)
      {
        if (strcmp(title, ea->title) == 0)
        {
          pos = ftell(list);
          break;
        }
        ea = ea->next;
      }

      if (i < extraAreasCount) continue;

      // new area

      if (extraAreasLISTCount % ALLOCITEMS == 0)
      {
        if (!extraAreasLIST)
        {
          extraAreasLIST = (long *) malloc(ALLOCSIZE);
          alloclen = ALLOCSIZE;
        }
        else
        {
          if ((eal =  (long *) realloc(extraAreasLIST, alloclen + ALLOCSIZE)))
          {
            extraAreasLIST = eal;
            alloclen += ALLOCSIZE;
          }
          else return;
        }
      }

      extraAreasLIST[extraAreasLISTCount] = pos + 1;
      extraAreasLISTCount++;
      pos = ftell(list);
    }
  }
}


const char *soup_reply::getExtraArea (int &count_or_state)
{
  if (count_or_state == -1)
  {
    if(!extraAreasLIST) initExtraAreaLIST();

    count_or_state = extraAreasCount + extraAreasLISTCount;
    return NULL;
  }

  else if (count_or_state < extraAreasCount)
  {
    EXTRA_AREA *ea = extraAreas;

    for (int i = 0; i < count_or_state; i++) ea = ea->next;

    count_or_state = (ea->subscribed ? 1 : 0);
    return ea->title;
  }

  else
  {
    int idx = count_or_state - extraAreasCount;
    long pos = extraAreasLIST[idx];

    count_or_state = (pos > 0 ? 0 : 1);
    fseek(list, (pos > 0 ? pos : -pos) - 1, SEEK_SET);
    fgetsnl(line, sizeof(line), list);

    return strtok(mkstr(line), "\t");
  }
}


void soup_reply::setExtraArea (int extra, bool subscribe, const char *title)
{
  EXTRA_AREA *ea;

  // dropping mainDriver area persOffset() (AREA_EXTRA) will have no effect
  // on the area itself, but area_list::areaConfig() will report now that
  // the configuration has changed
  bm->areaList->gotoArea(bm->driverList->getOffset(mainDriver) +
                         mainDriver->persOffset());
  bm->areaList->setDropped(true);

  if (title)
  {
    ea = new EXTRA_AREA;
    ea->title = strdupplus(title);
    ea->subscribed = subscribe;
    ea->next = NULL;
    extraAreasCount++;

    if (!extraAreas) extraAreas = ea;
    else
    {
      EXTRA_AREA *a = extraAreas;
      while (a->next) a = a->next;
      a->next = ea;
    }
  }
  else if (extra < extraAreasCount)
  {
    ea = extraAreas;
    for (int i = 0; i < extra; i++) ea = ea->next;
    ea->subscribed = subscribe;
  }
  else
  {
    int idx = extra - extraAreasCount;
    long pos = extraAreasLIST[idx];

    if (((pos > 0) && subscribe) || ((pos < 0) && !subscribe))
      extraAreasLIST[idx] = -pos;
  }
}
