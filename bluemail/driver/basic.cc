/*
 * blueMail offline mail reader
 * basic driver definitions

 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <ctype.h>
#include <string.h>
#ifdef __MINGW32__
#include <sys/timeb.h>
#else
#include <sys/time.h>
#endif
#ifndef BIG_ENDIAN
  #define BIG_ENDIAN
#endif
#include "bluewave.h"
#include "../bmail.h"
#include "../../common/auxil.h"


/*
   main driver
*/

main_driver::~main_driver ()
{
}


/*
   reply driver
*/

reply_driver::~reply_driver ()
{
}


/*
   basic main driver
*/

basic_main::basic_main ()
{
  bm = NULL;
  tool = NULL;

  NAID = 0;   // next area ID
              // counts the areas already returned by getNextArea()
  NLID = 0;   // next letter ID
              // counts the letters already returned by getNextLetter()
  currentArea = 0;
  maxAreas = 0;

  persArea = false;

  letterBody = NULL;
  bulletins = NULL;
}


inline int basic_main::getNoOfAreas ()
{
  return maxAreas;
}


void basic_main::selectArea (int area)
{
  currentArea = area;
  resetLetters();
}


inline void basic_main::resetLetters ()
{
  NLID = 0;
}


inline const char **basic_main::getBulletins ()
{
  return bulletins;
}


int basic_main::persOffset () const
{
  return (persArea ? 1 : 0);
}


/*
   basic reply driver
*/

basic_reply::basic_reply ()
{
  bm = NULL;
  NLID = 1;
  noOfLetters = 0;
  replyBody = NULL;
  *replyPacketName = *error = '\0';
}


inline int basic_reply::getNoOfAreas ()
{
  return 1;
}


void basic_reply::selectArea (int area)
{
  // REPLY_AREA has area number 0
  if (area == 0) resetLetters();
}


inline int basic_reply::getNoOfLetters ()
{
  return noOfLetters;
}


inline void basic_reply::initMSF (int **msgstat)
{
  (void) msgstat;
}


inline int basic_reply::readMSF (int **msgstat)
{
  (void) msgstat;
  return 0;
}


inline const char *basic_reply::saveMSF (int **msgstat)
{
  (void) msgstat;
  return NULL;
}


inline const char *basic_reply::getPacketID () const
{
  return NULL;
}


inline const char **basic_reply::getBulletins ()
{
  return NULL;
}


inline bool basic_reply::offlineConfig ()
{
  return false;
}


area_header *basic_reply::getReplyArea (const char *type)
{
  // REPLY_AREA has area number 0
  return new area_header(bm, 0, "REPLY", "REPLY", "Letters written by you",
                         type, A_REPLYAREA | A_LIST, noOfLetters, 0, 0, 0);
}


bool basic_reply::makeReply ()
{
  char rname[MYMAXPATH], sname[MYMAXPATH];
  bool success;

  const char *replyDir = bm->resourceObject->get(ReplyDir);

  mkfname(rname, replyDir, replyPacketName);
  bool oldReplyPacket = (access(rname, R_OK | W_OK) == 0);

  if ((noOfLetters == 0) && !bm->areaList->areaConfig())
  {
    if (oldReplyPacket) return (remove(rname) == 0);
    else return true;
  }

  if (mychdir(bm->resourceObject->get(ReplyWorkDir)) != 0) return false;

  // rename old file, so don't trash old replies if packing fails
  strcpy(sname, rname);
  sname[strlen(sname) - 1] = '$';
  if (oldReplyPacket && (rename(rname, sname) == -1)) return false;

  if ((success = putReplies()))
  {
    // pack all reply files
    bm->service->pack(replyDir, replyPacketName, ALL_FILES_PATTERN);

    if ((success = (access(rname, R_OK | W_OK) == 0))) remove(sname);
  }

  if (!success) rename(sname, rname);

  clearDirectory(".");     // clean up
  return success;
}


file_list *basic_reply::unpackReplies ()
{
  char fname[MYMAXPATH];
  const char *replyWorkDir = bm->resourceObject->get(ReplyWorkDir);
  file_list *replies = NULL;

  mkfname(fname, bm->resourceObject->get(ReplyDir), replyPacketName);

  if (access(fname, R_OK | W_OK) == 0)
    if (bm->service->unpack(fname, replyWorkDir, false) == 0)
      replies = new file_list(replyWorkDir);

  return replies;
}


