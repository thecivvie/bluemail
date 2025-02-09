/*
 * blueMail offline mail reader
 * area_header and area_list

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "bmail.h"
#include "../common/auxil.h"
#include "../common/error.h"


/*
   area header
*/

area_header::area_header (bmail *bm, int areaNumber, const char *number,
                          const char *shortname, const char *title,
                          const char *type, int flags, int noOfLetters,
                          int noOfPersonal, int maxFromToLen, int maxSubjLen)
{
  this->bm = bm;
  this->number = strdupplus(number);
  this->shortname = strdupplus(shortname);
  this->title = strdupplus(title);
  this->type = strdupplus(type);
  this->flags = flags;
  this->noOfLetters = noOfLetters;
  this->noOfPersonal = noOfPersonal;
  this->maxFromToLen = maxFromToLen;
  this->maxSubjLen = maxSubjLen;

  driver = bm->driverList->getDriver(areaNumber);
  area = areaNumber - bm->driverList->getOffset(driver);
}


area_header::~area_header ()
{
  delete[] type;
  delete[] title;
  delete[] shortname;
  delete[] number;
}


inline const char *area_header::getNumber () const
{
  return number;
}


inline const char *area_header::getShortName () const
{
  return shortname;
}


inline const char *area_header::getTitle () const
{
  return title;
}


inline const char *area_header::getAreaType () const
{
  return type;
}


inline bool area_header::isCollection () const
{
  return (flags & A_COLLECTION) != 0;
}


inline bool area_header::isReplyArea () const
{
  return (flags & A_REPLYAREA) != 0;
}


inline bool area_header::isSubscribed () const
{
  return (flags & A_SUBSCRIBED) != 0;
}


inline bool area_header::isForced () const
{
  return (flags & A_FORCED) != 0;
}


inline bool area_header::isToList () const
{
  return (flags & A_LIST) != 0;
}


inline bool area_header::useAlias () const
{
  return (flags & A_ALIAS) != 0;
}


inline bool area_header::isNetmail () const
{
  return (flags & A_NETMAIL) != 0;
}


inline bool area_header::isInternet () const
{
  return (flags & A_INTERNET) != 0;
}


inline bool area_header::isReadonly () const
{
  return (flags & A_READONLY) != 0;
}


inline bool area_header::hasTo () const
{
  return ((flags & A_NETMAIL) != 0 || (flags & A_INTERNET) == 0);
}


inline bool area_header::allowPrivate () const
{
  return (flags & A_ALLOWPRIV) != 0;
}


inline bool area_header::isLatin1 () const
{
  return (flags & A_CHRSLATIN1) != 0;
}


inline bool area_header::isMarked () const
{
  return (flags & A_MARKED) != 0;
}


inline void area_header::setMarked (bool on)
{
  if (on) flags |= A_MARKED;
  else flags &= ~A_MARKED;
}


void area_header::setAdded (bool clear)
{
  if (clear) flags &= ~(A_ADDED | A_DROPPED);

  if (flags & A_DROPPED) flags &= ~A_DROPPED;
  else flags |= A_ADDED;
}


inline bool area_header::isAdded () const
{
  return (flags & A_ADDED) != 0;
}


void area_header::setDropped (bool clear)
{
  if (clear) flags &= ~(A_ADDED | A_DROPPED);

  if (flags & A_ADDED) flags &= ~A_ADDED;
  else flags |= A_DROPPED;
}


inline bool area_header::isDropped () const
{
  return (flags & A_DROPPED) != 0;
}


inline int area_header::getNoOfLetters () const
{
  return noOfLetters;
}


int area_header::getNoOfUnread ()
{
  return (bm->driverList->getReadObject(driver))->getNoOfUnread(area);
}


int area_header::getNoOfPersonal () const
{
  return noOfPersonal;
}


inline int area_header::getMaxFromToLen () const
{
  return maxFromToLen;
}


inline int area_header::getMaxSubjLen () const
{
  return maxSubjLen;
}


inline int area_header::supportedMSF () const
{
  return (bm->driverList->getReadObject(driver))->supportedMSF();
}


/*
   area list
*/

area_list::area_list (bmail *bm)
{
  this->bm = bm;
  noOfAreas = 0;
  currentArea = 0;

  if (bm->driverList->canReply())
    noOfAreas += bm->driverList->getReplyDriver()->getNoOfAreas();

  noOfAreas += bm->driverList->getMainDriver()->getNoOfAreas();

  activeHeader = new int[noOfAreas];
  areaHeader = new area_header *[noOfAreas];

  for (int a = 0; a < noOfAreas; a++)
    areaHeader[a] = bm->driverList->getDriver(a)->getNextArea();

  if (bm->getServiceType() != ST_DEMO) readAreaMarks();

  shortlist = !bm->resourceObject->isYes(LongAreaList);
  filter = NULL;
  relist(false);
}


