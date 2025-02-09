/*
 * blueMail offline mail reader
 * letter_header and letter_list

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "bmail.h"
#include "../common/auxil.h"


/*
   letter header
*/

letter_header::letter_header (bmail *bm, int LetterID, const char *from,
                              const char *to, const char *subject,
                              const char *date, int msgNum, int replyTo,
                              int AreaID, int flags, bool latin1,
                              net_address *netAddr, const char *replyAddr,
                              const char *replyIn, const char *replyID,
                              main_driver *driver)
{
  dl = bm->driverList;
  ro = bm->resourceObject;

  this->latin1 = latin1;       // needed by setFrom, setTo and setSubject
  this->driver = driver;       // needed by setSubject

  this->LetterID = LetterID;

  this->from = NULL;
  setFrom(from);

  this->to = NULL;
  setTo(to);

  this->subject = NULL;
  setSubject(subject);

  this->date = strdupplus(date);
  this->msgNum = msgNum;
  this->replyTo = replyTo;
  this->AreaID = AreaID;
  this->flags = flags;

  if (netAddr) this->netAddr = *netAddr;
  // else information will be set later

  this->replyAddr = strdupplus(replyAddr);
  this->replyIn = strdupplus(replyIn);
  this->replyID = strdupplus(replyID);

  read = dl->getReadObject(driver);
}


letter_header::~letter_header ()
{
  delete[] replyID;
  delete[] replyIn;
  delete[] replyAddr;
  delete[] date;
  delete[] subject;
  delete[] to;
  delete[] from;
}


inline int letter_header::getLetterID () const
{
  return LetterID;
}


const char *letter_header::getFrom () const
{
  return from;
}


void letter_header::setFrom (const char *from)
{
  delete[] this->from;
  this->from = stralloc(from);
  mimeDecodeHeader(from, this->from, &latin1);
  cropesp(this->from);
}


const char *letter_header::getTo () const
{
  return to;
}


void letter_header::setTo (const char *to)
{
  delete[] this->to;
  this->to = stralloc(to);
  mimeDecodeHeader(to, this->to, &latin1);
  cropesp(this->to);
}


const char *letter_header::getSubject () const
{
  return subject;
}


void letter_header::setSubject (const char *subject)
{
  delete[] this->subject;
  this->subject = stralloc(subject);
  mimeDecodeHeader(subject, this->subject, &latin1);
  cropesp(this->subject);

  if (ro->isYes(StripRe) && (driver != dl->getReplyDriver()))
  {
    const char *s = stripRE(this->subject);
    if (s != this->subject) strcpy(this->subject, s);
  }
}


const char *letter_header::getDate () const
{
  return date;
}


inline int letter_header::getMsgNum () const
{
  return msgNum;
}


int letter_header::getReplyTo () const
{
  return replyTo;
}


int letter_header::getAreaID () const
{
  // don't tell the real (driver's) area number, but bmail's one
  return AreaID + dl->getOffset(driver);
}


int letter_header::getFlags () const
{
  return flags;
}


bool letter_header::isPrivate () const
{
  return (flags & PRIVATE) != 0;
}


char *letter_header::Flags () const
{
  static const char *flagname[] = {NULL, CRASH_TXT, RECEIVED_TXT, SENT_TXT,
                                   DIRECT_TXT, FILEATTACHED_TXT, INTRANSIT_TXT,
                                   ORPHAN_TXT, KILL_TXT, LOCAL_TXT, HOLD_TXT,
                                   IMMEDIATE_TXT, FILEREQUEST_TXT, NULL, NULL,
                                   FILEUPDATEREQ_TXT};
  static char result[77];
  *result='\0';

  // put "direct" flag to position 5 (after "sent"),
  // clear "private" and unused flags
  int f = (flags & 0x8000) | ((flags & 0x0FF0) << 1) |
          ((flags & DIRECT) >> 8) | (flags & 0x000E);

  for (int bit = 0; bit < 16; bit++)
  {
    if (f & 1)
    {
      if (!*result) strcpy(result, flagname[bit]);
      else sprintf(result , "%s %s", result, flagname[bit]);
    }
    f >>= 1;
  }

  return result;
}