inline void basic_reply::getConfig ()
{
}


const char *basic_reply::getExtraArea (int &count)
{
  count = 0;
  return NULL;
}


inline void basic_reply::setExtraArea (int extra, bool subscribe,
                                       const char *title)
{
  (void) extra;
  (void) subscribe;
  (void) title;
}


/*
   routines common to more than one driver
*/

basic_tool::basic_tool (bmail *bm, main_driver *driver)
{
  this->bm = bm;
  this->driver = driver;
  msgid = from = faddr = raddr = to = taddr = NULL;
  subject = date = newsgrps = fupto = refs = inreplyto = NULL;
  qpEnc = false;
}


basic_tool::~basic_tool ()
{
  delete[] inreplyto;
  delete[] refs;
  delete[] fupto;
  delete[] newsgrps;
  delete[] date;
  delete[] subject;
  delete[] taddr;
  delete[] to;
  delete[] raddr;
  delete[] faddr;
  delete[] from;
  delete[] msgid;
}


// determine character set according to FidoNet Standards Proposal 0054
// (I need kludge 2, too. Hopefully, nobody will notice or complain.)
void basic_tool::fsc0054 (const char *message, char kludge, bool *latin1)
{
  const char *found;
  static char chrs[] = " CHRS: ";
  static char charset[] = " CHARSET: ";

  chrs[0] = charset[0] = kludge;

  if ((found = strstr(message, chrs))) found += strlen(chrs);
  if (!found && (found = strstr(message, charset))) found += strlen(charset);
  if (found)
  {
    if (strncasecmp(found, "LATIN-1", 7) == 0) *latin1 = true;
    if (strncasecmp(found, "IBMPC", 5) == 0) *latin1 = false;
  }
}


// initialize message status flags
void basic_tool::MSFinit (int **msgstat, int offset,
                          PERSIDX *persIDX, int persmsgs)
{
  int stat, p = 0;

  // skip collection areas
  for (int a = offset; a < driver->getNoOfAreas(); a++)
  {
    driver->selectArea(a);
    for (int l = 0; l < driver->getNoOfLetters(); l++)
    {
      if (p < persmsgs && persIDX[p].area == a && persIDX[p].msgnum == l)
      {
        stat = MSF_PERSONAL;
        p++;
      }
      else stat = 0;

      msgstat[a][l] = stat;
    }
  }
}


// read Blue Wave style XTI file
void basic_tool::XTIread (int **msgstat, int offset, const char *pfname)
{
  FILE *xtifile;
  XTI_REC xti;

  if ((xtifile = bm->fileList->ftryopen(pfname, "rb")))
  {
    // skip collection areas
    for (int a = offset; a < driver->getNoOfAreas(); a++)
    {
      driver->selectArea(a);
      for (int l = 0; l < driver->getNoOfLetters(); l++)
      {
        fscanf(xtifile, "%c%c", &xti.flags, &xti.marks);
        // message status flags match XTI flags
        msgstat[a][l] = (xti.marks << 8) + xti.flags;
      }
    }
    fclose(xtifile);
  }
  else driver->initMSF(msgstat);
}


// write Blue Wave style XTI file
const char *basic_tool::XTIsave (int **msgstat, int offset,
                                 const char *name, bool extxti)
{
  FILE *xtifile;
  static char xtifname[13];
  XTI_REC xti;

  sprintf(xtifname, "%.8s%s", name, (extxti ? ".xti" : ""));

  if ((xtifile = fopen(xtifname, "wb")))
  {
    // skip collection areas
    for (int a = offset; a < driver->getNoOfAreas(); a++)
    {
      driver->selectArea(a);
      for (int l = 0; l < driver->getNoOfLetters(); l++)
      {
        // message status flags match XTI flags
        xti.flags = msgstat[a][l] & 0xFF;
        xti.marks = msgstat[a][l] >> 8;
        fprintf(xtifile, "%c%c", xti.flags, xti.marks);
      }
    }
    fclose(xtifile);
    return xtifname;
  }
  else return NULL;
}


char *basic_tool::getFidoAddr (const char *msg)
{
  char *pos, *origin = strstr((char*)msg, "\n * Origin: ");

  if (origin)
  {
    pos = origin + 1;

    // search end of line
    while (*pos && (*pos != '\n')) pos++;

    // search opening bracket
    while (pos > origin && *pos != '(') pos--;

    // according to FidoNet Standards Proposal 0007 the nodeid must follow
    // the opening bracket, but to handle violations, too, we skip non-digits
    while (*pos && (*pos != '\n') && !is_digit(*pos)) pos++;

    if (is_digit(*pos)) return pos;
  }

  return NULL;
}