area_list::~area_list ()
{
  while (noOfAreas) delete areaHeader[--noOfAreas];

  delete[] areaHeader;
  delete[] activeHeader;
}


void area_list::relist (bool change)
{
  bool showAll, anyMarked, isReplyArea;

  noOfActive = 0;

  if (!change) shortlist = !shortlist;   // will be switched in the loop

  showAll = false;
  anyMarked = isAnyMarked();

  while (noOfAreas && (noOfActive == 0))
  {
    shortlist = !shortlist;

    for (int a = 0; a < noOfAreas; a++)
    {
      isReplyArea = ((a == 0) && bm->driverList->canReply());

      if ((anyMarked && shortlist ? isReplyArea || areaHeader[a]->isMarked()
                                  : areaHeader[a]->isSubscribed() ||
                                    areaHeader[a]->isToList() ||
                                    areaHeader[a]->getNoOfLetters())
          || showAll)
        if (!shortlist || areaHeader[a]->getNoOfLetters() || isReplyArea)
          activeHeader[noOfActive++] = a;
    }

    showAll = true;
  }

  // filter
  int listedAreas = 0;
  for (int a = 0; a < noOfActive; a++)
    if (!filter || filter[activeHeader[a]] ||
        (bm->driverList->canReply() && (a == 0)))
      activeHeader[listedAreas++] = activeHeader[a];

  noOfActive = listedAreas;
}


int area_list::checkArea (int area) const
{
  if (area == -1) return currentArea;

  if (area < 0 || area >= noOfAreas) fatalError("Invalid area.");

  return area;
}


const char *area_list::getNumber () const
{
  return areaHeader[currentArea]->getNumber();
}


const char *area_list::getShortName (int area) const
{
  return areaHeader[checkArea(area)]->getShortName();
}


const char *area_list::getTitle (int area) const
{
  return areaHeader[checkArea(area)]->getTitle();
}


const char *area_list::getAreaType () const
{
  return areaHeader[currentArea]->getAreaType();
}


bool area_list::isCollection () const
{
  return areaHeader[currentArea]->isCollection();
}


bool area_list::isReplyArea () const
{
  return areaHeader[currentArea]->isReplyArea();
}


bool area_list::isSubscribed () const
{
  return areaHeader[currentArea]->isSubscribed();
}


bool area_list::isForced () const
{
  return areaHeader[currentArea]->isForced();
}


bool area_list::useAlias () const
{
  return areaHeader[currentArea]->useAlias();
}


bool area_list::isNetmail (int area) const
{
  return areaHeader[checkArea(area)]->isNetmail();
}


bool area_list::isInternet (int area) const
{
  return areaHeader[checkArea(area)]->isInternet();
}


bool area_list::isReadonly () const
{
  return areaHeader[currentArea]->isReadonly();
}


bool area_list::hasTo () const
{
  return areaHeader[currentArea]->hasTo();
}


bool area_list::allowPrivate () const
{
  return areaHeader[currentArea]->allowPrivate();
}


bool area_list::isLatin1 () const
{
  return areaHeader[currentArea]->isLatin1();
}


bool area_list::isLatin1 (int area) const
{
  return (area >= 0 && area < noOfAreas ? areaHeader[area]->isLatin1()
                                        : false);
}


bool area_list::isMarked () const
{
  return areaHeader[currentArea]->isMarked();
}


bool area_list::isAnyMarked () const
{
  for (int a = 0; a < noOfAreas; a++)
    if (areaHeader[a]->isMarked()) return true;

  return false;
}


void area_list::setMarked (bool on)
{
  areaHeader[currentArea]->setMarked(on);
}


void area_list::setAdded (bool clear)
{
  areaHeader[currentArea]->setAdded(clear);
}


bool area_list::isAdded () const
{
  return areaHeader[currentArea]->isAdded();
}


void area_list::setDropped (bool clear)
{
  areaHeader[currentArea]->setDropped(clear);
}


bool area_list::isDropped () const
{
  return areaHeader[currentArea]->isDropped();
}


int area_list::getNoOfLetters () const
{
  return areaHeader[currentArea]->getNoOfLetters();
}


int area_list::getNoOfUnread () const
{
  return areaHeader[currentArea]->getNoOfUnread();
}


int area_list::getNoOfPersonal () const
{
  return areaHeader[currentArea]->getNoOfPersonal();
}


int area_list::getMaxFromToLen () const
{
  return areaHeader[currentArea]->getMaxFromToLen();
}


int area_list::getMaxSubjLen () const
{
  return areaHeader[currentArea]->getMaxSubjLen();
}


int area_list::supportedMSF () const
{
  return areaHeader[currentArea]->supportedMSF();
}


