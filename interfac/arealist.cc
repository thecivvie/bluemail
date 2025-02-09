/*
 * blueMail offline mail reader
 * little area list and area list

 Copyright (c) 1996 Kolossvary Tamas <thomas@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "interfac.h"
#include "../common/auxil.h"


/*
   little area list window
*/

LittleAreaListWindow::LittleAreaListWindow ()
{
  noOfListed = 0;
  listed = NULL;
  filter = NULL;
  type = AREAS_ALL;
}


LittleAreaListWindow::~LittleAreaListWindow ()
{
  delete[] filter;
  DestroyChain();
}


void LittleAreaListWindow::MakeActive ()
{
  list_max_y = LINES - 8;
  list_max_x = COLS - 44;

  // max. length of an area title
  sprintf(format, " %%-%d.%ds ", list_max_x - 2, list_max_x - 2);

  list = new Window(list_max_y + 3, list_max_x + 2,
                    (LINES - list_max_y - 3) >> 1,
                    (COLS - list_max_x - 2) >> 1,
                    C_LALBORDER, "Select Area", C_LALTOPTEXT);

  if (filter)
  {
    chtype fmark[2] = {' ' | C_LALBORDER, FILTER_SIGN | C_LALBORDER};
    list->putchnstr(0, list_max_x, fmark, 2);
  }

  list->delay_update();

  // MakeChain() is public should have been called prior to MakeActive()
  if (!listed) MakeChain(type);

  relist();
  Draw();
}


void LittleAreaListWindow::Delete ()
{
  delete list;
}


void LittleAreaListWindow::Quit ()
{
  delete[] filter;
  filter = NULL;
}


void LittleAreaListWindow::MakeChain (int type)
{
  bool isListed = false;

  this->type = type;

  DestroyChain();

  int current_area = bm.areaList->getAreaNo();
  int letter_area = bm.letterList->getAreaID();
  int noOfAreas = bm.areaList->getNoOfAreas();

  // filter
  listed = new int[noOfAreas];
  for (int i = 0; i < noOfAreas; i++)
  {
    bm.areaList->gotoArea(i);

    // initialize listed, so that oneSearch() can be used to check filter
    listed[i] = i;

    if (!bm.areaList->isReplyArea() && !bm.areaList->isCollection())
    {
      if ((type == AREAS_ALL || bm.areaList->isNetmail()) &&
          (!filter || (oneSearch(i, filter) == FND_YES)))
      {
        listed[noOfListed++] = i;
        if (i == letter_area) isListed = true;
      }
    }
  }

  bm.areaList->gotoArea(current_area);   // restore

  Move(HOME);

  if (isListed)
    while (listed[active] != letter_area) Move(DOWN);
}


void LittleAreaListWindow::DestroyChain ()
{
  noOfListed = 0;
  delete[] listed;
  listed = NULL;
}


int LittleAreaListWindow::noOfItems ()
{
  return noOfListed;
}


void LittleAreaListWindow::oneLine (int i)
{
  bm.areaList->gotoArea(listed[topline + i]);
  sprintf(list->lineBuf, format, bm.areaList->getTitle());
  interface->charconv_in(list->lineBuf, bm.areaList->isLatin1());
  list->PUTLINE(i, (bm.areaList->isReadonly() ? C_LALISTRO : C_LALIST));
}


searchtype LittleAreaListWindow::oneSearch (int l, const char *what)
{
  bm.areaList->gotoArea(listed[l]);
  return (strexcmp(interface->charconv_in(bm.areaList->isLatin1(),
                                          bm.areaList->getTitle()),
                   what) ? FND_YES : FND_NO);
}


int LittleAreaListWindow::tellArea () const
{
  return listed[active];
}


void LittleAreaListWindow::askFilter ()
{
  char input[QUERYINPUT + 1];

  strncpy(input, (filter ? filter : ""), QUERYINPUT);
  input[QUERYINPUT] = '\0';

  if (interface->QueryBox("Filter on:", input, QUERYINPUT) != GOT_ESC)
  {
    savePos();

    delete[] filter;
    filter = strdupplus(input);
    MakeChain(type);

    // empty list isn't allowed
    if (noOfListed == 0)
    {
      if (*filter) interface->ErrorWindow("No match");
      delete[] filter;
      filter = NULL;
      MakeChain(type);
      restorePos();
    }
  }
}


void LittleAreaListWindow::extrakeys (int key)
{
  switch (key)
  {
    case 'C':
      interface->charsetToggle();
      break;

    case '|':
      askFilter();
      interface->update();
      break;
  }
}


/*
   area list window
*/

AreaListWindow::AreaListWindow ()
{
  oneSearchActive = true;
  listed = NULL;
  filter = NULL;

  areainfo = !bm.resourceObject->isYes(SuppressAreaListInfo);
}