void basic_tool::fidodate (char *date, unsigned int size, time_t time)
{
  static char fmt[] = "%d %b %y  %H:%M:%S";     // to avoid warning for %y

  strftime(date, size, fmt, localtime(&time));
}


const char *basic_tool::rfc822_date ()
{
  static char date[40], *zone;

  time_t now = time(NULL);
#ifdef __GLIBC__
  (void) zone;
  strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S %z", localtime(&now));
#else
  strftime(date, sizeof(date), "%a, %d %b %Y %H:%M:%S\n%Z", localtime(&now));

  if ((zone = strrchr(date, '\n')))
  {
    if (zone - date <= (int) sizeof(date) - 7)
    {
#ifdef __MINGW32__
      struct timeb t;
      ftime(&t);
      int mwest = t.timezone - (t.dstflag == 0 ? 0 : 60);
#else
      struct timezone tz;
      gettimeofday(NULL, &tz);
      int mwest = tz.tz_minuteswest;
#endif
      int tzh = -mwest / 60;
      int tzm = mwest % 60;
      sprintf(zone, " %+03d%02d", tzh, mwest >= 0 ? tzm : -tzm);
    }
    else *zone = ' ';
  }
#endif

  return date;
}


const char **basic_tool::bulletin_list (const char file[][13], int count,
                                                               int type)
{
  file_list *fl = bm->fileList;
  int files = 0;
  const char **bulletins = new const char *[fl->getNoOfFiles() + 1];

  for (int i = 0; i < count; i++)
    if (file[i][0]) fl->add(bulletins, file[i], &files);

  if (type & BL_QWK)
  {
    fl->add(bulletins, "blt-*", &files);
    fl->add(bulletins, "session.txt", &files);
  }

  if (type & BL_NEWFILES) fl->add(bulletins, "newfiles.*", &files);

  bulletins[files] = NULL;

  return bulletins;
}


void basic_tool::mimeCharset (const char *buffer, bool *latin1, bool *is8bit)
{
  const char *found, *cset;
  size_t lenc;

  if ((found = strcasestr(buffer, "charset=")))
  {
    found += 8;
    while (*found && (*found == ' ' || *found == '"')) found++;

    int i = 0;
    while ((cset = charsetmap[i++].charset))
    {
      lenc = strlen(cset);

      if ((strncasecmp(found, cset, lenc) == 0) && !is_digit(found[lenc]))
      {
        *latin1 = charsetmap[--i].isLatin1;
        *is8bit = charsetmap[i].is8bit;
        break;
      }
    }
  }
}


bool basic_tool::isQP (const char *source)
{
  char *found;
  static char qph[] = "\n CONTENT-TRANSFER-ENCODING:";
  static char qpb[] = "QUOTED-PRINTABLE\n";

  qph[1] = '\1';
  found = strcasestr(source, qph);
  if (found)
    if (strncasecmp(lcrop(found + 28), qpb, 17) != 0) found = NULL;

  if (!found && (found = strcasestr(source, "\n\1CONTENT-TYPE:")) &&
                (strncasecmp(lcrop(found + 15), "MULTIPART", 9) == 0))
  {
    qph[1] = '\n';
    found = strcasestr(source, qph + 1);
    if (found)
      if (strncasecmp(lcrop(found + 28), qpb, 17) != 0) found = NULL;
  }

  return (found != NULL);
}


bool basic_tool::isUTF8 (const char *source)
{
  char *found;
  bool dummy, is8bit = true;

  if ((found = strcasestr(source, "\n\1CONTENT-TYPE:")))
    mimeCharset(lcrop(found + 15), &dummy, &is8bit);

  return !is8bit;
}


// decode a RFC2045 encoded body
void basic_tool::mimeDecodeBody (char *source)
{
  unsigned char *p = (unsigned char *) source;
  char hex[3];
  unsigned int qp;

  while (*source)
  {
    // skip kludge lines
    if (*source == '\1')
    {
      while (*source && (*source != '\n')) *p++ = *source++;
      continue;
    }

    *p = *source++;

    if (*p == '=')
    {
      // soft line break?
      if (*source == '\n')
      {
        p--;
        source++;
      }
      else if (ishex(source))
      {
        qp = 0;
        hex[0] = *source++;
        hex[1] = *source++;
        hex[2] = '\0';
        sscanf(hex, "%x", &qp);
        if (qp != 0) *p = qp;
      }
    }

    p++;
  }

  *p = '\0';
}


