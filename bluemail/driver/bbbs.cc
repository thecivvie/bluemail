/*
 * blueMail offline mail reader
 * BBBS driver

   using some sample code and generous information
   from Kim B. Heino, author of BBBS (http://www.bbbs.net)

 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "bbbs.h"
#include "../../common/auxil.h"
#include "../../common/error.h"
#include "../../common/mysystem.h"


/*
   BBBS main driver
*/

bbbs::bbbs (bmail *bm)
{
  this->bm = bm;
  tool = new basic_tool(bm, this);

  areas = NULL;
  persIDX = NULL;
  persmsgs = 0;

  persArea = bm->resourceObject->isYes(PersonalArea);
  if (bm->getServiceType() == ST_REPLY) persArea = false;

  user0 = !bm->resourceObject->isYes(BBBSUser_1);

  body = NULL;
  unpacked = NULL;
  msgdat = NULL;
}


bbbs::~bbbs ()
{
  while (maxAreas--)
  {
    delete[] body[maxAreas];
    delete[] areas[maxAreas].title;
  }

  free(letterBody);
  delete[] body;
  delete[] persIDX;
  delete[] areas;
  delete tool;

  if (msgdat) fclose(msgdat);
}


const char *bbbs::init ()
{
  char sysop[9];

  sprintf(sysop, "You (#%d)", (user0 ? 0 : 1));

  const char *username = bm->resourceObject->get(UserName);
  bm->resourceObject->set(LoginName, username);
  bm->resourceObject->set(AliasName, username);
  bm->resourceObject->set(SysOpName, sysop);
  bm->resourceObject->set(BBSName, "BBBS Message Base");
  bm->resourceObject->set(hasLoginName, "N");

  if (!getAreas()) return error;

  if ((persmsgs = buildIndices()) == -1) return error;

  return NULL;
}


area_header *bbbs::getNextArea ()
{
  area_header *ah;
  char areanum[12];

  bool isPersArea = (persArea && (NAID == 0));

  body[NAID] = new BODYTYPE[areas[NAID].totmsgs];

  sprintf(areanum, "%d", NAID - persOffset());

  int flags = A_LIST;
  unlong st = areas[NAID].status;

  if (isPersArea) flags |= A_COLLECTION | A_READONLY;
  else flags |= (isNetmail(NAID) ? A_NETMAIL : 0) |
                (isFido(NAID) ? 0 : A_INTERNET) |
                (st & stat_alias ? A_ALIAS : 0) |
                (st & (stat_allowpriv | stat_postarea) ? A_ALLOWPRIV : 0);

  ah = new area_header(bm, NAID + bm->driverList->getOffset(this),
                       (isPersArea ? "PERS" : areanum),
                       (isPersArea ? "PERSONAL" : areas[NAID].title),
                       (isPersArea ? "Letters addressed to you"
                                   : areas[NAID].title),
                       (isPersArea ? "BBBS Personal" : "BBBS"),
                       flags, areas[NAID].totmsgs, areas[NAID].numpers,
                       71, 71);
  NAID++;

  return ah;
}


inline int bbbs::getNoOfLetters ()
{
  return areas[currentArea].totmsgs;
}


