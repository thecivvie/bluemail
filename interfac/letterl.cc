/*
 * blueMail offline mail reader
 * letter list

 Copyright (c) 1996 Kolossvary Tamas <thomas@tvnet.hu>
 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <ctype.h>
#include <string.h>
#include "interfac.h"
#include "../common/auxil.h"


const char *llopts[] =
{
  "This",
  "Personal",
  "Marked",
  "Listed",
  "Quit"
};


LetterListWindow::LetterListWindow ()
{
  oneSearchActive = true;
  listed = NULL;
  filter = NULL;
  initFilterFlag();
  resetSubjOffset();

  smartScroll = bm.resourceObject->isYes(SmartScrollLetterList);
}


LetterListWindow::~LetterListWindow ()
{
  delete[] filter;
  DestroyChain();
}


void LetterListWindow::initSort ()
{
  letter_sort = LL_SORT_BY_SUBJ;
  netmail_sort = letter_sort;

  const char *sb = bm.resourceObject->get(SortLettersBy);
  if (sb)
  {
    switch (toupper(*sb))
    {
      case 'S':
        letter_sort = LL_SORT_BY_SUBJ;
        break;

      case 'N':
        letter_sort = LL_SORT_BY_MSGNUM;
        break;

      case 'L':
        letter_sort = LL_SORT_BY_LASTNAME;
        break;
    }
  }

  sb = bm.resourceObject->get(SortNetmailBy);
  if (sb)
  {
    switch (toupper(*sb))
    {
      case 'S':
        netmail_sort = LL_SORT_BY_SUBJ;
        break;

      case 'N':
        netmail_sort = LL_SORT_BY_MSGNUM;
        break;

      case 'L':
        netmail_sort = LL_SORT_BY_LASTNAME;
        break;
    }
  }
}


void LetterListWindow::initFilterFlag ()
{
  filterLast = filterHeader = true;
}


void LetterListWindow::getSortFilterOptions (int &netmail_sort,
                                             int &letter_sort,
                                             const char *&filter,
                                             bool &filterLast,
                                             bool &filterHeader)
{
  netmail_sort = this->netmail_sort;
  letter_sort = this->letter_sort;
  filter = strdupplus(this->filter);
  filterLast = this->filterLast;
  filterHeader = this->filterHeader;
}


void LetterListWindow::setSortFilterOptions (int netmail_sort,
                                             int letter_sort,
                                             const char *filter,
                                             bool filterLast,
                                             bool filterHeader)
{
  this->netmail_sort = netmail_sort;
  this->letter_sort = letter_sort;
  delete[] this->filter;
  this->filter = strdupplus(filter);
  this->filterLast = filterLast;
  this->filterHeader = filterHeader;
}


void LetterListWindow::resetSubjOffset ()
{
  subjOffset = 0;
}


void LetterListWindow::MakeActive ()
{
  static char desc[] = "Letters in ";
  static char stat[] = "%%c%%c%%c";
  static char msgn[] = "%%6d ";
  static char head[] = " %%-%d.%ds";
  static char from[] = "From";
  static char to[] = "To";
  static char subj[] = "Subject";
  int fromLen, toLen, subjLen, areaLen;
  int xmore = 0, ymore = 0;
  bool mark = bm.resourceObject->isYes(DrawSortMark);
  int *ll_sort = (bm.areaList->isNetmail() ? &netmail_sort : &letter_sort);

  bm.letterList->sort(*ll_sort);

  if (bm.resourceObject->isYes(FullsizeLetterList))
  {
    xmore = 4;
    ymore = 8;
  }

  list_max_x = COLS - 6 + xmore;
  list_max_y = LINES - 11 + ymore;

  MakeChain();

  // list may indeed be empty, because the contents of the list may have
  // changed (unread letter now read, edited letter now different header)
  // while a filter is active, but we know that items exist
  if (noOfItems() == 0)
  {
    delete[] filter;
    filter = NULL;

    MakeChain();
  }

  if (noOfItems() < list_max_y) list_max_y = noOfItems();

  char *topline = new char[list_max_x + 1];

  sprintf(format, "%s%%.%ds", desc, list_max_x - (int) strlen(desc) - 4);
  sprintf(topline, format, bm.areaList->getTitle());
  interface->charconv_in(topline, bm.areaList->isLatin1());

  list = new Window((ymore ? LINES : list_max_y + 3), list_max_x + 2,
                    (xmore ? 0 : 2), (COLS - list_max_x - 2) >> 1,
                    C_LLBORDER, topline, C_LLAREA);

  delete[] topline;

  list->attrib(C_LLTOPTEXT);
  list->putstring(0, 3, desc);

  // 19 columns = 1 (main border) + 1 (space) + 1 (border)
  //              (these 3 columns on both sides) +
  //              3 (stat) + 7 (msgn & space) + 3 (spaces before heads)
  int total = COLS - 19 + xmore;

  subjLen = (bm.areaList->hasTo() ? total >> 1 : total * 65 / 100);
  total -= subjLen;
  toLen = (bm.areaList->hasTo() ? total >> 1 : 0);
  fromLen = total - toLen;

  char *p = format;
  p += sprintf(p, stat);

  showArea = (bm.areaList->isReplyArea() || bm.areaList->isCollection());

  if (showArea)
  {
    areaLen = fromLen + 6;   // 6 = msgn without the space: give...
    toLen++;                 // ...an extra character to this field
    p += sprintf(p, head, areaLen, areaLen);
  }
  else
  {
    p += sprintf(p, msgn);
    p += sprintf(p, head, fromLen, fromLen);
  }

  p += sprintf(p, head, toLen, toLen);
  p += sprintf(p, head, subjLen, subjLen);

  if (showArea) sprintf(list->lineBuf, format, ' ', ' ', ' ', "Area",
                        (bm.areaList->isReplyArea() ? to : from), subj);
  else sprintf(list->lineBuf, format, ' ', ' ', ' ', 0, from, to, subj);

  list->attrib(C_LLHEADER);
  list->putstring(1, 1, list->lineBuf);
  if (!showArea)
  {
    list->putstring(1, 6, "Msg#");

    if (mark)
    {
      int pos = 0;

      switch (*ll_sort)
      {
        case LL_SORT_BY_SUBJ:
          pos = strstr(list->lineBuf, subj) - list->lineBuf + strlen(subj) + 1;
          break;

        case LL_SORT_BY_MSGNUM:
          pos = 3;
          break;

        case LL_SORT_BY_LASTNAME:
          pos = 16;
          break;
      }

      list->putstring(1, pos, "( )");
      list->putch(1, pos + 1, ACS_DARROW);
    }
  }

  if (filter)
  {
    chtype fmark[2] = {' ' | C_LLBORDER, FILTER_SIGN | C_LLBORDER};
    list->putchnstr(0, list_max_x, fmark, 2);
  }

  list->delay_update();

  if (bm.areaList->isReplyArea()) interface->isoToggleSet(false);

  relist();
  Draw();
  Select();
}


void LetterListWindow::Delete ()
{
  delete list;
}


void LetterListWindow::Quit ()
{
  delete[] filter;
  filter = NULL;
}


void LetterListWindow::MakeChain ()
{
  int noOfLetters = bm.letterList->getNoOfLetters();

  DestroyChain();

  // filter
  oneSearchActive = false;
  listed = new bool[noOfLetters];
  for (int i = 0; i < noOfLetters; i++)
  {
    if (!filter || (oneSearch(i, filter) == FND_YES)) listed[i] = true;
    else listed[i] = false;
  }

  oneSearchActive = true;

  bm.letterList->setFilter(listed);
  bm.letterList->relist(false);
}


void LetterListWindow::DestroyChain ()
{
  delete[] listed;
  listed = NULL;
}


int LetterListWindow::noOfItems ()
{
  return bm.letterList->getNoOfActive();
}


void LetterListWindow::oneLine (int i)
{
  bm.letterList->gotoActive(topline + i);

  int stat = bm.letterList->getStatus();
  char marked = (stat & MSF_MARKED ? 'M' : ' ');
  char replied = (stat & MSF_REPLIED ? '~' : ' ');
  char read = (stat & MSF_READ ? '*' : ' ');

  const char *from = From();
  const char *to = bm.letterList->getTo();
  const char *subj = bm.letterList->getSubject();
  const char *in = bm.letterList->getReplyIn();

  subj = (subjOffset <= (int) strlen(subj) ? subj + subjOffset : "");

  if (showArea)
    sprintf(list->lineBuf, format, marked, replied, read,
            in && *in ? in
                      : bm.areaList->getTitle(bm.letterList->getAreaID()),
            (bm.areaList->isReplyArea() ? to : from), subj);
  else
    sprintf(list->lineBuf, format, marked, replied, read,
            bm.letterList->getMsgNum(), from, to, subj);

  const char *l = bm.resourceObject->get(LoginName);
  const char *a = bm.resourceObject->get(AliasName);

  chtype c_ll = C_LLISTUNRD;

  if ((*l && (strcasecmp(l, from) == 0)) || (*a && (strcasecmp(a, from) == 0)))
    c_ll = C_LLFROMUSER;
  if (bm.letterList->isPersonal()) c_ll = C_LLTOUSER;
  if (stat & MSF_READ) c_ll = C_LLISTREAD;

  interface->charconv_in(list->lineBuf, bm.letterList->isLatin1());
  list->PUTLINE(i, c_ll);
}


searchtype LetterListWindow::oneSearch (int l, const char *what)
{
  const char *col1, *col2;
  bool found1, found2, found3, found;

  if (oneSearchActive) bm.letterList->gotoActive(l);
  else bm.letterList->gotoLetter(l);

  if (oneSearchActive || filterHeader)
  {
    const char *from = From();
    const char *to = bm.letterList->getTo();
    const char *subj = bm.letterList->getSubject();
    const char *in = bm.letterList->getReplyIn();

    if (showArea)
    {
      col1 = (in && *in ? in
                        : bm.areaList->getTitle(bm.letterList->getAreaID()));
      col2 = (bm.areaList->isReplyArea() ? to : from);
    }
    else
    {
      col1 = from;
      col2 = to;
    }

    found1 = (strexcmp(interface->charconv_in(bm.letterList->isLatin1(), col1),
                       what) != NULL);

    found2 = (strexcmp(interface->charconv_in(bm.letterList->isLatin1(),
                                              col2),
                       what) != NULL);
    found3 = (strexcmp(interface->charconv_in(bm.letterList->isLatin1(),
                                              subj),
                       what) != NULL);

    int isNot;
    (void) strex(isNot, what);
    found = (isNot ? found1 && found2 && found3 : found1 || found2 || found3);
  }
  else
  {
    char *message = (char *) bm.letterList->getBody();
    found = (strexcmp(interface->charconv_in(message, bm.letterList->isLatin1()),
                      what) != NULL);
    // because we modified message body, letterwin must not (re)use it
    interface->letterwin.Reset();
  }

  return (found ? FND_YES : FND_NO);
}


const char *LetterListWindow::From ()
{
  const char *from = bm.letterList->getFrom();

  if (!*from)
  {
    bool isInternet = bm.areaList->isInternet(bm.letterList->getAreaID());
    from = bm.letterList->getNetAddr().get(isInternet);
    if (!from) from = "";
  }

  return from;
}


void LetterListWindow::ResetActive ()
{
  active = bm.letterList->getActive();
}


void LetterListWindow::Select ()
{
  bm.letterList->gotoActive(active);
}


void LetterListWindow::FirstUnread ()
{
  topline = 0;
  active = -1;
  relist();
  NextUnread();

  // all letters read?
  if ((active == noOfItems() - 1) && bm.letterList->isRead())
  {
    Move(HOME);
    Draw();
  }
}


void LetterListWindow::NextUnread ()
{
  do
  {
    Move(DOWN);
    bm.letterList->gotoActive(active);
  }
  while (bm.letterList->isRead() && (active + 1 < noOfItems()));

  Draw();
}


void LetterListWindow::PrevUnread ()
{
  do
  {
    Move(UP);
    bm.letterList->gotoActive(active);
  }
  while (bm.letterList->isRead() && (active > 0));

  Draw();
}


void LetterListWindow::askFilter ()
{
  char input[QUERYINPUT + 1];

  strncpy(input, (filter ? filter : ""), QUERYINPUT);
  input[QUERYINPUT] = '\0';

  if (interface->QueryBox("Filter on:", input, QUERYINPUT) != GOT_ESC)
  {
    savePos();

    delete[] filter;
    filter = strdupplus(input);
    MakeChain();

    // empty list isn't allowed
    if (noOfItems() == 0)
    {
      if (*filter) interface->ErrorWindow("No match");
      delete[] filter;
      filter = NULL;
      MakeChain();
      restorePos();
      initFilterFlag();
    }
  }
  else filterHeader = filterLast;
}


void LetterListWindow::extrakeys (int key)
{
  int *ll_sort = (bm.areaList->isNetmail() ? &netmail_sort : &letter_sort), r;

  Select();

  switch (key)
  {
    case 'A':
      interface->changestate(addressbook);
      break;

    case 'C':
      interface->charsetToggle();
      break;

    case 'E':
      if (bm.areaList->isReplyArea()) interface->letterwin.KeyHandle('E');
      else
      {
        interface->letterwin.setLetterParam(bm.areaList->getAreaNo(), IS_NEW);
        if (bm.areaList->isNetmail()) interface->changestate(addressbook);
        interface->letterwin.EnterLetter();
      }
      interface->update();
      break;

    case 'D':
    case 'K':
    case KEY_DC:
      if (bm.areaList->isReplyArea())
      {
        interface->letterwin.KeyHandle(key);
        if (key == 'D') interface->update();
      }
      break;

    case 'L':
      bm.letterList->relist(true);
      ResetActive();
      interface->update();
      break;

    case 'O':
      if (*ll_sort == LL_SORT_LASTTYPE) *ll_sort = LL_SORT_FIRSTTYPE;
      else (*ll_sort)++;
      interface->letterwin.Reset();
      interface->update();
      break;

    case 'P':
    case 'S':
      if ((key == 'S') || interface->letterwin.okPrint())
        if ((r = interface->WarningWindow(key == 'P' ? "Print which?"
                                                     : "Save which?",
                                          llopts, 5)) > WW_NO)
          if (interface->letterwin.Save(r, (key == 'P')) && (r == SAVE_THIS))
            Move(DOWN);
      interface->update();
      break;

    case 'U':
    case 'M':
      r = (key == 'U' ? MSF_READ : MSF_MARKED);
      if (bm.areaList->supportedMSF() & r)
      {
        bm.letterList->setStatus(bm.letterList->getStatus() ^ r);
        interface->setAnyRead();
        Move(DOWN);
        Draw();
      }
      break;

    case 'B':
    case '|':
      filterLast = filterHeader;
      filterHeader = (key == '|');
      askFilter();
      interface->update();
      break;

    case '!':
      if (interface->areas.makeReply()) interface->update();
      break;

    case '+':
      if (subjOffset < bm.areaList->getMaxSubjLen())
      {
        subjOffset++;
        relist();
        Draw();
      }
      break;

    case '-':
      if (subjOffset > 0)
      {
        subjOffset--;
        relist();
        Draw();
      }
      break;

    case KEY_LEFT:
      PrevUnread();
      break;

    case KEY_RIGHT:
      NextUnread();
      break;
  }
}