// encode a message body according to RFC2045
bool basic_tool::mimeEncodeBody (FILE *dest, const char *body)
{
  const unsigned char *b = (const unsigned char *) body;
  int col = 0;
  bool success = true;

  while (*b)
  {
    if ((*b & 0x80) || (*b == '='))
    {
      success = (fprintf(dest, "=%02X", *b) == 3);
      col += 3;
    }
    else
    {
      success = (fputc(*b, dest) == *b);
      col++;
    }

    if (*b == '\n') col = 0;
    else if (col > 71 && b[1] != '\n')
    {
      // line must not be longer than 75 characters plus LF
      fprintf(dest, "=\n");
      col = 0;
    }

    if (success) b++;
    else break;
  }

  return success;
}


// decode an UTF-8 / RFC2279 encoded body
void basic_tool::utf8DecodeBody (char *source)
{
  size_t chrs;

  while (*source)
  {
    // skip kludge lines
    if (*source == '\1')
    {
      while (*source && (*source != '\n')) source++;
      if (*source) source++;
      continue;
    }

    chrs = utf8_decode(source, '\n');

    while (*source && (*source != '\n')) source++;
    if (*source)
    {
      source++;
      // shorten body by amount of UTF-8 charcters decoded
      if (chrs) memmove(source, source + chrs, strlen(source + chrs) + 1);
    }
  }
}


// parse a header conforming to RFC822 and RFC1036 stored in a file,
// set charset and return number of headerlines
long basic_tool::parseHeader (FILE *source, long end, bool *latin1)
{
  long headerlines = 0;
  int name, n_len, addr, a_len;
  bool dummy;

  delete[] msgid; msgid = NULL;
  delete[] from; from = NULL;
  delete[] faddr; faddr = NULL;
  delete[] raddr; raddr = NULL;
  delete[] to; to = NULL;
  delete[] taddr; taddr = NULL;
  delete[] subject; subject = NULL;
  delete[] date; date = NULL;
  delete[] newsgrps; newsgrps = NULL;
  delete[] fupto; fupto = NULL;
  delete[] refs; refs = NULL;
  delete[] inreplyto; inreplyto = NULL;

  qpEnc = false;

  while (ftell(source) < end)
  {
    bool wasEOL = false;
    int c = ' ';
    char *fline = NULL;

    // get a header line, including the continuation part(s) if it's folded
    while (ftell(source) < end && (!wasEOL || (c == ' ' || c == '\t')))
    {
      if (fline == line) fline = strdupplus(line);

      // Must be fgets rather than fgetsnl!
      if (!fgets(line, sizeof(line), source)) break;

      bool eol = (strchr(line, '\n') != NULL);

      if (eol) mkstr(line);

      if (fline)
      {
        char *start = line;

        if (wasEOL) start = lcrop(start);

        char *temp = new char[strlen(fline) + strlen(start) + 2];
        sprintf(temp, "%s%s%s", fline, (wasEOL ? " " : ""), start);

        delete[] fline;
        fline = temp;
      }
      else fline = line;

      if (eol && !*fline) break;

      if (eol) headerlines++;

      wasEOL = eol;

      ungetc((c = fgetc(source)), source);
    }

    if (!fline || !*fline) break;

    if (strncasecmp(fline, "Message-ID:", 11) == 0)
      msgid = strdupplus(lcrop(fline + 11));

    else if (strncasecmp(fline, "From:", 5) == 0)
    {
      char *p = lcrop(fline + 5);

      parseAddress(p, name, n_len, addr, a_len);

      if (n_len)
      {
        from = new char[n_len + 1];
        strncpy(from, p + name, n_len);
        from[n_len] = '\0';
      }

      if (a_len)
      {
        faddr = new char[a_len + 1];
        strncpy(faddr, p + addr, a_len);
        faddr[a_len] = '\0';
      }
    }

    else if (strncasecmp(fline, "Reply-To:", 9) == 0)
    {
      char *p = lcrop(fline + 9);

      parseAddress(p, name, n_len, addr, a_len);

      if (a_len)
      {
        raddr = new char[a_len + 1];
        strncpy(raddr, p + addr, a_len);
        raddr[a_len] = '\0';
      }
    }

    else if (strncasecmp(fline, "To:", 3) == 0)
    {
      char *p = lcrop(fline + 3);

      parseAddress(p, name, n_len, addr, a_len);

      int len = (n_len ? n_len : a_len);
      int pos = (n_len ? name : addr);

      if (len)
      {
        to = new char[len + 1];
        strncpy(to, p + pos, len);
        to[len] = '\0';
      }

      if (a_len)
      {
        taddr = new char[a_len + 1];
        strncpy(taddr, p + addr, a_len);
        taddr[a_len] = '\0';
      }
    }

    else if (strncasecmp(fline, "Subject:", 8) == 0)
      subject = strdupplus(lcrop(fline + 8));

    else if (strncasecmp(fline, "Date:", 5) == 0)
      date = strdupplus(lcrop(fline + 5));

    else if (strncasecmp(fline, "Newsgroups:", 11) == 0)
      newsgrps = strdupplus(lcrop(fline + 11));

    else if (strncasecmp(fline, "Followup-To:", 12) == 0)
      fupto = strdupplus(lcrop(fline + 12));

    else if (strncasecmp(fline, "References:", 11) == 0)
      refs = strdupplus(lcrop(fline + 11));

    else if (strncasecmp(fline, "In-Reply-To:", 12) == 0)
      inreplyto = strdupplus(lcrop(fline + 12));

    else if (strncasecmp(fline, "Content-Type:", 13) == 0)
      mimeCharset(lcrop(fline + 13), latin1, &dummy);

    else if (strncasecmp(fline, "Content-Transfer-Encoding:", 26) == 0 &&
             strcasecmp(lcrop(fline + 26), "quoted-printable") == 0)
      qpEnc = true;

    if (fline != line) delete[] fline;
  }

  return headerlines;
}