letter_header *bbbs::getNextLetter ()
{
  int area, letter;
  msgrec msgRec;
  char date[20];
  const char *from;
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

  if (!openFile(area - persOffset(), DAT)) fatalError(error);

  fseek(msgdat, letter * sizeof(msgrec), SEEK_SET);
  if (fread(&msgRec, sizeof(msgrec), 1, msgdat) != 1)
    fatalError("Could not get next letter.");

  // unlock .dat file
  if (NLID == areas[currentArea].totmsgs - 1)
  {
    fclose(msgdat);
    msgdat = NULL;
  }

  body[area][letter].extraline = ((msgRec.status & mstat_extraline) != 0);
  body[area][letter].lines = msgRec.lines;
  body[area][letter].pointer = (long) msgRec.offset;

  sprintf(date,
          "%02u.%02u.%04u %02u:%02u:%02u", bun_day(msgRec.dated),
                                           bun_month(msgRec.dated),
                                           bun_year(msgRec.dated) + 1900,
                                           bun_hour(msgRec.dated),
                                           bun_minute(msgRec.dated),
                                           bun_second(msgRec.dated));

  int flags = (msgRec.status & mstat_private ? PRIVATE : 0) |
              (msgRec.status & mstat_readed ? RECEIVED : 0);

  if (isNetmail(area) && !isFido(area)) // isEmail
  {
    from = "";
    na.set(msgRec.msgfrom);
  }
  else
  {
    from = msgRec.msgfrom;
    na.set(msgRec.zonefrom, msgRec.netfrom, msgRec.nodefrom, msgRec.pointfrom);

    // add flags
    if (isNetmail(area))
    {
      net_address dummy;
      tool->getBBBSAddr(msgRec.subject, 0, dummy, flags);
    }
  }

  NLID++;
  letter_header *lh = new letter_header(bm, letter, from, msgRec.msgto,
                                        msgRec.subject, date,
                                        (int) msgRec.number,
                                        (int) msgRec.replyto,
                                        area, flags, isLatin1(), &na,
                                        NULL, NULL, NULL, this);

  if (msgRec.status & mstat_killed) lh->setMarked();

  return lh;
}


// file access according to Kim B. Heino's msgview.c sample code
const char *bbbs::getBody (int area, int letter, letter_header *lh)
{
  char data[128];
  const char kludge[] = "\1";

  free(letterBody);
  letterBody = NULL;

  if (!openFile(area - persOffset(), TXT)) fatalError(error);
  fseek(msgdat, body[area][letter].pointer, SEEK_SET);

  if (body[area][letter].extraline)     // a kludge for kludges
    for (;;)
    {
      *data = '\0';
      fread(data, 1, 1, msgdat);
      if (!*data) break;
      unpacked = (char *) kludge;
      addLetterBody(-1);
      fread(data + 1, 1, (byte) data[0] & 127, msgdat);
      addLetterBody(unpack((byte *) data));
    }

  for (unlong i = 0; i < body[area][letter].lines;)     // message text
  {
    *data = '\0';
    fread(data, 1, 1, msgdat);
    if (*data) fread(data + 1, 1, (byte) data[0] & 127, msgdat);
    addLetterBody(unpack((byte *) data));
    if (*unpacked != '\1') i++;
  }

  // unlock .txt file
  fclose(msgdat);
  msgdat = NULL;

  char *p = strrchr(letterBody, '\0');

  do p--;
  // strip blank lines
  while (p >= letterBody && (*p == ' ' || *p == '\n'));

  *++p = '\0';

  // try to find a 'From: ' line
  if (!isFido(area))
  {
    static char from[] = "\nFrom: ";
    char *fromline = letterBody, *nl;

    if (strncmp(fromline, from + 1, 6) == 0 ||
        (fromline = strstr(letterBody, from)))
    {
      int name, n_len, addr, a_len;

      if ((nl = strchr(fromline + 6, '\n'))) *nl = '\0';
      // parseAddress() needs a null-terminated string
      parseAddress(fromline + 6, name, n_len, addr, a_len);

      // get sender's name from e-mail
      if (isNetmail(area))
      {
        if (n_len)
        {
          char *fname = new char[n_len + 1];
          strncpy(fname, fromline + 6 + name, n_len);
          fname[n_len] = '\0';
          lh->setFrom(fname);
          delete[] fname;
        }
      }
      // get sender's address from news
      else
      {
        if (a_len)
        {
          char *faddr = new char[a_len + 1];
          strncpy(faddr, fromline + 6 + addr, a_len);
          faddr[a_len] = '\0';
          net_address &na = lh->getNetAddr();
          na.set(faddr);
          delete[] faddr;
        }
      }

      if (nl) *nl = '\n';
    }
  }

  return letterBody;
}


