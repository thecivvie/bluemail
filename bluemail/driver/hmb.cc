/*
 * blueMail offline mail reader
 * Hudson Message Base driver

 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "hmb.h"
#include "../../common/auxil.h"
#include "../../common/error.h"
#include "../../common/mysystem.h"


/*
   Hudson Message Base main driver
*/

hmb::hmb (bmail *bm)
{
  this->bm = bm;
  tool = new basic_tool(bm, this);

  persHIDX = NULL;
  persmsgs = 0;

  persArea = bm->resourceObject->isYes(PersonalArea);
  if (bm->getServiceType() == ST_REPLY) persArea = false;

  body = NULL;
  idx = hdr = txt = NULL;
}


hmb::~hmb ()
{
  while (maxAreas--)
  {
    delete[] body[maxAreas];
    delete[] board[maxAreas].title;
    delete[] board[maxAreas].address;
  }

  delete[] letterBody;
  delete[] body;
  delete[] persHIDX;
  delete tool;

  if (idx) fclose(idx);
  if (hdr) fclose(hdr);
  if (txt) fclose(txt);
}


const char *hmb::init ()
{
  FILE *info;

  const char *username = bm->resourceObject->get(UserName);
  bm->resourceObject->set(LoginName, username);
  bm->resourceObject->set(AliasName, username);
  bm->resourceObject->set(SysOpName, "You");
  bm->resourceObject->set(BBSName, "Hudson Message Base");
  bm->resourceObject->set(hasLoginName, "N");

  for (int i = 0; i < 200; i++) isNetmail[i] = false;

  if (!(info = openFile("info"))) return error;

  size_t rec = fread(&hmb_info, sizeof(hmb_info), 1, info);
  fclose(info);

  if (rec != 1) return "Could not read info file.";

  persmsgs = getPersIndices();          // detects netmail areas, too
  if (persmsgs == -1) return error;

  maxAreas = getAreas();
  body = new bodytype *[maxAreas];
  for (int i = 0; i < maxAreas; i++) body[i] = NULL;

  if (!(txt = openFile("txt")) || !(idx = openFile("idx"))) return error;

  return NULL;
}


area_header *hmb::getNextArea ()
{
  area_header *ah;
  int totmsgs, numpers, flags;

  bool isPersArea = (persArea && (NAID == 0));

  if (isPersArea) totmsgs = numpers = persmsgs;
  else
  {
    totmsgs = getLSBshort(hmb_info.msgs_on_board[board[NAID].number]);
    numpers = board[NAID].numpers;
  }

  body[NAID] = new bodytype[totmsgs];

  flags = A_LIST | (board[NAID].address ? 0 : A_READONLY);

  if (!isPersArea && isNetmail[board[NAID].number]) flags |= A_NETMAIL;

  ah = new area_header(bm, NAID + bm->driverList->getOffset(this),
                       (isPersArea ? "PERS" : board[NAID].shortname),
                       (isPersArea ? "PERSONAL" : board[NAID].shortname),
                       (isPersArea ? "Letters addressed to you"
                                   : board[NAID].title),
                       (isPersArea ? "Hudson Personal" : "Hudson"),
                       flags | (isPersArea ? A_COLLECTION | A_READONLY : 0),
                       totmsgs, numpers, 35, 71);
  NAID++;

  return ah;
}


int hmb::getNoOfLetters ()
{
  if (persArea && (currentArea == 0)) return persmsgs;
  else return getLSBshort(hmb_info.msgs_on_board[board[currentArea].number]);
}