// return last parseHeader() result
char *basic_tool::getMessageID () const
{
  return msgid;
}


// return last parseHeader() result
char *basic_tool::getFrom () const
{
  return (from ? from : (char *) "");
}


// return last parseHeader() result
char *basic_tool::getFaddr () const
{
  return faddr;
}


// return last parseHeader() result
char *basic_tool::getReplyToAddr () const
{
  return raddr;
}


// return last parseHeader() result
char *basic_tool::getTo () const
{
  return (to ? to : (char *) "");
}


// return last parseHeader() result
char *basic_tool::getTaddr () const
{
  return taddr;
}


// return last parseHeader() result
char *basic_tool::getSubject () const
{
  return (subject ? subject : (char *) "");
}


// return last parseHeader() result
char *basic_tool::getDate () const
{
  return (date ? date : (char *) "");
}


// return last parseHeader() result
char *basic_tool::getNewsgroups () const
{
  return newsgrps;
}


// return last parseHeader() result
char *basic_tool::getFollowupTo () const
{
  return fupto;
}


// return last parseHeader() result
char *basic_tool::getReferences () const
{
  return refs;
}


// return last parseHeader() result
char *basic_tool::getInReplyTo () const
{
  return inreplyto;
}


// return last parseHeader() result
bool basic_tool::isQP () const
{
  return qpEnc;
}


// get a complete line from file
char *basic_tool::fgetsline (FILE *file)
{
  char *temp, *fline = NULL;

  do
  {
    // Must be fgets rather than fgetsnl!
    if (!fgets(line, sizeof(line), file)) break;

    temp = new char[slen(fline) + strlen(line) + 1];
    sprintf(temp, "%s%s", (fline ? fline : ""), line);

    delete[] fline;
    fline = temp;
  }
  while (!strchr(line, '\n'));

  return fline;
}


// create an unique string suitable for message IDs
const char *basic_tool::uniqueID (int nr)
{
  static char id[24];

  sprintf(id, "%08lx.%08lx.bm%03x",
              (long) time(NULL), (long) rand(), nr & 0xfff);
  return id;
}