inline void bbbs::initMSF (int **msgstat)
{
  (void) msgstat;
}


int bbbs::readMSF (int **msgstat)
{
  userconfstat usrcfgstat;
  int stat, p = 0;

  FILE *file = bm->fileList->ftryopen("areausr4.dat", "xrb");

  if (file && !user0)
    fseek(file, (maxAreas - persOffset()) * sizeof(userconfstat), SEEK_SET);

  for (int a = persOffset(); a < maxAreas; a++)
  {
    memset(&usrcfgstat, 0, sizeof(userconfstat));

    if (file) fread(&usrcfgstat, sizeof(userconfstat), 1, file);

    for (int l = 0; l < areas[a].totmsgs; l++)
    {
      stat = (areas[a].first + l <= usrcfgstat.lastread ? MSF_READ : 0);

      if (p < persmsgs && persIDX[p].area == a && persIDX[p].msgnum == l)
      {
        p++;
        stat |= MSF_PERSONAL;
      }

      msgstat[a][l] = stat;
    }
  }

  if (file) fclose(file);

  return (MSF_READ | MSF_PERSONAL);
}


const char *bbbs::saveMSF (int **msgstat)
{
  FILE *file;

  size_t noOfAreas = maxAreas - persOffset();
  long seekpos = (user0 ? 0 : noOfAreas * sizeof(userconfstat));

  if ((file = bm->fileList->ftryopen("areausr4.dat", "Xr+b")))
  {
    userconfstat *usrcfgstat = new userconfstat[noOfAreas];

    if (fseek(file, seekpos, SEEK_SET) != 0 ||
        fread(usrcfgstat, sizeof(userconfstat), noOfAreas, file) != noOfAreas)
    {
      delete[] usrcfgstat;
      fclose(file);
      return NULL;
    }

    for (int a = persOffset(); a < maxAreas; a++)
    {
      usrcfgstat[a - persOffset()].lastread = 0;

      for (int l = 0; l < areas[a].totmsgs; l++)
        if (msgstat[a][l] & MSF_READ)
          usrcfgstat[a - persOffset()].lastread = areas[a].first + l;
        else break;
    }

    if (fseek(file, seekpos, SEEK_SET) != 0 ||
        fwrite(usrcfgstat, sizeof(userconfstat), noOfAreas, file) != noOfAreas)
    {
      delete[] usrcfgstat;
      fclose(file);
      return NULL;
    }

    delete[] usrcfgstat;
    fclose(file);
    return "";
  }
  else return NULL;
}


inline const char *bbbs::getPacketID () const
{
  return "bbbs";
}


inline bool bbbs::isLatin1 ()
{
  // The character set of a message stored into BBBS is *always* IBM.
  // If BBBS can determine the character set from a Fido CHRS kludge
  // or an Internet content transfer encoding / type description, it
  // converts from the given character set into IBM.
  // The SysOp should properly configure import character sets for
  // conferences, which will be used in case there is no character
  // set hint inside the message. It will then be converted into IBM
  // using this import character set.
  return false;
}


inline bool bbbs::offlineConfig ()
{
  return false;
}