AreaListWindow::~AreaListWindow ()
{
  delete[] filter;
  DestroyChain();
}


void AreaListWindow::MakeActive ()
{
  int col;

  bm.areaList->updateCollectionStatus();

  list_max_x = COLS - 6;
  list_max_y = LINES - 14 + (areainfo ? 0 : 3);

  MakeChain();

  // list may indeed be empty, because the contents of the list may
  // have changed (long/short area list) while a filter is active,
  // but we know that items exist
  if (noOfItems() == 0)
  {
    delete[] filter;
    filter = NULL;

    MakeChain();
  }

  char *topline = new char[list_max_x + 1];
  sprintf(topline, "Message Areas (%d)", noOfItems());

  list = new Window(list_max_y + 3, list_max_x + 2,
                    2, (COLS - list_max_x - 2) >> 1,
                    C_ALBORDER, topline, C_ALTOPTEXT);

  hasPersonal = bm.driverList->hasPersonal();

  list->attrib(C_ALHEADER);
  list->putstring(1, 3, "Area#  Description");
  col = list_max_x - 15;        // 15 = length of "Total   Unread  " - 1
  if (hasPersonal) col -= 11;   // 11 = length of "   Personal"
  list->putstring(1, col, "Total   Unread");
  if (hasPersonal) list->putstring(1, col + 17, "Personal");
                                         // 17 = length of "Total   Unread   "
  col = list_max_x - 28;        // 28 = length of " areano  "
                                //      plus "   Total   Unread  "
  if (hasPersonal) col -= 11;   // 11 = length of "   Personal"
  sprintf(format, "%%c%%6.6s  %%-%d.%ds", col, col);

  if (filter)
  {
    chtype fmark[2] = {' ' | C_ALBORDER, FILTER_SIGN | C_ALBORDER};
    list->putchnstr(0, list_max_x, fmark, 2);
  }

  list->delay_update();

  if (areainfo)
  {
    char format2[11];
    static char info_on[] = "Info on ";
    static char and_area[] = " and Area";
    const char *area = &and_area[5];
    const char *packetName = bm.resourceObject->get(PacketName);

    int maxLen = list_max_x - strlen(info_on) - strlen(and_area) - 4;

    sprintf(format2, "%%s%%.%ds%%s", maxLen);
    if (packetName) sprintf(topline, format2, info_on, packetName, and_area);
    else sprintf(topline, "%s%s", info_on, area);

    int lines = 4;

    info = new ShadowedWin(lines, list_max_x + 2,
                           list_max_y + lines, (COLS - list_max_x - 2) >> 1,
                           C_ALBORDER, topline, C_ALINFODESCR);

    info->putch(0, 0, ACS_LTEE);
    info->putch(0, list_max_x + 1, ACS_RTEE);

    info->attrib(C_ALTOPTEXT);
    info->putstring(0, 3, info_on);
    int offset = slen(packetName);
    if (offset > maxLen) offset = maxLen;
    info->putstring(0, 3 + strlen(info_on) + offset, (packetName ? and_area
                                                                 : area));
    info->attrib(C_ALINFODESCR);
    info->putstring(1, 4, "BBS:");
    info->putstring(2, 2, "Sysop:");
    info->putstring(2, list_max_x - 25, "Type:");

    info->attrib(C_ALINFOTEXT);
    sprintf(topline, "%-65.65s", bm.resourceObject->get(BBSName));
    interface->charconv_in(topline, bm.isLatin1());
    info->putstring(1, 9, topline);

    sprintf(topline, "%-40.40s", bm.resourceObject->get(SysOpName));
    interface->charconv_in(topline, bm.isLatin1());
    info->putstring(2, 9, topline);
  }
  else info = NULL;

  delete[] topline;

  relist();
  Draw();
  Select();
}


void AreaListWindow::Delete ()
{
  delete info;
  delete list;
}


void AreaListWindow::Quit ()
{
  delete[] filter;
  filter = NULL;
}


void AreaListWindow::Touch ()
{
  list->touch();
  if (areainfo) info->touch();
}


void AreaListWindow::MakeChain ()
{
  int noOfAreas = bm.areaList->getNoOfAreas();

  DestroyChain();

  // filter
  oneSearchActive = isOneListed = false;
  listed = new bool[noOfAreas];
  for (int i = 0; i < noOfAreas; i++)
  {
    if (!filter || (oneSearch(i, filter) == FND_YES))
      isOneListed = listed[i] = true;
    else listed[i] = false;
  }

  oneSearchActive = true;

  bm.areaList->setFilter(listed);
  bm.areaList->relist(false);
}


void AreaListWindow::DestroyChain ()
{
  delete[] listed;
  listed = NULL;
}