// encode a message header according to RFC2047
void basic_tool::mimeEncodeHeader (FILE *dest, const char *header)
{
  const char startEnc[] = "=?ISO-8859-1?Q?", endEnc[] = "?=";
  int chrs = 0;
  bool qp = false;

  const unsigned char *h = (const unsigned char *) header;

  while (*h)
  {
    if (!qp && (*h & 0x80))
    {
      chrs += fprintf(dest, startEnc);
      qp = true;
    }

    if (qp)
    {
      if (*h == ' ')
      {
        if (h[1] == '<' || h[1] == '(' || h[1] == '\"')
        {
          fprintf(dest, "%s ", endEnc);
          qp = false;
          chrs = 0;
        }
        else
        {
          fputc('_', dest);
          chrs++;
        }
      }

      else if (*h & 0x80 || *h == '=' || *h == '?' || *h == '_')
        chrs += fprintf(dest, "=%02X", *h);

      else if (*h == '>' || *h == ')' || *h == '\"')
      {
        fprintf(dest, "%s%c", endEnc, *h);
        qp = false;
        chrs = 0;
      }

      else
      {
        fputc(*h, dest);
        chrs++;
      }

      if (chrs > 70 && h[1] != '\0')
      {
        // encoding must not be longer than 75 characters plus LF
        fprintf(dest, "%s\n %s", endEnc, startEnc);
        chrs = strlen(startEnc);
      }
    }
    else
    {
      fputc(*h, dest);
      chrs++;

      if (*h == ' ') chrs = 0;
    }

    h++;
  }

  if (qp) fprintf(dest, endEnc);
}


// write a message header according to RFC822 / RFC1036
void basic_tool::writeHeader (FILE *file, bool isEmail, int nr,
                              const char *from, const char *to_newsgrp,
                              const char *subject, const char *date,
                              const char *refs, const char *netAddr, bool enc)
{
  const char *organization = bm->resourceObject->get(Organization);

  fprintf(file, "Message-ID: <%s%s>\n",
                uniqueID(nr), strchr(bm->resourceObject->get(AddrPart), '@'));

  fprintf(file, "From: ");
  mimeEncodeHeader(file, from);
  fputc('\n', file);

  if (isEmail)
  {
    fprintf(file, "To: ");
    mimeEncodeHeader(file, to_newsgrp);
    if (netAddr && *netAddr) fprintf(file, "%s%s%s\n",
                                           (*to_newsgrp ? " <" : ""),
                                           netAddr,
                                           (*to_newsgrp ? ">" : ""));
    else fputc('\n', file);
  }
  else fprintf(file, "Newsgroups: %s\n", to_newsgrp);

  fprintf(file, "Subject: ");
  mimeEncodeHeader(file, subject);
  fprintf(file, "\nDate: %s\n", date);

  if (refs)
    fprintf(file, (isEmail ? "In-Reply-To: %s\n" : "References: %s\n"), refs);

  fprintf(file, "MIME-Version: 1.0\nContent-Transfer-Encoding: %s\n"
                "Content-Type: text/plain; charset=ISO-8859-1\n",
                (enc ? bm->resourceObject->get(MIMEBody) : "7bit"));

  if (!isEmail && organization && *organization)
    fprintf(file, "Organization: %s\n", organization);

  fprintf(file, (isEmail ? "X-Mailer: %s\n" : "X-Newsreader: %s\n"),
                bm->version());

  fputc('\n', file);
}


// get a BBBS style FidoNet address from a subject line
// (set address and add flags)
void basic_tool::getBBBSAddr (const char *subject, unsigned int zone,
                              net_address &na, int &flags)
{
  int bbbsflag = 0;
  char c = *subject;

  while (c)
  {
    subject++;

    switch (c)
    {
      case '!':
        bbbsflag |= CRASH;
        c = *subject;
        break;

      case '-':
        bbbsflag |= HOLD;
        c = *subject;
        break;

      case '>':
        bbbsflag |= FILEATTACHED;
        c = *subject;
        break;

      case '<':
        bbbsflag |= FILEREQUEST;
        c = *subject;
        break;

      case '/':
        bbbsflag |= KILL;
        c = *subject;
        break;

      default:
        subject--;
        c = '\0';
    }
  }

  na.set(subject);

  if (!na.get() && (zone > 0))
  // possibly, the zone is missing
  {
    char addr[FIDOADDRLEN + 1];

    na.set(NULL);
    sprintf(addr, "%u:", zone);
    strncat(addr, subject, FIDOADDRLEN - 6);
    na.set(addr);

    if (!na.get())
    // no FidoNet style address found
    {
      na.set(NULL);
      return;           // don't change flags
    }
  }

  flags |= bbbsflag;
}