bool bbbs::getAreas ()
{
  FILE *areacfg;
  unlong count_of_confs;
  confrec1 confRec1;
  confrec2 confRec2;
  bool success = true;

  if (!(areacfg = bm->fileList->ftryopen("areacfg4.dat", "xrb")) ||
      (fread(&count_of_confs, sizeof(count_of_confs), 1, areacfg) != 1) ||
      (fseek(areacfg, sizeof(cfgrec2), SEEK_SET) != 0))
  {
    strcpy(error, "Could not access area config file.");
    if (areacfg) fclose(areacfg);
    return false;
  }

  // we're somewhat limited here
  maxAreas = (int) count_of_confs + persOffset();

  body = new BODYTYPE *[maxAreas];
  areas = new BBBS_AREA_INFO[maxAreas];
  for (int i = 0; i < maxAreas; i++)
  {
    body[i] = NULL;
    areas[i].title = NULL;
    areas[i].first = 0;
  }

  if (persArea) areas[0].totmsgs = areas[0].numpers = 0;

  for (int a = persOffset(); a < maxAreas; a++)
    if (fread(&confRec1, sizeof(confrec1), 1, areacfg) != 1 ||
        fread(&confRec2, sizeof(confrec2), 1, areacfg) != 1)
    {
      strcpy(error, "Could not read area config file.");
      success = false;
      break;
    }
    else
    {
      areas[a].title = strdupplus(confRec1.confname);
      areas[a].status = confRec1.status | (*confRec2.nntpname ? stat_nntp : 0);
    }

  fclose(areacfg);

  return success;
}


char *bbbs::getConfFile (int area)
{
  static char areafile[13];

  sprintf(areafile, "%.8x", area);
  return areafile;
}


bool bbbs::openFile (int area, filetype ftyp)
{
  static int lastArea;
  static filetype lastFtyp;

  if (!msgdat)
  {
    // initialize static variables
    lastArea = -1;
    lastFtyp = NONE;
  }

  if (lastArea != area || lastFtyp != ftyp)
  {
    lastArea = area;
    lastFtyp = ftyp;

    if (msgdat) fclose(msgdat);
    msgdat = NULL;

    char *areafile = getConfFile(area);
    strcat(areafile, (ftyp == DAT ? ".dat" : ".txt"));

    if (!(msgdat = bm->fileList->ftryopen(areafile, "xrb")))
    {
      sprintf(error, "Could not open %s file.", areafile);
      return false;
    }
  }

  return true;
}