int AreaListWindow::noOfItems ()
{
  return bm.areaList->getNoOfActive();
}


void AreaListWindow::oneLine (int i)
{
  int nr;
  char *p = list->lineBuf;

  bm.areaList->gotoActive(topline + i);

  p += sprintf(p, format, (bm.areaList->isMarked() ? '=' : ' '),
                          bm.areaList->getNumber(), bm.areaList->getTitle());

  if ((nr = bm.areaList->getNoOfLetters())) p += sprintf(p, "  %5d ", nr);
  else p += sprintf(p, "      . ");

  if ((nr = bm.areaList->getNoOfUnread())) p += sprintf(p, "   %5d   ", nr);
  else p += sprintf(p, "       .   ");

  if (hasPersonal)
  {
    if ((nr = bm.areaList->getNoOfPersonal())) sprintf(p, "   %5d   ", nr);
    else sprintf(p, "       .   ");
  }

  interface->charconv_in(list->lineBuf, bm.areaList->isLatin1());

  list->PUTLINE(i,
               (bm.areaList->isReplyArea()
               ?
               C_ALREPLY | (bm.areaList->getNoOfLetters() ? A_BOLD : A_NORMAL)
               :
               (bm.areaList->getNoOfUnread() ? C_ALISTUNRD : C_ALISTREAD)));

  if ((topline + i == active) && areainfo)
  {
    strcpy(list->lineBuf, bm.areaList->getAreaType());

    if (!bm.areaList->isReplyArea() && !bm.areaList->isCollection())
    {
      if (bm.areaList->isInternet())
      {
        if (bm.areaList->isNetmail()) strcat(list->lineBuf, " Email");
        else strcat(list->lineBuf, " News");
      }
      else
      {
        if (bm.areaList->isNetmail()) strcat(list->lineBuf, " Netmail");
        else strcat(list->lineBuf, " Echo");
      }
    }

    info->clreol(2, list_max_x - 19);
    info->putstring(2, list_max_x - 19, list->lineBuf);
    info->delay_update();
  }
}


searchtype AreaListWindow::oneSearch (int l, const char *what)
{
  if (oneSearchActive) bm.areaList->gotoActive(l);
  else bm.areaList->gotoArea(l);

  return (strexcmp(interface->charconv_in(bm.areaList->isLatin1(),
                                          bm.areaList->getTitle()),
                   what) ? FND_YES : FND_NO);
}


void AreaListWindow::ResetActive ()
{
  active = bm.areaList->getActive();
}


void AreaListWindow::Select ()
{
  bm.areaList->gotoActive(active);
}


void AreaListWindow::FirstUnread ()
{
  int i;
  bool hasLetters = false;

  topline = active = 0;

  for (i = 0; i < noOfItems(); i++)
  {
    bm.areaList->gotoActive(i);
    hasLetters = (hasLetters || (bm.areaList->getNoOfLetters() > 0));
    if (bm.areaList->getNoOfUnread() == 0) Move(DOWN);
    else
    {
      Draw();
      break;
    }
  }

  // all areas read?
  if (i == noOfItems())
  {
    if (hasLetters)
    {
      topline = 0;
      active = -1;
      NextAvail();
    }
    else
    {
      Move(HOME);
      Draw();
    }
  }
}


void AreaListWindow::NextAvail ()
{
  do
  {
    Move(DOWN);
    bm.areaList->gotoActive(active);
  }
  while (bm.areaList->getNoOfLetters() == 0 && active + 1 < noOfItems());

  Draw();
}


void AreaListWindow::PrevAvail ()
{
  do
  {
    Move(UP);
    bm.areaList->gotoActive(active);
  }
  while (bm.areaList->getNoOfLetters() == 0 && active > 0);

  Draw();
}