bool letter_header::isLatin1 () const
{
  return latin1;
}


void letter_header::setLatin1 (bool latin1)
{
  this->latin1 = latin1;
}


net_address &letter_header::getNetAddr ()
{
  return netAddr;
}


inline const char *letter_header::getReplyAddr () const
{
  return replyAddr;
}


inline const char *letter_header::getReplyIn () const
{
  return replyIn;
}


const char *letter_header::getReplyID () const
{
  return replyID;
}


inline main_driver *letter_header::getDriver () const
{
  return driver;
}


bool letter_header::isPersonal () const
{
  if (dl->hasPersonal())   // driver did already flag personal messages
  {
    return (getStatus() & MSF_PERSONAL) != 0;
  }
  else
  {
    const char *l, *a;

    l = ro->get(LoginName);
    a = ro->get(AliasName);

    return ((*l && (strcasecmp(l, to) == 0)) ||
            (*a && (strcasecmp(a, to) == 0)));
  }
}


inline bool letter_header::isRead () const
{
  return read->isRead(AreaID, LetterID);
}


inline void letter_header::setRead ()
{
  read->setRead(AreaID, LetterID, true);
}


void letter_header::setMarked ()
{
  read->setStatus(AreaID, LetterID,
                  read->getStatus(AreaID, LetterID) | MSF_MARKED);
}


int letter_header::getStatus () const
{
  return read->getStatus(AreaID, LetterID);
}


inline void letter_header::setStatus (int stat)
{
  read->setStatus(AreaID, LetterID, stat);
}


inline const char *letter_header::getBody ()
{
  return driver->getBody(AreaID, LetterID, this);
}


/*
   letter list
*/

letter_list::letter_list (bmail *bm, int areaNumber, int areaType)
{
  driver_list *dl = bm->driverList;

  ro = bm->resourceObject;
  driver = dl->getDriver(areaNumber);
  area = areaNumber - dl->getOffset(driver);
  read = dl->getReadObject(driver);
  this->areaType = areaType;

  filter = NULL;
  init();
}


letter_list::~letter_list ()
{
  cleanup();
}


void letter_list::init ()
{
  driver->selectArea(area);
  noOfLetters = driver->getNoOfLetters();
  currentLetter = 0;

  activeHeader = new int[noOfLetters];
  letterHeader = new letter_header *[noOfLetters];

  driver->resetLetters();
  for (int l = 0; l < noOfLetters; l++)
    letterHeader[l] = driver->getNextLetter();

  shortlist = !ro->isYes(LongLetterList);
  relist(false);
}


void letter_list::cleanup ()
{
  while (noOfLetters) delete letterHeader[--noOfLetters];

  delete[] letterHeader;
  delete[] activeHeader;
}


void letter_list::sort (int sorttype)
{
  // must be the same order like the LL_SORT_BY constants!
  int (*func[])(const void *, const void *) = {lsortbysubj, lsortbymsgnum,
                                               lsortbylastname};

  if (noOfLetters && !(areaType & A_REPLYAREA) && !(areaType & A_COLLECTION))
  {
    qsort(letterHeader, noOfLetters, sizeof(letter_header *), func[sorttype]);
    relist(false);
  }
}


void letter_list::relist (bool change)
{
  noOfActive = 0;

  if (!change) shortlist = !shortlist;   // will be switched in the loop

  while (noOfLetters && (noOfActive == 0))
  {
    shortlist = !shortlist;

    for (int l = 0; l < noOfLetters; l++)
      if (!letterHeader[l]->isRead() || !shortlist)
        activeHeader[noOfActive++] = l;
  }

  // filter
  int listedLetters = 0;
  for (int l = 0; l < noOfActive; l++)
    if (!filter || filter[activeHeader[l]])
      activeHeader[listedLetters++] = activeHeader[l];

  noOfActive = listedLetters;
}


const char *letter_list::getFrom () const
{
  return letterHeader[currentLetter]->getFrom();
}


const char *letter_list::getTo () const
{
  return letterHeader[currentLetter]->getTo();
}


const char *letter_list::getSubject () const
{
  return letterHeader[currentLetter]->getSubject();
}


