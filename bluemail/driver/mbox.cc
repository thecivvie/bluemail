/*
 * blueMail offline mail reader
 * Unix style mail file (Berkeley format - mbox) driver

 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include "mbox.h"
#include "../../common/auxil.h"
#include "../../common/error.h"


/*
   Unix style mail file (Berkeley format - mbox) main driver
*/

mbox::mbox (bmail *bm)
{
  this->bm = bm;
  tool = new basic_tool(bm, this);

  maxAreas = 1;
  totmsgs = 0;

  anchor = NULL;
  msg = NULL;

  mboxf = NULL;
}


mbox::~mbox ()
{
  while (totmsgs) delete msg[--totmsgs];

  delete[] letterBody;
  delete[] msg;
  delete tool;

  if (mboxf) fclose(mboxf);
}


const char *mbox::init ()
{
  const char *username = bm->resourceObject->get(UserName);
  bm->resourceObject->set(LoginName, username);
  bm->resourceObject->set(AliasName, username);
  bm->resourceObject->set(BBSName, "Mail File");
  bm->resourceObject->set(hasLoginName, "N");

  mfile = bm->getLastFileName();
  bm->resourceObject->set(SysOpName, fname(mfile));

  strncpy(lockfile, mfile, MYMAXPATH);
  lockfile[MYMAXPATH] = '\0';
  strcat(lockfile, ".lock");

  if (!(mboxf = bm->fileList->ftryopen(fname(mfile), "xrt")) ||
      (access(lockfile, F_OK) == 0)) return "Could not open mail file.";

  buildIndices();

  return NULL;
}


area_header *mbox::getNextArea ()
{
  return new area_header(bm, bm->driverList->getOffset(this),
                         "mbox", "mbox", "Letters addressed to you", "Mbox",
                         A_LIST | A_NETMAIL | A_INTERNET | A_CHRSLATIN1,
                         totmsgs, totmsgs, OTHERFROMTOLEN, OTHERSUBJLEN);
}


inline int mbox::getNoOfLetters ()
{
  return totmsgs;
}


letter_header *mbox::getNextLetter ()
{
  net_address na;
  bool latin1 = isLatin1();

  fseek(mboxf, msg[NLID]->header, SEEK_SET);
  msg[NLID]->headerlines = tool->parseHeader(mboxf, msg[NLID]->end, &latin1);

  if (tool->getFaddr()) na.set(tool->getFaddr());

  int letter = NLID++;
  return new letter_header(bm, letter,
                           tool->getFrom(),
                           tool->getTo(),
                           tool->getSubject(),
                           tool->getDate(),
                           NLID, 0, 0, PRIVATE, latin1, &na,
                           tool->getReplyToAddr(), NULL,
                           tool->getMessageID(), this);
}