letter_header *hmb::getNextLetter ()
{
  int area, letter;
  HMB_HDR hmb_hdr;
  int len, flags;
  net_address na;
  unsigned int zone;

  bool isPersArea = (persArea && (currentArea == 0));

  if (isPersArea)
  {
    area = persHIDX[NLID].area;
    letter = persHIDX[NLID].letter;
  }
  else
  {
    area = currentArea;
    letter = NLID;
  }

  getNextHdr(&hmb_hdr, area, isPersArea);

  len = hmb_hdr.from_length;
  char *from = new char[len + 1];
  strncpy(from, hmb_hdr.from, len);
  from[len] = '\0';

  // due to Y2K problems, to_length may be destroyed by date
  len = (hmb_hdr.date_length == 9 ? sizeof(hmb_hdr.to) : hmb_hdr.to_length);
  char *to = new char[len + 1];
  strncpy(to, hmb_hdr.to, len);
  to[len] = '\0';

  len = hmb_hdr.subj_length;
  char *subject = new char[len + 1];
  strncpy(subject, hmb_hdr.subject, len);
  subject[len] = '\0';

  len = hmb_hdr.date_length + 2 + hmb_hdr.time_length;
  char *date = new char[len + 1];
  strncpy(date, hmb_hdr.date, hmb_hdr.date_length);
  date[hmb_hdr.date_length] = '\0';
  // due to Y2K problems, the date may be too long
  if (hmb_hdr.date_length == 9)
  {
    date[6] = date[7];
    date[7] = date[8];
    date[8] = date[9];
  }
  strcat(date, "  ");
  strncat(date, hmb_hdr.time, hmb_hdr.time_length);

  flags = (hmb_hdr.msg_attr & HMB_MSG_PRIVATE ? PRIVATE : 0) |
          (hmb_hdr.msg_attr & HMB_MSG_RECEIVED ? RECEIVED : 0) |
          ((hmb_hdr.msg_attr & HMB_MSG_NETMAIL) == 0 &&
           (hmb_hdr.msg_attr & HMB_MSG_LOCAL) != 0 &&
           (hmb_hdr.msg_attr & HMB_MSG_OUTECHO) == 0 ? SENT : 0) |
          (hmb_hdr.msg_attr & HMB_MSG_LOCAL ? LOCAL : 0) |
          (hmb_hdr.net_attr & HMB_NET_KILL ? KILL : 0) |
          (hmb_hdr.net_attr & HMB_NET_SENT ? SENT : 0) |
          (hmb_hdr.net_attr & HMB_NET_FATTACH ? FILEATTACHED : 0) |
          (hmb_hdr.net_attr & HMB_NET_CRASH ? CRASH : 0);

  size_t size = sizeof(HMB_TXT);
  body[area][letter].pointer = getLSBshort(hmb_hdr.start_record) * size;
  size -= sizeof(unsigned char);
  body[area][letter].msglen = getLSBshort(hmb_hdr.records) * size;

  if ((zone = hmb_hdr.orig_zone)) na.set(zone, getLSBshort(hmb_hdr.orig_net),
                                         getLSBshort(hmb_hdr.orig_node), 0);
  // point only available through message (body)

  letter_header *lh = new letter_header(bm, letter, from, to, subject, date,
                                        getLSBshort(hmb_hdr.msgnum),
                                        getLSBshort(hmb_hdr.prev_reply),
                                        area, flags, isLatin1(), &na, NULL,
                                        NULL, NULL, this);

  delete[] from;
  delete[] to;
  delete[] subject;
  delete[] date;

  NLID++;
  return lh;
}