const char *letter_list::getDate () const
{
  return letterHeader[currentLetter]->getDate();
}


int letter_list::getMsgNum () const
{
  return letterHeader[currentLetter]->getMsgNum();
}


int letter_list::getReplyTo () const
{
  return letterHeader[currentLetter]->getReplyTo();
}


int letter_list::getAreaID () const
{
  return letterHeader[currentLetter]->getAreaID();
}


int letter_list::getFlags () const
{
  return letterHeader[currentLetter]->getFlags();
}


bool letter_list::isPrivate () const
{
  return letterHeader[currentLetter]->isPrivate();
}


char *letter_list::Flags () const
{
  return letterHeader[currentLetter]->Flags();
}


bool letter_list::isLatin1 () const
{
  return letterHeader[currentLetter]->isLatin1();
}


net_address &letter_list::getNetAddr ()
{
  return letterHeader[currentLetter]->getNetAddr();
}


const char *letter_list::getReplyAddr () const
{
  return letterHeader[currentLetter]->getReplyAddr();
}


const char *letter_list::getReplyIn () const
{
  return letterHeader[currentLetter]->getReplyIn();
}


const char *letter_list::getReplyID () const
{
  return letterHeader[currentLetter]->getReplyID();
}


bool letter_list::isPersonal () const
{
  return letterHeader[currentLetter]->isPersonal();
}


bool letter_list::isRead () const
{
  bool stat = letterHeader[currentLetter]->isRead();
  // harmonize between the real message and the collected message
  if (areaType & A_COLLECTION) read->setRead(area, currentLetter, stat);

  return stat;
}


void letter_list::setRead ()
{
  letterHeader[currentLetter]->setRead();
  // handle collected message, too
  if (areaType & A_COLLECTION) read->setRead(area, currentLetter, true);
}


int letter_list::getStatus () const
{
  int stat = letterHeader[currentLetter]->getStatus();
  // harmonize between the real message and the collected message
  if (areaType & A_COLLECTION) read->setStatus(area, currentLetter, stat);

  return stat;
}


void letter_list::setStatus (int stat)
{
  letterHeader[currentLetter]->setStatus(stat);
  // handle collected message, too
  if (areaType & A_COLLECTION) read->setStatus(area, currentLetter, stat);
}


const char *letter_list::getBody () const
{
  return letterHeader[currentLetter]->getBody();
}


int letter_list::getNoOfLetters () const
{
  return noOfLetters;
}


int letter_list::getNoOfActive () const
{
  return noOfActive;
}


void letter_list::gotoLetter (int letter)
{
  if (letter >= 0 && letter < noOfLetters) currentLetter = letter;
}


void letter_list::gotoActive (int letter)
{
  if (letter >= 0 && letter < noOfActive) currentLetter = activeHeader[letter];
}


int letter_list::getCurrent () const
{
  return currentLetter;
}


int letter_list::getActive () const
{
  int l;

  for (l = 0; l < noOfActive; l++)
    if (activeHeader[l] >= currentLetter) break;

  return l;
}


void letter_list::refreshLetters ()
{
  cleanup();
  init();
}


void letter_list::setFilter (bool *filter)
{
  this->filter = filter;
}


/*
   letter qsort compare functions
*/

int lsortbysubj (const void *a, const void *b)
{
  const char *la = stripRE((*(letter_header **) a)->getSubject());
  const char *lb = stripRE((*(letter_header **) b)->getSubject());
  int result = strcasecoll(la, lb);
  return (result ? result : lsortbymsgnum(a, b));
}


int lsortbymsgnum (const void *a, const void *b)
{
  int la = (*(letter_header **) a)->getLetterID();
  int lb = (*(letter_header **) b)->getLetterID();
  if (la < lb) return -1;
  if (la > lb) return 1;
  return 0;
}


int lsortbylastname (const void *a, const void *b)
{
  const char *la = strrword((*(letter_header **) a)->getFrom());
  const char *lb = strrword((*(letter_header **) b)->getFrom());
  int result = strcasecoll(la, lb);
  return (result ? result : lsortbymsgnum(a, b));
}