const char *mbox::getBody (int area, int letter, letter_header *lh)
{
  (void) area;
  (void) lh;
  int kar, qi = -1;
  char qs[] = ">From ";
  unsigned char *qp = NULL;

  // get number of headerlines plus separation line
  long hlines = msg[letter]->headerlines + 1;

  delete[] letterBody;
  letterBody = new char[msg[letter]->end - msg[letter]->header + hlines + 1];
  unsigned char *p = (unsigned char *) letterBody;

  fseek(mboxf, msg[letter]->header, SEEK_SET);

  bool startOfLine = true;

  while (ftell(mboxf) < msg[letter]->end)
  {
    kar = fgetc(mboxf);

    // a message shouldn't (must not) contain a 0x00 character
    if (kar == 0) kar = ' ';

    // convert header lines into kludge lines
    if (startOfLine && hlines)     // to prevent letterBody from overflow
    {
      *p++ = '\1';
      hlines--;
    }

    // ">From " quoting
    if (qi >= 0)
    {
      if (qi > 0 || kar != '>')
      {
        if (kar == qs[++qi])
        {
          if (qs[qi])     // end of ">From " quoting
          {
            memmove(qp, qp + 1, p - qp - 1);
            p--;
            qi = -1;
          }
        }
        else qi = -1;     // no ">From " quoting, quit
      }
    }

    // start of possible ">From " quoting
    if (startOfLine && (kar == '>'))
    {
      qp = p;
      qi = 0;
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


inline void mbox::initMSF (int **msgstat)
{
  (void) msgstat;
}


int mbox::readMSF (int **msgstat)
{
  for (int l = 0; l < totmsgs; l++) msgstat[0][l] = msg[l]->stat;

  return (MSF_READ | MSF_REPLIED | MSF_MARKED);
}


const char *mbox::saveMSF (int **msgstat)
{
  // Is it possible to avoid copying the whole mail file twice without
  // extensive use of memory? After inserting the (X-)Status:-line(s),
  // a message can shrink or grow, so if we'd intend to do all the
  // writing only in the mail file, where should we start - at the top or
  // at the end? Another problem: OSes using CRLF, where the length of a
  // string read or written differs from its size in the file!
  // If anyone knows a better (i.e. quicker) solution, please let me know.

  int fd;
  FILE *tfile;
  size_t len;

  // create old style lockfile
  if ((fd = open(lockfile, O_WRONLY | O_CREAT | O_EXCL, 0400)) == -1)
    return NULL;
  else close(fd);

  // take the mail file's fingerprint
  off_t msize = bm->fileList->getSize();
  time_t mtime = bm->fileList->getDate();
  fclose(mboxf);

  // reopen it
  if (!(mboxf = bm->fileList->ftryopen(fname(mfile), "Xr+t")) ||
      fsize(mfile) != msize || fmtime(mfile) != mtime)
  {
    remove(lockfile);
    return "!";           // no chance to handle this reopen error
  }

  // we now assume that the mail file hasn't been altered in the meantime

  const char *tmpfile = mymktmpfile(NULL);

  // recreate it temporarily
  if ((tfile = fopen(tmpfile, "w+t")))
  {
    int i = 0, n = 1;
    long pos, lastpos = -1;
    bool newline = true;

    while (fgets(line, sizeof(line), mboxf))
    // Must be fgets rather than fgetsnl!
    {
      pos = ftell(mboxf);

      if (newline && (pos == msg[i]->body && msg[i]->body > msg[i]->header))
        fprintf(tfile, "%s", Status(msgstat[0][i]));

      if (lastpos != msg[i]->status && lastpos != msg[i]->x_status)
        fprintf(tfile, "%s", line);

      if (n < totmsgs && pos == msg[n]->top) i = n++;

      newline = (strchr(line, '\n') != NULL);
      if (newline) lastpos = pos;
    }

    if (fseek(mboxf, 0, SEEK_SET) != 0 || fseek(tfile, 0, SEEK_SET) != 0)
    {
      fclose(tfile);
      remove(tmpfile);
      remove(lockfile);
      return NULL;
    }

    while ((len = fread(line, sizeof(char), sizeof(line), tfile)))
      if (fwrite(line, sizeof(char), len, mboxf) != len)
      {
        fclose(tfile);
        catastrophe(true, tmpfile);
      }

    fclose(tfile);
    off_t tsize = fsize(tmpfile);

    if (tsize < msize)
      if (fseek(mboxf, 0, SEEK_SET) != 0 ||
          ftruncate(fileno(mboxf), tsize) != 0) catastrophe(false, tmpfile);

    // close mail file (no longer needed open anyway)
    // in order to restore its original mtime
    fclose(mboxf);
    mboxf = NULL;
    fmtime(mfile, mtime);

    remove(tmpfile);
    remove(lockfile);
    return "";
  }

  remove(tmpfile);
  remove(lockfile);
  return NULL;
}


inline const char *mbox::getPacketID () const
{
  return "mbox";
}


inline bool mbox::isLatin1 ()
{
  return true;
}


inline bool mbox::offlineConfig ()
{
  return false;
}


void mbox::buildIndices ()
{
  long lastpos = 0;
  bool isHeader = false;
  MSG_INFO *m = NULL, *last = NULL;

  while (fgetsnl(line, sizeof(line), mboxf))
  {
    if (strncmp(line, "From ", 5) == 0)
    {
      totmsgs++;
      m = new MSG_INFO;
      if (!anchor) anchor = m;
      if (last) last->next = m;
      m->top = lastpos;
      m->header = ftell(mboxf);
      m->status = 0;
      m->x_status = 0;
      m->body = m->header;     // just in case we won't find a body later
      m->stat = 0;
      m->headerlines = 0;      // will be set in getNextLetter()
      m->next = NULL;
      last = m;
      isHeader = true;
    }

    else if (!m) continue;     // junk before first "From "

    else if (isHeader && (strncasecmp(line, "Status:", 7) == 0 ||
                          strncasecmp(line, "X-Status:", 9) == 0))
    {
      if (line[1] == '-') m->x_status = lastpos;
      else m->status = lastpos;

      m->stat |= (strchr(line, 'R') ? MSF_READ : 0) |
                 (strchr(line, 'A') ? MSF_REPLIED : 0) |
                 (strchr(line, 'F') ? MSF_MARKED : 0);
    }

    else if (*line == '\n' && m->body == m->header)
    {
      m->body = ftell(mboxf);
      isHeader = false;
    }

    lastpos = ftell(mboxf);
  }

  if (totmsgs)
  {
    msg = new MSG_INFO *[totmsgs];
    m = anchor;
    int i = 0;
    while (m)
    {
      m->end = (m->next ? m->next->top : ftell(mboxf));
      if (m->body == m->header) m->body = m->end;          // no body found
      msg[i++] = m;
      m = m->next;
    }
  }
}


const char *mbox::Status (int stat) const
{
  static char statlines[] = "Status: _O\nX-Status: __\n";

  statlines[8] = (stat & MSF_READ ? 'R' : ' ');
  statlines[21] = (stat & MSF_REPLIED ? 'A' : ' ');
  statlines[22] = (stat & MSF_MARKED ? 'F' : ' ');

  return statlines;
}


void mbox::catastrophe (bool mbox_destroyed, const char *tmpfile) const
{
  char message[131 + MYMAXPATH];

  sprintf(message, "%s%s%s%s",
                   "Could not rewrite mail file. ",
                   (mbox_destroyed ? "It certainly is destroyed now!" :
                                     "It contains junk at its end now!"),
                   "\n       A copy of the original mail file has been "
                                                   "stored as:\n\n       ",
                   canonize(tmpfile));
  fatalError(message);
}


/*
   Unix style mail file (Berkeley format - mbox) reply driver
*/

mbox_reply::mbox_reply (bmail *bm, main_driver *mainDriver)
{
  this->bm = bm;
  this->mainDriver = (mbox *) mainDriver;
  firstReply = currentReply = NULL;
}


mbox_reply::~mbox_reply ()
{
  REPLY *next, *r = firstReply;

  while (r)
  {
    next = r->next;

    delete[] r->header.from;
    delete[] r->header.to;
    delete[] r->header.subject;
    delete[] r->header.date;
    delete[] r->header.inreplyto;
    delete[] r->header.netAddr;
    if (r->filename) remove(r->filename);
    delete[] r->filename;

    delete r;
    r = next;
  }

  delete[] replyBody;
}


const char *mbox_reply::init ()
{
  sprintf(replyPacketName, "%.8s.col", mainDriver->getPacketID());

  if (bm->getServiceType() != ST_REPLY) replyInf();

  return (getReplies() ? NULL : error);
}


inline area_header *mbox_reply::getNextArea ()
{
  return getReplyArea("Mbox Replies");
}


letter_header *mbox_reply::getNextLetter ()
{
  net_address na;

  na.set(currentReply->header.netAddr);

  letter_header *newLetter = new letter_header(bm, NLID,
                                               currentReply->header.from,
                                               currentReply->header.to,
                                               currentReply->header.subject,
                                               currentReply->header.date,
                                               NLID, 0, 1, PRIVATE,
                                               currentReply->latin1,
                                               &na, NULL, NULL,
                                               currentReply->header.inreplyto,
                                               this);

  NLID++;
  currentReply = currentReply->next;

  return newLetter;
}


const char *mbox_reply::getBody (int area, int letter, letter_header *lh)
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


void mbox_reply::resetLetters ()
{
  NLID = 1;
  currentReply = firstReply;
}


inline bool mbox_reply::isLatin1 ()
{
  return mainDriver->isLatin1();
}


void mbox_reply::enterLetter (letter_header *newLetter,
                                  const char *newLetterFileName, long msglen)
{
  REPLY *newReply = new REPLY;

  newReply->header.from = strdupplus(newLetter->getFrom());
  newReply->header.to = strdupplus(newLetter->getTo());
  newReply->header.subject = strdupplus(newLetter->getSubject());
  newReply->header.date = strdupplus(mainDriver->tool->rfc822_date());
  newReply->header.inreplyto = strdupplus(newLetter->getReplyID());
  net_address na = newLetter->getNetAddr();
  newReply->header.netAddr = strdupplus(na.get(true));
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


void mbox_reply::killLetter (int letter)
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


const char *mbox_reply::newLetterBody (int letter, long size)
{
  REPLY *r = firstReply;
  for (int l = 1; l < letter; l++) r = r->next;

  if (size >= 0) r->length = size;
  return r->filename;
}


void mbox_reply::replyInf ()
{
  fcreate(bm->resourceObject->get(InfWorkDir), bm->getLastFileName(false));
  bm->service->createSystemInfFile(0, mainDriver->getPacketID(),
                                      strrchr(replyPacketName, '.') + 1);
}


bool mbox_reply::putReplies ()
{
  bool success = true;

  REPLY *r = firstReply;

  for (int l = 0; l < noOfLetters; l++)
  {
    if (!put1Reply(r, l))
    {
      success = false;
      break;
    }
    r = r->next;
  }

  return success;
}


bool mbox_reply::put1Reply (REPLY *r, int letter)
{
  FILE *reply;
  char *replyText;
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

    success = ((reply = fopen(fname(r->filename), "wb")) != NULL);

    if (success)
    {
      mainDriver->tool->writeHeader(reply, true, letter, r->header.from,
                                    r->header.to, r->header.subject,
                                    r->header.date, r->header.inreplyto,
                                    r->header.netAddr, has8bit);

      if (has8bit && (toupper(*(bm->resourceObject->get(MIMEBody))) == 'Q'))
        success = mainDriver->tool->mimeEncodeBody(reply, replyText);
      else
        success = (fwrite(replyText, sizeof(char), r->length, reply) ==
                                                          (size_t) r->length);

      fclose(reply);
    }
  }

  delete[] replyText;
  return success;
}


bool mbox_reply::getReplies ()
{
  file_list *replies;
  bool success = true;

  if (!(replies = unpackReplies())) return true;

  noOfLetters = replies->getNoOfFiles();

  REPLY seed, *r = &seed;
  seed.next = NULL;

  for (int l = 0; l < noOfLetters; l++)
  {
    r->next = new REPLY;
    r = r->next;
    r->header.from = NULL;
    r->header.to = NULL;
    r->header.subject = NULL;
    r->header.date = NULL;
    r->header.inreplyto = NULL;
    r->header.netAddr = NULL;
    r->filename = NULL;
    r->next = NULL;

    replies->gotoFile(l);

    if (!get1Reply(r, replies->getName()))
    {
      strcpy(error, "Could not get reply.");
      success = false;
      break;
    }
  }

  firstReply = seed.next;

  delete replies;
  if (!success) clearDirectory(".");     // clean up

  return success;
}


bool mbox_reply::get1Reply (REPLY *r, const char *msg)
{
  FILE *msgFile, *reply;
  char filename[13], filepath[MYMAXPATH], *replyText = NULL;
  static int fnr = 1;
  bool success;

  sprintf(filename, "mbox.%03d", fnr++);
  mkfname(filepath, bm->resourceObject->get(WorkDir), filename);

  if (!(msgFile = fopen(msg, "rt"))) return false;

  success = ((reply = fopen(filepath, "wt")) != NULL);

  if (success)
  {
    long msglen = fsize(msg);
    r->latin1 = isLatin1();

    (void) mainDriver->tool->parseHeader(msgFile, msglen, &r->latin1);

    const char *from = mainDriver->tool->getFrom();
    const char *faddr = mainDriver->tool->getFaddr();
    r->header.from = new char[strlen(from) + slen(faddr) + 4];
    char *p = (char *) r->header.from;
    p += sprintf(p, "%s", from);
    if (faddr) sprintf(p, "%s<%s>", (*from ? " " : ""), faddr);

    r->header.to = strdupplus(mainDriver->tool->getTo());
    r->header.subject = strdupplus(mainDriver->tool->getSubject());
    r->header.date = strdupplus(mainDriver->tool->getDate());

    r->header.inreplyto = strdupplus(mainDriver->tool->getInReplyTo());
    if (!r->header.inreplyto)
      r->header.inreplyto = strdupplus(mainDriver->tool->getReferences());

    r->header.netAddr = strdupplus(mainDriver->tool->getTaddr());

    replyText = new char[msglen + 1];
    msglen = fread(replyText, sizeof(char), msglen, msgFile);
    replyText[msglen] = '\0';

    if (mainDriver->tool->isQP()) mainDriver->tool->mimeDecodeBody(replyText);

    msglen = strlen(replyText);
    success = (fwrite(replyText, sizeof(char), msglen, reply) ==
                                                             (size_t) msglen);
    fclose(reply);

    r->filename = strdupplus(filepath);
    r->length = msglen;
  }

  fclose(msgFile);
  delete[] replyText;
  if (success) success = (remove(msg) == 0);

  return success;
}