void AreaListWindow::editMSF ()
{
  int r, stat, dop, areas = 0;
  static const char *fops[] = {"* Read", "~ Reply", "Mark", "Clear ALL", "Quit"};
  static const char *dops[] = {"Clear", "Set", "Toggle", "Quit"};
  static const char *aops[] = {"This area", "Listed areas", "All areas", "Quit"};

  enum {FOPS = 5, FOP_READ = 4, FOP_REPLY = 3, FOP_MARK = 2, FOP_CLEARALL = 1,
        DOPS = 4, DOP_CLEAR = 3, DOP_SET = 2, DOP_TOGGLE = 1,
        AOPS = 4, AOP_THIS = 3, AOP_LISTED = 2, AOP_ALL = 1};

  char area[50];
  sprintf(area, "%.48s:", bm.areaList->getTitle());
  interface->charconv_in(area, bm.areaList->isLatin1());

  if ((r = interface->WarningWindow("Which status flag?", fops, FOPS)) <= WW_NO)
    return;

  switch (r)
  {
    case FOP_READ:
      stat = MSF_READ;
      break;

    case FOP_REPLY:
      stat = MSF_REPLIED;
      break;

    case FOP_MARK:
      stat = MSF_MARKED;
      break;

    default:
      stat = MSF_READ | MSF_REPLIED | MSF_MARKED;
  }

  // check whether status is supported
  if ((r == FOP_READ || r == FOP_REPLY || r == FOP_MARK) &&
      ((bm.areaList->supportedMSF() & stat) == 0))
  {
    interface->ErrorWindow("Status flag not supported by driver!");
    return;
  }

  if (r == FOP_CLEARALL) dop = DOP_CLEAR;
  else if ((dop = interface->WarningWindow("Do what?", dops, DOPS)) <= WW_NO)
    return;

  if ((r = interface->WarningWindow(area, aops, AOPS)) <= WW_NO) return;

  if (r == AOP_THIS) areas = active + 1;
  if (r == AOP_LISTED) areas = bm.areaList->getNoOfActive();
  if (r == AOP_ALL) areas = bm.areaList->getNoOfAreas();

  for (int a = (r == AOP_THIS ? active : 0); a < areas; a++)
  {
    if (r == AOP_ALL) bm.areaList->gotoArea(a);
    else bm.areaList->gotoActive(a);

    if (!bm.areaList->isReplyArea() && (bm.areaList->getNoOfLetters() > 0))
    {
      bm.areaList->getLetterList();

      for (int l = 0; l < bm.letterList->getNoOfLetters(); l++)
      {
        bm.letterList->gotoLetter(l);

        switch (dop)
        {
          case DOP_CLEAR:
            bm.letterList->setStatus(bm.letterList->getStatus() & ~stat);
            break;

          case DOP_SET:
            bm.letterList->setStatus(bm.letterList->getStatus() | stat);
            break;

          case DOP_TOGGLE:
            bm.letterList->setStatus(bm.letterList->getStatus() ^ stat);
            break;
        }
      }

      delete bm.letterList;
      bm.letterList = NULL;
    }
  }

  interface->setAnyRead();
  Select();
}


bool AreaListWindow::makeReply ()
{
  bool update = false;

  if (bm.driverList->canReply())
  {
    if (!bm.areaList->repliesOK())
    {
      interface->ErrorWindow("Cannot save reply in unknown area!");
      return update;
    }

    if (interface->isUnsaved())
    {
      if (interface->WarningWindow("This will overwrite any existing "
                                   "replies. Continue?") == WW_YES)
      {
        if (bm.areaList->makeReply()) interface->setSaved();
        else interface->ErrorWindow("Unable to save replies!");
      }
      update = true;
    }
  }

  return update;
}


void AreaListWindow::askFilter ()
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
    if (!isOneListed)
    {
      if (*filter) interface->ErrorWindow("No match");
      delete[] filter;
      filter = NULL;
      MakeChain();
      restorePos();
    }
  }
}


void AreaListWindow::extrakeys (int key)
{
  Select();

  switch (key)
  {
    case 'A':
      interface->changestate(addressbook);
      break;

    case 'B':
      if (interface->bulletins.noOfItems() == 0)
        interface->ErrorWindow("No bulletins available");
      else interface->changestate(bulletinlist);
      break;

    case 'C':
      interface->charsetToggle();
      break;

    case 'E':
      if (bm.areaList->isReplyArea())
        interface->InfoWindow("Cannot enter letter there");
      else
      {
        interface->letterwin.setLetterParam(bm.areaList->getAreaNo(), IS_NEW);
        if (bm.areaList->isNetmail()) interface->changestate(addressbook);
        interface->letterwin.EnterLetter();
        interface->update();
      }
      break;

    case 'F':
      editMSF();
      interface->update();
      break;

    case 'I':
      areainfo = !areainfo;
      interface->update();
      break;

    case 'L':
      bm.areaList->relist(true);
      ResetActive();
      interface->update();
      break;

    case 'O':
      if (bm.driverList->offlineConfig())
        interface->changestate(offlineconflist);
      else interface->ErrorWindow("Offline configuration is unavailable");
      break;

    case 'S':
      if (bm.resourceObject->isYes(SkipLetterList)) interface->Select();
      break;

    case '!':
      if (makeReply()) interface->update();
      break;

    case '=':
      if (!bm.areaList->isReplyArea())
      {
        bm.areaList->setMarked(!bm.areaList->isMarked());
        interface->setAnyMarked();
        Move(DOWN);
        Draw();
      }
      break;

    case '|':
      askFilter();
      interface->update();
      break;

    case KEY_LEFT:
      PrevAvail();
      break;

    case KEY_RIGHT:
      NextAvail();
      break;
  }
}