int area_list::getNoOfAreas () const
{
  return noOfAreas;
}


int area_list::getNoOfActive () const
{
  return noOfActive;
}


int area_list::getAreaNo () const
{
  return currentArea;
}


int area_list::getActive ()
{
  int a;

  for (a = 0; a < noOfActive; a++)
    if (activeHeader[a] >= currentArea) break;

  return a;
}


void area_list::gotoArea (int area)
{
  if (area >= 0 && area < noOfAreas) currentArea = area;
}


void area_list::gotoActive (int area)
{
  if (area >= 0 && area < noOfActive) currentArea = activeHeader[area];
}


bool area_list::findNetmail (int *area) const
{
  bool hasNetmail = false;
  *area = -1;

  for (int a = 0; a < noOfAreas; a++)
    if (areaHeader[a]->isNetmail())
    {
      if (hasNetmail)   // multiple netmail areas
      {
        *area = -1;
        break;
      }

      hasNetmail = true;
      *area = a;
    }

  return hasNetmail;
}


void area_list::getLetterList ()
{
  bm->letterList = new letter_list(bm, currentArea,
                                   (isReplyArea() ? A_REPLYAREA : 0) |
                                   (isCollection() ? A_COLLECTION : 0));
}


void area_list::enterLetter (int areaNo, const char *from, const char *to,
                             const char *subject, int replyTo, int flags,
                             net_address *netAddress, const char *replyIn,
                             const char *replyID, const char *filename,
                             long length)
{
  reply_driver *replyDriver = bm->driverList->getReplyDriver();

  int current_area = currentArea;
  gotoArea(areaNo);

  // pass some essential information to the reply driver
  letter_header *newLetter = new letter_header(bm, 0, from, to, subject, "",
                                               0, replyTo, areaNo, flags,
                                               isLatin1(), netAddress, NULL,
                                               replyIn, replyID, replyDriver);
  replyDriver->enterLetter(newLetter, filename, length);
  delete newLetter;

  gotoArea(current_area);   // restore
  refreshReplyArea();
}


void area_list::killLetter (int letterNo)
{
  bm->driverList->getReplyDriver()->killLetter(letterNo);
  refreshReplyArea();
}


const char *area_list::newLetterBody (int letterNo, long size)
{
  return bm->driverList->getReplyDriver()->newLetterBody(letterNo, size);
}


bool area_list::repliesOK ()
{
  bool ok = true;

  // this function will only be called if replies are possible
  // if so, there *is* a REPLY_AREA and it has area number 0
  letter_list *ll = new letter_list(bm, 0, A_COLLECTION | A_READONLY);

  for (int l = 0; l < ll->getNoOfLetters(); l++)
  {
    ll->gotoLetter(l);
    if (ll->getAreaID() == 0)
    {
      ok = false;
      break;
    }
  }

  delete ll;

  return ok;
}


bool area_list::makeReply ()
{
  return bm->driverList->getReplyDriver()->makeReply();
}


void area_list::refreshReplyArea ()
{
  // this function will only be called if replies are possible
  // if so, there *is* a REPLY_AREA and it has area number 0
  delete areaHeader[0];
  areaHeader[0] = bm->driverList->getReplyDriver()->getNextArea();

  // inside a letter list and it's the reply letter list?
  if (bm->letterList && (currentArea == 0)) bm->letterList->refreshLetters();
}


void area_list::updateCollectionStatus ()
{
  for (int a = 0; a < noOfAreas; a++)
    if (areaHeader[a]->isCollection())
    {
      letter_list *ll = new letter_list(bm, a, A_COLLECTION);
      for (int l = 0; l < ll->getNoOfLetters(); l++)
      {
        ll->gotoLetter(l);
        // getting the status forces the reader to harmonize the message
        // status flags between the real message and the collected message
        ll->getStatus();
      }
      delete ll;
    }
}


void area_list::setFilter (bool *filter)
{
  this->filter = filter;
}


bool area_list::areaConfig () const
{
  for (int a = 0; a < noOfAreas; a++)
    if (areaHeader[a]->isAdded() || areaHeader[a]->isDropped()) return true;

  return false;
}


void area_list::readAreaMarks ()
{
  char fname[MYMAXPATH], buffer[MYMAXLINE];
  FILE *f;

  mkfname(fname, bm->resourceObject->get(InfDir), bm->getAreaMarksFile());

  if ((f = fopen(fname, "rt")))
  {
    while (fgetsnl(buffer, sizeof(buffer), f))
    {
      mkstr(buffer);

      for (int a = 0; a < noOfAreas; a++)
        if (strcmp(buffer, getTitle(a)) == 0) areaHeader[a]->setMarked(true);
    }

    fclose(f);
  }
}