const char *hmb::getBody (int area, int letter, letter_header *lh)
{
  HMB_TXT hmb_txt;
  int kar;

  delete[] letterBody;
  letterBody = new char[body[area][letter].msglen + 1];
  unsigned char *p = (unsigned char *) letterBody;

  fseek(txt, body[area][letter].pointer, SEEK_SET);
  unsigned int chunks = body[area][letter].msglen / (sizeof(HMB_TXT) -
                                                     sizeof(unsigned char));

  while (chunks--)
  {
    if (fread(&hmb_txt, sizeof(HMB_TXT), 1, txt) != 1)
      fatalError("Could not get next letter.");

    for (int c = 0; c < hmb_txt.length; c++)
    {
      kar = hmb_txt.chunk[c];

      // a message shouldn't (must not) contain a 0x00 character
      if (kar == 0) kar = ' ';

      // end of line?
      if (kar == '\r') *p++ = '\n';
      else if (kar != '\n') *p++ = kar;
    }
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


void hmb::resetLetters ()
{
  NLID = 0;
  fseek(hdr, 0, SEEK_SET);
}


inline void hmb::initMSF (int **msgstat)
{
  (void) msgstat;
}


int hmb::readMSF (int **msgstat)
{
  HMB_IDX hmb_idx;
  unsigned int msg;
  unsigned char bidx;
  int a, stat, p = 0;

  unsigned int *l = new unsigned int[maxAreas];
  for (a = 0; a < maxAreas; a++) l[a] = 0;

  while (fread(&hmb_idx, sizeof(HMB_IDX), 1, idx) == 1)
  {
    msg = getLSBshort(hmb_idx.msgnum);

    // if not marked for deletion
    if (msg != 0xFFFF)
    {
      bidx = hmb_idx.board - 1;
      a = getAreaIndex(bidx);

      if (a != -1)
        if (l[a] < getLSBshort(hmb_info.msgs_on_board[bidx]))
        {
          stat = (msg <= board[a].lastread ? MSF_READ : 0);
          if (p < persmsgs && persHIDX[p].msgnum == msg)
          {
            board[a].numpers++;   // count personal messages
            persHIDX[p].area = a;
            persHIDX[p].letter = l[a];
            p++;
            stat |= MSF_PERSONAL;
          }
          msgstat[a][l[a]++] = stat;
        }
    }
  }

  delete[] l;
  return (MSF_READ | MSF_PERSONAL);
}


const char *hmb::saveMSF (int **msgstat)
{
  FILE *file;
  UINT lastread;
  HMB_HDR hmb_hdr;

  memset(&lastread, 0, sizeof(lastread));

  if ((file = bm->fileList->ftryopen("lastread.bbs", "Xr+b")))
  {
    for (int i = 0; i < 200; i++)
      if (fwrite(&lastread, sizeof(lastread), 1, file) != 1)
      {
        fclose(file);
        return NULL;
      }

    for (int a = persOffset(); a < maxAreas; a++)
    {
      selectArea(a);
      memset(&lastread, 0, sizeof(lastread));

      for (int l = 0; l < getNoOfLetters(); l++)
      {
        getNextHdr(&hmb_hdr, a, false);
        if (msgstat[a][l] & MSF_READ)
          memcpy(&lastread, hmb_hdr.msgnum, sizeof(lastread));
      }

      if (fseek(file, board[a].number << 1, SEEK_SET) != 0 ||
          fwrite(&lastread, sizeof(lastread), 1, file) != 1)
      {
        fclose(file);
        return NULL;
      }
    }

    fclose(file);
    return "";
  }
  else return NULL;
}


inline const char *hmb::getPacketID () const
{
  return "hudson";
}


inline bool hmb::isLatin1 ()
{
  return false;
}


inline bool hmb::offlineConfig ()
{
  return false;
}


bool hmb::isNetmailArea (int area) const
{
  if (area >= 0 && area < 200) return isNetmail[area];
  else return false;
}


FILE *hmb::openFile (const char *which)
{
  FILE *file;
  char buf[13];

  sprintf(buf, "msg%s.bbs", which);

  if (!(file = bm->fileList->ftryopen(buf, "xrb")))
    sprintf(error, "Could not lock %s file.", which);

  return file;
}


int hmb::getPersIndices ()
{
  int pers = 0;
  HMB_HDR hmb_hdr;
  struct hIDX
  {
    unsigned int msgnum;
    hIDX *next;
  } *anchor = new hIDX, *pidx = anchor, *i, *next;

  if (!(hdr = openFile("hdr")))
  {
    delete pidx;
    return -1;
  }

  const char *username = bm->resourceObject->get(LoginName);
  int ulen = strlen(username);

  while (fread(&hmb_hdr, sizeof(HMB_HDR), 1, hdr) == 1)
    if ((hmb_hdr.msg_attr & HMB_MSG_DELETED) == 0)
    {
      // detect netmail areas
      if (hmb_hdr.msg_attr & HMB_MSG_NETMAIL)
        isNetmail[hmb_hdr.board - 1] = true;

      // detect personal messages
      if (ulen == hmb_hdr.to_length &&
          strncasecmp(hmb_hdr.to, username, hmb_hdr.to_length) == 0)
          /* || (alen &&          AliasName) */
      {
        pidx->msgnum = getLSBshort(hmb_hdr.msgnum);
        pidx->next = new hIDX;
        pidx = pidx->next;
        pers++;
      }
    }

  persHIDX = new PERSHIDX[pers];
  pers = 0;
  i = anchor;

  while (i != pidx)
  {
    next = i->next;
    persHIDX[pers++].msgnum = i->msgnum;
    delete i;
    i = next;
  }

  delete pidx;

  return pers;
}


int hmb::getAreas ()
{
  FILE *file;
  char *line, *listOfAreas, *areaNo, bbsname[50];
  int areas = persOffset(), bbsnumber;
  static char unavailable[] = "unavailable";
  net_address na;
  UINT lastread;

  if (persArea)
  {
    board[0].number = 255;   // internal pseudo-number for the personal area
    board[0].title = NULL;
    board[0].address = NULL;
  }

  const char *areasHMB =
    (bm->getServiceType() == ST_REPLY ? "areas.hmb"
                                      : bm->resourceObject->get(AreasHMBFile));

  if (areasHMB && (file = fopen(areasHMB, "rt")))
  {
    while ((line = tool->fgetsline(file)))
    {
      mkstr(line);

      if (strncmp(line, "; Netmail: ", 11) == 0)
      {
        listOfAreas = line + 11;

        while ((areaNo = strtok(listOfAreas, " ")))
        {
          bbsnumber = atoi(areaNo);

          if (bbsnumber >= 1 && bbsnumber <= 200)
            isNetmail[bbsnumber - 1] = true;

          listOfAreas = NULL;
        }
      }

      if (*line != ';' && *line != '\n')
      {
        const char *l = line;
        char *arg = stralloc(line);

        // board number
        l = nextArg(l, arg);
        bbsnumber = (l ? atoi(arg) : 0);

        // description
        if (l) l = nextArg(l, arg);
        sprintf(bbsname, "%.49s", l ? arg : unavailable);

        // address
        if (l) l = nextArg(l, arg);
        na.set(l ? arg : NULL);

        if (bbsnumber >= 1 && bbsnumber <= 200)
        {
          board[areas].number = bbsnumber - 1;
          sprintf(board[areas].shortname, "%d", bbsnumber);
          board[areas].title = strdupplus(bbsname);
          board[areas].address = strdupplus(na.get());
          board[areas].lastread = 0;
          board[areas].numpers = 0;
          areas++;
        }

        delete[] arg;
      }

      delete[] line;
    }

    fclose(file);

    if (areas == 0) fatalError("Could not read any area.");
  }
  else
  {
    for (int i = 0, j = persOffset(); i < 200; i++, j++)
    {
      board[j].number = i;
      sprintf(board[j].shortname, "%d", i + 1);
      board[j].title = strdupplus(unavailable);
      board[j].address = NULL;
      board[j].lastread = 0;
      board[j].numpers = 0;
      areas++;
    }
  }

  if ((file = bm->fileList->ftryopen("lastread.bbs", "rb")))
  {
    for (int i = persOffset(); i < areas; i++)
      if (fseek(file, board[i].number << 1, SEEK_SET) == 0)
        if (fread(&lastread, sizeof(lastread), 1, file) == 1)
          board[i].lastread = getLSBshort(lastread);
    fclose(file);
  }

  return areas;
}


void hmb::getNextHdr (HMB_HDR *hmb_hdr, int area, bool isPersArea) const
{
  bool fok;

  while ((fok = (fread(hmb_hdr, sizeof(HMB_HDR), 1, hdr) == 1)))
    if ((hmb_hdr->msg_attr & HMB_MSG_DELETED) == 0)
    {
      if (isPersArea)
      {
        if (getLSBshort(hmb_hdr->msgnum) == persHIDX[NLID].msgnum) break;
      }
      else
      {
        if (hmb_hdr->board == board[area].number + 1) break;
      }
    }

  if (!fok) fatalError("Header file corrupt.");
}


int hmb::getAreaIndex (unsigned char area) const
{
  int a;

  // 255 is an internal pseudo-number for the personal area
  if (area == 255) return 0;

  for (a = persOffset(); a < maxAreas; a++)
    if (board[a].number == area) break;

  return (a == maxAreas ? -1 : a);
}


int hmb::getAreaIndex (const char *title) const
{
  int a;

  for (a = persOffset(); a < maxAreas; a++)
    if (strcasecmp(board[a].title, title) == 0) break;

  return (a == maxAreas ? -1 : a);
}


unsigned char hmb::getBoard (const char *title) const
{
  int a;

  for (a = persOffset(); a < maxAreas; a++)
    if (strcasecmp(board[a].title, title) == 0) break;

  return (a == maxAreas ? 0 : board[a].number + 1);
}


const char *hmb::getBoardAddr (const char *title) const
{
  int a;

  for (a = persOffset(); a < maxAreas; a++)
    if (strcasecmp(board[a].title, title) == 0) break;

  return (a == maxAreas ? NULL : board[a].address);
}


/*
   Hudson Message Base reply driver
*/

hmb_reply::hmb_reply (bmail *bm, main_driver *mainDriver)
{
  this->bm = bm;
  this->mainDriver = (hmb *) mainDriver;
  firstReply = currentReply = NULL;
}


hmb_reply::~hmb_reply ()
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


const char *hmb_reply::init ()
{
  sprintf(replyPacketName, "%.8s.col", mainDriver->getPacketID());
  strcpy(scriptname, "import");
#ifdef SCRIPT_BAT
  strcat(scriptname, ".bat");
#endif

  if (bm->getServiceType() != ST_REPLY) replyInf();

  return (getReplies() ? NULL : error);
}


inline area_header *hmb_reply::getNextArea ()
{
  return getReplyArea("Hudson Replies");
}


letter_header *hmb_reply::getNextLetter ()
{
  net_address na;

  int area = mainDriver->getAreaIndex(currentReply->board - 1);
  area += (area == -1 ? 1 : bm->driverList->getOffset(mainDriver));

  int flags = getLSBshort(currentReply->msgh.attribute) & ~(IMMEDIATE |
                                                            DIRECT |
                                                            UNUSED_M1 |
                                                            UNUSED_M2);
  flags |= (currentReply->flag_direct ? DIRECT : 0);

  na.set(getLSBshort(currentReply->msgh.dest_zone),
         getLSBshort(currentReply->msgh.dest_net),
         getLSBshort(currentReply->msgh.dest_node),
         getLSBshort(currentReply->msgh.dest_point));

  letter_header *newLetter = new letter_header(bm, NLID,
                                               currentReply->msgh.from,
                                               currentReply->msgh.to,
                                               currentReply->msgh.subject,
                                               currentReply->msgh.date,
                                               NLID,
                                               getLSBshort(currentReply->msgh.reply_to),
                                               area, flags, isLatin1(), &na,
                                               NULL, NULL, NULL, this);

  NLID++;
  currentReply = currentReply->next;

  return newLetter;
}


const char *hmb_reply::getBody (int area, int letter, letter_header *lh)
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


void hmb_reply::resetLetters ()
{
  NLID = 1;
  currentReply = firstReply;
}


inline bool hmb_reply::isLatin1 ()
{
  return mainDriver->isLatin1();
}


void hmb_reply::enterLetter (letter_header *newLetter,
                             const char *newLetterFileName, long msglen)
{
  char date[20];
  int flags = LOCAL;

  REPLY *newReply = new REPLY;
  memset(newReply, 0, sizeof(REPLY));

  strncpy(newReply->msgh.from, newLetter->getFrom(), 35);
  strncpy(newReply->msgh.to, newLetter->getTo(), 35);
  strncpy(newReply->msgh.subject, newLetter->getSubject(), 71);

  time_t now = time(NULL);
  mainDriver->tool->fidodate(date, sizeof(date), now);
  strncpy(newReply->msgh.date, date, 20);

  net_address orig_na;   // needed for origin line below, too
  orig_na.set(mainDriver->getBoardAddr(bm->areaList->getTitle()));

  // may not implicitly be needed for echomail, but it doesn't harm either
  // and we need it later to build the MSGID
  putLSBshort(newReply->msgh.orig_zone, orig_na.getZone());
  putLSBshort(newReply->msgh.orig_net, orig_na.getNet());
  putLSBshort(newReply->msgh.orig_node, orig_na.getNode());
  putLSBshort(newReply->msgh.orig_point, orig_na.getPoint());

  if (bm->areaList->isNetmail())
  {
    net_address dest_na = newLetter->getNetAddr();

    putLSBshort(newReply->msgh.dest_zone, dest_na.getZone());
    putLSBshort(newReply->msgh.dest_net, dest_na.getNet());
    putLSBshort(newReply->msgh.dest_node, dest_na.getNode());
    putLSBshort(newReply->msgh.dest_point, dest_na.getPoint());

    flags |= newLetter->getFlags() & ~(IMMEDIATE | DIRECT);
    newReply->flag_direct = ((newLetter->getFlags() & DIRECT) != 0);
  }

  putLSBshort(newReply->msgh.reply_to, newLetter->getReplyTo());
  putLSBshort(newReply->msgh.attribute, flags);

  newReply->board = mainDriver->getBoard(bm->areaList->getTitle());

  // origin line
  if (!bm->areaList->isNetmail())
  {
    FILE *file;
    char *text, origin[80];

    if ((file = fopen(newLetterFileName, "rt")))
    {
      text = new char[msglen + 1];

      if (fread(text, sizeof(char), msglen, file) == (size_t) msglen)
      {
        text[msglen] = '\0';
        strcpy(origin, "\n * Origin: ");

        if (!strstr(text, origin))
        {
          sprintf(origin + 12, "%s (%s)\n", bm->resourceObject->get(Origin),
                                            orig_na.get());
          long olen = strlen(origin);
          int off = (msglen > 0 && text[msglen - 1] == '\n' ? 1 : 0);

          fclose(file);

          if ((file = fopen(newLetterFileName, "wt")))
          {
            fwrite(text, sizeof(char), msglen, file);
            fwrite(origin + off, sizeof(char), olen - off, file);
            msglen += olen - off;
          }
        }
      }

      delete[] text;
      fclose(file);
    }
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


void hmb_reply::killLetter (int letter)
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


const char *hmb_reply::newLetterBody (int letter, long size)
{
  REPLY *r = firstReply;
  for (int l = 1; l < letter; l++) r = r->next;

  if (size >= 0) r->length = size;
  return r->filename;
}


void hmb_reply::replyInf ()
{
  HMB_INFO hmb_info;
  char fname[MYMAXPATH];
  const char *dir = bm->resourceObject->get(InfWorkDir);
  FILE *file;

  memset(&hmb_info, 0, sizeof(hmb_info));
  mkfname(fname, dir, "msginfo.bbs");

  if ((file = fopen(fname, "wb")))
  {
    fwrite(&hmb_info, sizeof(hmb_info), 1, file);
    fclose(file);
  }

  fcreate(dir, "msgidx.bbs");
  fcreate(dir, "msghdr.bbs");
  fcreate(dir, "msgtxt.bbs");

  const char *areasHMB = bm->resourceObject->get(AreasHMBFile);

  if (areasHMB)
  {
    mkfname(fname, dir, "areas.hmb");
    fcopy(areasHMB, fname);

    if ((file = fopen(fname, "at")))
    {
      fprintf(file, "; Netmail:");
      for (int i = 0; i < 200; i++)
        if (mainDriver->isNetmailArea(i)) fprintf(file, " %d", i + 1);
      fprintf(file, "\n");
      fclose(file);
    }
  }

  bm->service->createSystemInfFile((areasHMB ? fmtime(areasHMB) : 0),
                                   mainDriver->getPacketID(),
                                   strrchr(replyPacketName, '.') + 1);
}


bool hmb_reply::putReplies ()
{
  FILE *ctrl;
  bool success = true;

  if (!(ctrl = fopen(scriptname, "wt"))) return false;

  fprintf(ctrl, "%s\n", SH_SCRIPT);

  REPLY *r = firstReply;

  for (int l = 0; l < noOfLetters; l++)
  {
    if (!put1Reply(ctrl, r))
    {
      success = false;
      break;
    }
    r = r->next;
  }

  fclose(ctrl);
  return success;
}


bool hmb_reply::put1Reply (FILE *ctrl, REPLY *r)
{
  FILE *reply;
  char *replyText = NULL, *p, kludges[256], *q, *addr;
  const char *file = fname(r->filename);
  bool success = false;

  if (r->board != 0)
  {
    fprintf(ctrl, myenvf(), "HMBWRITE");
    fprintf(ctrl, " %d %s", r->board, file);
    if (fputc('\n', ctrl) != '\n') return false;

    if (!(reply = fopen(r->filename, "rt"))) return false;

    replyText = new char[r->length + 1];
    success = (fread(replyText, sizeof(char), r->length, reply) ==
                                                          (size_t) r->length);
    fclose(reply);

    if (success)
    {
      replyText[r->length] = '\0';

      if ((success = ((reply = fopen(file, "wb")) != NULL)))
      {
        for (p = replyText; *p; p++)
          if (*p == '\n') *p = '\r';

        // write header
        success = (fwrite(&r->msgh, sizeof(FIDO_MSG_HEADER), 1, reply) == 1);

        // add kludge lines

        p = kludges;
        *kludges = '\0';
        net_address orig_na, dest_na;
        unsigned int pnr;

        orig_na.set(getLSBshort(r->msgh.orig_zone),
                    getLSBshort(r->msgh.orig_net),
                    getLSBshort(r->msgh.orig_node),
                    getLSBshort(r->msgh.orig_point));
        dest_na.set(getLSBshort(r->msgh.dest_zone),
                    getLSBshort(r->msgh.dest_net),
                    getLSBshort(r->msgh.dest_node),
                    getLSBshort(r->msgh.dest_point));

        p += sprintf(p, "\1MSGID: %s %08lx\r", orig_na.get(), (long) time(NULL));
        p += sprintf(p, "\1CHRS: IBMPC 2\r");

        if (dest_na.get())   // netmail
        {
          addr = dest_na.get();
          q = strrchr(addr, '.');
          if (q) *q = '\0';
          p += sprintf(p, "\1INTL %s ", addr);

          addr = orig_na.get();
          q = strrchr(addr, '.');
          if (q) *q = '\0';
          p += sprintf(p, "%s\r", addr);

          if ((pnr = orig_na.getPoint())) p += sprintf(p, "\1FMPT %u\r", pnr);
          if ((pnr = dest_na.getPoint())) p += sprintf(p, "\1TOPT %u\r", pnr);
          if (r->flag_direct) p += sprintf(p, "\1FLAGS DIR\r");

          p += sprintf(p, "\1PID: %s\r", bm->version());
        }

        size_t klen = strlen(kludges);

        if (success && *kludges)
          success = (fwrite(kludges, sizeof(char), klen, reply) == klen);

        // write body
        if (success)
          success = (fwrite(replyText, sizeof(char), r->length, reply) ==
                                                          (size_t) r->length);

        // body shouldn't be empty
        if (success && (r->length == 0) && (klen == 0))
          success = (fputc('\r', reply) == '\r');

        if (success) success = (fputc('\0', reply) == '\0');

        fclose(reply);
      }
    }
  }

  delete[] replyText;
  return success;
}


bool hmb_reply::getReplies ()
{
  file_list *replies;
  const char *ctrlFile = NULL;
  FILE *ctrl = NULL;
  bool success = true;

  if (!(replies = unpackReplies())) return true;

  if (!(ctrlFile = replies->exists(scriptname)) ||
      !(ctrl = replies->ftryopen(ctrlFile, "rt")))
  {
    strcpy(error, "Could not open reply file.");
    success = false;
  }
  else
  {
    noOfLetters = replies->getNoOfFiles() - 1;

    delete[] mainDriver->tool->fgetsline(ctrl);

    REPLY seed, *r = &seed;
    seed.next = NULL;

    for (int l = 0; l < noOfLetters; l++)
    {
      r->next = new REPLY;
      r = r->next;
      r->filename = NULL;
      r->next = NULL;

      if (!get1Reply(ctrl, r))
      {
        strcpy(error, "Could not get reply.");
        success = false;
        break;
      }
    }

    firstReply = seed.next;
  }

  if (ctrl) fclose(ctrl);
  if (ctrlFile) remove(ctrlFile);
  delete replies;
  if (!success) clearDirectory(".");     // clean up

  return success;
}


bool hmb_reply::get1Reply (FILE *ctrl, REPLY *r)
{
  FILE *oldReply, *newReply;
  char filepath[MYMAXPATH], *p;
  bool success = true;

  r->flag_direct = false;

  const char *cmd = mkstr(mainDriver->tool->fgetsline(ctrl)), *l = cmd;
  char *arg = stralloc(cmd);

  // skip $HMBWRITE
  l = nextArg(l, arg);

  l = nextArg(l, arg);
  r->board = (unsigned char) atoi(arg);

  l = nextArg(l, arg);
  const char *filename = fname(arg);

  if ((oldReply = fopen(filename, "rb")))
  {
    success = (fread(&r->msgh, sizeof(FIDO_MSG_HEADER), 1, oldReply) == 1);

    long msglen = fsize(filename) - sizeof(FIDO_MSG_HEADER);
    if (msglen < 0) msglen = 0;

    char *replyText = new char[msglen + 1];
    msglen = fread(replyText, sizeof(char), msglen, oldReply);
    replyText[msglen] = '\0';

    mkfname(filepath, bm->resourceObject->get(WorkDir), filename);

    if (success && (msglen > 0) && (newReply = fopen(filepath, "wt")))
    {
      for (p = replyText; *p; p++)
        if (*p == '\r') *p = '\n';

      // skip kludge lines
      p = replyText;
      while (*p && (*p == '\1'))
      {
        if (strncasecmp(p + 1, "FLAGS DIR\n", 10) == 0) r->flag_direct = true;
        while (*p && (*p != '\n')) p++;
        if (*p) p++;
      }

      msglen = strlen(p);

      success = (fwrite(p, sizeof(char), msglen, newReply) == (size_t) msglen);
      fclose(newReply);

      r->filename = strdupplus(filepath);
      r->length = msglen;
    }
    else success = false;

    fclose(oldReply);
    delete[] replyText;
  }
  else success = false;

  if (success) success = (remove(filename) == 0);

  delete[] arg;
  delete[] cmd;

  return success;
}