int bbbs::buildIndices ()
{
  int msgs, pmsgs, pers = 0;
  msgrec msgRec;
  pIDX anchor, *pidx = &anchor, *d;

  bool success = true;
  const char *username = bm->resourceObject->get(LoginName);

  for (int a = persOffset(); a < maxAreas; a++)
  {
    // real areas (BBBS conferences)
    if (!openFile(a - persOffset(), DAT))
    {
      success = false;
      break;
    }

    msgs = pmsgs = 0;

    while (fread(&msgRec, sizeof(msgrec), 1, msgdat) == 1)
    {
      if (msgs == 0) areas[a].first = msgRec.number;

      msgs++;

      if (strcasecmp(username, msgRec.msgto) == 0)
          /* ||      AliasName */
      {
        pers++;
        pmsgs++;
        pidx->next = new pIDX;
        pidx = pidx->next;
        pidx->persidx.area = a;
        pidx->persidx.msgnum = msgs - 1;
        pidx->next = NULL;
      }
    }

    areas[a].totmsgs = msgs;
    areas[a].numpers = pmsgs;
  }

  // unlock .dat file
  if (msgdat) fclose(msgdat);
  msgdat = NULL;

  if (pers)
  {
    persIDX = new PERSIDX[pers];
    pidx = anchor.next;
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

  return (success ? pers : -1);
}


// pre-generated Huffman tree (only the part from index 223 to 443)
// according to Kim B. Heino's msgview.c sample code
bbbs::TREE bbbs::tree[] =
{
  {120, 170}, {175, 211}, {138, 174}, {179, 195}, {135, 157}, {183, 204},
  {205, 220}, {131, 148}, {166, 202}, {223, 224}, {132, 152}, {167, 171},
  {180, 182}, {197, 209}, {213, 178}, {184, 194}, {199, 201}, {225, 226},
  {123, 125}, {151, 177}, {206, 207}, {221, 134}, {203, 227}, {228, 229},
  {137, 140}, {158, 149}, {150, 181}, {212, 230}, {231, 232}, { 96, 103},
  {129, 172}, {176, 185}, {233, 234}, {235, 236}, {127, 133}, {159, 237},
  {101, 238}, {239, 240}, {141, 198}, {126, 156}, {241, 242}, {243, 107},
  {153, 155}, {169, 244}, {210, 245}, {246, 114}, {186, 115}, {118, 124},
  {247, 248}, {106, 163}, {214, 249}, {250, 251}, {252, 104}, {117, 253},
  {254, 255}, {256, 119}, {122, 168}, {215, 105}, {257, 258}, {109, 160},
  {259, 260}, {192, 261}, {142, 262}, {263, 264}, {265, 266}, {130, 267},
  {268, 269}, {270, 128}, {271, 139}, {272, 273}, {274, 217}, {108, 275},
  {276, 277}, {278, 165}, {279, 280}, {281, 282}, {283, 208}, {284, 136},
  {219,  97}, {285, 286}, {113, 287}, {288, 289}, {161, 290}, {291, 162},
  {292, 193}, {293, 294}, {295, 296}, {297, 196}, {298, 299}, {300,  98},
  {301, 302}, {303, 189}, {218, 154}, { 99, 304}, {305, 190}, {306, 307},
  {308, 309}, {310, 311}, {200, 312}, { 93, 313}, {314,  32}, {315, 316},
  {317, 318}, {188,  92}, {319, 320}, { 61, 144}, { 59, 321}, {322, 216},
  {191, 323}, { 64,  91}, {324, 146}, {325, 326}, {327, 328}, {  4, 329},
  {330, 145}, {102, 111}, {331, 147}, {332, 187}, {333, 112}, { 60, 334},
  {173, 121}, {335,   5}, {  6, 336}, {222, 337}, {338, 143}, {339,  62},
  {164, 340}, { 81, 341}, { 58,  94}, { 28, 342}, { 49, 343}, {344, 345},
  {346, 110}, {347,  56}, {348,  90}, {349, 350}, {351, 352}, { 27, 353},
  {354,  57}, {355, 356}, { 11, 357}, {358,   2}, { 55,   3}, {359,  88},
  {360,  53}, { 63,  39}, {361,  23}, { 29,  54}, { 38,   7}, {362,  22},
  {363,  24}, {364, 365}, {  1, 366}, {116,  42}, { 36,  43}, {367,  44},
  { 40,  31}, { 35,  50}, { 20,  25}, {368,  46}, { 21,  48}, { 15, 369},
  {370,  87}, { 19, 371}, {372, 373}, {  8, 374}, { 37,  34}, { 41,  66},
  {375,   9}, {376,  10}, { 47,  70}, { 33, 377}, {378, 379}, { 52, 380},
  { 45, 381}, {382, 383}, { 67, 384}, { 26,  17}, {385, 386}, { 30,  71},
  { 16, 387}, { 51, 388}, {389,  12}, { 18, 390}, {391, 392}, {393, 394},
  { 74, 395}, {396, 397}, {398,  68}, {399, 400}, { 86,  89}, { 80, 401},
  {402, 403}, { 13, 404}, {405, 406}, { 72, 407}, {408, 409}, { 14, 410},
  {411,  77}, {100, 412}, { 82, 413}, {414, 415}, {416,  75}, { 85, 417},
  {418, 419}, { 76, 420}, {421,  79}, {422,  83}, {423, 424}, {425,  78},
  {426,  69}, { 84, 427}, { 65,  73}, {428, 429}, {430, 431}, {432, 433},
  {434, 435}, {436, 437}, {  0, 438}, {439, 440}, {441, 442}
};


// according to Kim B. Heino's msgview.c sample code
int bbbs::unpack (byte *data)
{
  static char buff[128];

  if (*data < 128)   // not packed, first char (len) is < 128
  {
    data[*data + 1] = '\0';
    unpacked = (char *) data + 1;
    return *data;
  }
  else   // packed, len is >= 128 (actual packed len is (len - 128) * 8 bits)
  {
    int b, i, w, p;
    char c = 0;

    for (i = 1, w = 443, p = 0, b = (int) (*data & 127) << 3; b; b--)
    {
      if (!(b & 7)) c = data[i++];

      if (c & 1) w = tree[w - 223].c1;
      else w = tree[w - 223].c0;

      if (w < 223)
      {
        buff[p++] = w + 32;
        w = 443;
      }

      c >>= 1;
    }

    buff[p] = '\0';
    unpacked = buff;
    return p;
  }
}


void bbbs::addLetterBody (int len)
{
  int const ALLOCSIZE = 1024;
  static long alloclen, msglen;

  if (!letterBody)
  {
    letterBody = (char *) malloc(ALLOCSIZE);
    msglen = 0;

    if (letterBody)
    {
      *letterBody = '\0';
      alloclen = ALLOCSIZE;
    }
    else alloclen = 0;
  }

  bool lf = (len >= 0);

  if (lf) len++;       // because we have to add \n
  else len = -len;

  if (alloclen - msglen - 1 < len)
  {
    char *lb = (char *) realloc(letterBody, alloclen + ALLOCSIZE);

    if (lb)
    {
      letterBody = lb;
      alloclen += ALLOCSIZE;
    }
    else return;
  }

  sprintf(letterBody + msglen, (lf ? "%s\n" : "%s"), unpacked);
  msglen += len;
}


int bbbs::getAreaIndex (const char *title) const
{
  int a;

  for (a = persOffset(); a < maxAreas; a++)
    if (strcasecmp(areas[a].title, title) == 0) break;

  return (a == maxAreas ? -1 : a);
}


bool bbbs::isFido (int area) const
{
  return (areas[area].status & stat_nntp) == 0;
}


bool bbbs::isNetmail (int area) const
{
  return (areas[area].status & stat_postarea) != 0 &&
         (areas[area].status & (stat_fidoarea | stat_nntp)) != 0;
}


/*
   BBBS reply driver
*/

bbbs_reply::bbbs_reply (bmail *bm, main_driver *mainDriver)
{
  this->bm = bm;
  this->mainDriver = (bbbs *) mainDriver;
  firstReply = currentReply = NULL;
}


bbbs_reply::~bbbs_reply ()
{
  REPLY *next, *r = firstReply;

  while (r)
  {
    next = r->next;

    delete[] r->header.from;
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


const char *bbbs_reply::init ()
{
  sprintf(replyPacketName, "%.8s.col", mainDriver->getPacketID());
  strcpy(scriptname, "import");
#ifdef SCRIPT_BAT
  strcat(scriptname, ".bat");
#endif

  if (bm->getServiceType() != ST_REPLY) replyInf();

  return (getReplies() ? NULL : error);
}


inline area_header *bbbs_reply::getNextArea ()
{
  return getReplyArea("BBBS Replies");
}


letter_header *bbbs_reply::getNextLetter ()
{
  net_address na;

  int area = mainDriver->getAreaIndex(currentReply->header.areatag);
  area += (area == -1 ? 1 : bm->driverList->getOffset(mainDriver));

  int flags = currentReply->header.flags;

  if (currentReply->header.netAddr)
  {
    flags |= PRIVATE;
    na.set(currentReply->header.netAddr);
  }

  letter_header *newLetter = new letter_header(bm, NLID,
                                               currentReply->header.from,
                                               currentReply->header.to,
                                               currentReply->header.subject,
                                               "", NLID, 0, area, flags,
                                               isLatin1(), &na,
                                               NULL, NULL, NULL, this);

  NLID++;
  currentReply = currentReply->next;

  return newLetter;
}


const char *bbbs_reply::getBody (int area, int letter, letter_header *lh)
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


void bbbs_reply::resetLetters ()
{
  NLID = 1;
  currentReply = firstReply;
}


inline bool bbbs_reply::isLatin1 ()
{
  return mainDriver->isLatin1();
}


void bbbs_reply::enterLetter (letter_header *newLetter,
                              const char *newLetterFileName, long msglen)
{
  REPLY *newReply = new REPLY;

  newReply->header.from = strdupplus(newLetter->getFrom());
  newReply->header.to = strdupplus(newLetter->getTo());
  newReply->header.subject = strdupplus(newLetter->getSubject());
  newReply->header.areatag = strdupplus(bm->areaList->getTitle());
  newReply->header.flags = (newLetter->isPrivate() ? PRIVATE : 0);
  newReply->header.netAddr = NULL;

  if (bm->areaList->isNetmail())
  {
    newReply->header.flags |= newLetter->getFlags() & (CRASH | FILEATTACHED |
                                                       KILL | HOLD |
                                                       FILEREQUEST);
    net_address na = newLetter->getNetAddr();
    newReply->header.netAddr = strdupplus(na.get(bm->areaList->isInternet()));
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


void bbbs_reply::killLetter (int letter)
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


const char *bbbs_reply::newLetterBody (int letter, long size)
{
  REPLY *r = firstReply;
  for (int l = 1; l < letter; l++) r = r->next;

  if (size >= 0) r->length = size;
  return r->filename;
}


void bbbs_reply::replyInf ()
{
  char fname[MYMAXPATH];

  const char *areacfg = bm->fileList->exists("areacfg4.dat");
  const char *dir = bm->resourceObject->get(InfWorkDir);

  mkfname(fname, bm->resourceObject->get(InfWorkDir), "areacfg4.dat");
  fcopy(areacfg, fname);

  int noOfAreas = mainDriver->getNoOfAreas() - mainDriver->persOffset();

  for (int a = 0; a < noOfAreas; a++)
  {
    sprintf(fname, "%s.dat", mainDriver->getConfFile(a));
    fcreate(dir, fname);
  }

  bm->service->createSystemInfFile(fmtime(areacfg),
                                   mainDriver->getPacketID(),
                                   strrchr(replyPacketName, '.') + 1);
}


void bbbs_reply::quotation_out (FILE *stream, const char *string)
{
  char c;

  fputc('"', stream);

  if (string)
  {
    while ((c = *string++))
    {
      if (c == '"') fputc('\\', stream);
      fputc(c, stream);
    }
  }

  fputc('"', stream);
}


bool bbbs_reply::putReplies ()
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


bool bbbs_reply::put1Reply (FILE *ctrl, REPLY *r)
{
  bool success = false;
  int a = mainDriver->getAreaIndex(r->header.areatag);

  if (a != -1)
  {
    const char *file = fname(r->filename);
    bool isFido = mainDriver->isFido(a);
    bool isNetmail = mainDriver->isNetmail(a);

    fprintf(ctrl, myenvf(), "BBBSDIR");
    fprintf(ctrl, canonize("/%s"), "bbbs btxt2bbs ");
    quotation_out(ctrl, r->header.areatag);
    fputc(' ', ctrl);
    fprintf(ctrl, myenvf(), "PWD");
    fprintf(ctrl, canonize("/%s"), file);
    fprintf(ctrl, " /F ");
    quotation_out(ctrl, r->header.from);
    fprintf(ctrl, " /T ");

    const char *to = r->header.to;
    char *addr = NULL;

    if (isNetmail && !isFido && r->header.netAddr) // isEmail
    {
      addr = new char[strlen(r->header.to) + strlen(r->header.netAddr) + 4];
      sprintf(addr, "%s <%s>", r->header.to, r->header.netAddr);

      if (strlen(addr) <= 71) to = addr;
      else to = r->header.netAddr;
    }

    quotation_out(ctrl, to);

    fprintf(ctrl, " /S ");
    const char *subject = r->header.subject;
    char *subj = NULL;

    if (isFido && isNetmail && r->header.netAddr)
    {
      subj = new char[strlen(subject) + strlen(r->header.netAddr) + 7];
      *subj = '\0';

      if (r->header.flags & CRASH) strcat(subj, "!");
      if (r->header.flags & HOLD) strcat(subj, "-");
      if (r->header.flags & FILEATTACHED) strcat(subj, ">");
      if (r->header.flags & FILEREQUEST) strcat(subj, "<");
      if (r->header.flags & KILL) strcat(subj, "/");

      strcat(subj, r->header.netAddr);
      strcat(subj, " ");
      strcat(subj, subject);
      subject = subj;
    }

    quotation_out(ctrl, subject);

    success = (fputc('\n', ctrl) == '\n' && fcopy(r->filename, file) == 0);

    delete[] subj;
    delete[] addr;
  }

  return success;
}


bool bbbs_reply::getReplies ()
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


bool bbbs_reply::get1Reply (FILE *ctrl, REPLY *r)
{
  FILE *reply;
  char filepath[MYMAXPATH], *s = NULL;
  bool success = true;

  r->header.from = NULL;
  r->header.to = NULL;
  r->header.subject = NULL;
  r->header.flags = 0;
  r->header.netAddr = NULL;

  const char *cmd = mkstr(mainDriver->tool->fgetsline(ctrl)), *l = cmd;
  char *arg = stralloc(cmd);

  // skip $BBBSDIR/bbbs btxt2bbs
  l = nextArg(l, arg);
  l = nextArg(l, arg);

  l = nextArg(l, arg);
  r->header.areatag = strdupplus(arg);

  l = nextArg(l, arg);
  const char *filename = strdupplus(fname(arg));

  // get /F, /T and /S
  while ((l = nextArg(l, arg)))
  {
    if (strcmp(arg, "/F") == 0)
    {
      l = nextArg(l, arg);
      r->header.from = strdupplus(arg);
    }
    else if (strcmp(arg, "/T") == 0)
    {
      l = nextArg(l, arg);
      r->header.to = strdupplus(arg);
    }
    else if (strcmp(arg, "/S") == 0)
    {
      l = nextArg(l, arg);
      r->header.subject = strdupplus(arg);
    }
  }

  int a = mainDriver->getAreaIndex(r->header.areatag);

  if ((a != -1) && mainDriver->isNetmail(a))
  // get netAddr (and flags)
  {
    if (mainDriver->isFido(a))
    {
      net_address na;
      const char *subj = r->header.subject;

      mainDriver->tool->getBBBSAddr(subj, 0, na, r->header.flags);

      if ((r->header.netAddr = strdupplus(na.get())) && (s = strchr((char*)subj, ' ')))
        memmove((char *) subj, s + 1, strrchr(subj, '\0') - s);
    }
    else
    {
      int name, n_len, addr, a_len;

      parseAddress(r->header.to, name, n_len, addr, a_len);

      if (a_len)
      {
        char *taddr = new char[a_len + 1];
        strncpy(taddr, r->header.to + addr, a_len);
        taddr[a_len] = '\0';
        r->header.netAddr = strdupplus(taddr);
        delete[] taddr;
      }

      if (n_len)
      {
        memmove((char *) r->header.to, r->header.to + name, n_len);
        *((char *) r->header.to + n_len) = '\0';
      }
    }
  }

  if ((reply = fopen(filename, "rt")))
  {
    long msglen = fsize(filename);
    char *replyText = new char[msglen + 1];

    msglen = fread(replyText, sizeof(char), msglen, reply);
    replyText[msglen] = '\0';
    fclose(reply);

    mkfname(filepath, bm->resourceObject->get(WorkDir), filename);

    if ((reply = fopen(filepath, "wt")))
    {
      success = (fwrite(replyText, sizeof(char), msglen, reply) ==
                                                             (size_t) msglen);
      fclose(reply);

      r->filename = strdupplus(filepath);
      r->length = msglen;
    }
    else success = false;

    delete[] replyText;
  }
  else success = false;

  if (success) success = (remove(filename) == 0);

  delete[] filename;
  delete[] arg;
  delete[] cmd;

  return success;
}
