/*
 * blueMail offline mail reader
 * offline configuration list

 Copyright (c) 2003 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "interfac.h"
#include "../common/auxil.h"


OfflineConfigListWindow::OfflineConfigListWindow ()
{
  noOfAreas = noOfListed = 0;
  listed = NULL;
  filter = NULL;
}


OfflineConfigListWindow::~OfflineConfigListWindow ()
{
  delete[] filter;
  DestroyChain();
}


void OfflineConfigListWindow::MakeActive ()
{
  list_max_y = LINES - 8;   // 1 line from top, 3 lines border and header,
                            // 4 lines to bottom
  list_max_x = COLS - 6;

  sprintf(format, "  %%c   %%-%d.%ds  ", list_max_x - 8, list_max_x - 8);

  list = new Window(list_max_y + 3, list_max_x + 2,
                    1, (COLS - list_max_x - 2) >> 1,
                    C_OFFCFLBORDER, "Offline Configuration of Areas",
                    C_OFFCFLTOPTEXT);

  if (filter)
  {
    chtype fmark[2] = {' ' | C_OFFCFLBORDER, FILTER_SIGN | C_OFFCFLBORDER};
    list->putchnstr(0, list_max_x, fmark, 2);
  }

  list->delay_update();

  MakeChain();
  relist();
  Draw();
  Select();
}


void OfflineConfigListWindow::Delete ()
{
  delete list;
}


void OfflineConfigListWindow::Quit ()
{
  delete[] filter;
  filter = NULL;
}


void OfflineConfigListWindow::MakeChain ()
{
  bool real, pseudo;

  DestroyChain();

  int count = -1;
  (void) bm.getExtraArea(count);
  noOfAreas = bm.areaList->getNoOfAreas() + count;

  count = 0;

  // filter
  listed = new int[noOfAreas];
  for (int i = 0; i < noOfAreas; i++)
  {
    real = (i < bm.areaList->getNoOfAreas());
    pseudo = false;

    extraline = -1;

    if (real)
    {
      bm.areaList->gotoArea(i);
      pseudo = (strcmp(bm.areaList->getTitle(), AREA_EXTRA) == 0);
    }
    else extraline = i;

    // initialize listed, so that oneSearch() can be used to check filter
    listed[i] = i;

    if (!real || pseudo || (!bm.areaList->isReplyArea() &&
                            !bm.areaList->isCollection()))
      if (!filter || (oneSearch(i, filter) == FND_YES))
      {
        listed[noOfListed++] = i;
        if (pseudo) count = 1;
      }
  }

  // list is empty during filter "no match" error
  if (noOfListed)
  {
    sprintf(list->lineBuf, " (%d)", noOfListed - count);
    list->attrib(C_OFFCFLTOPTEXT);
    list->putstring(0, 33, list->lineBuf);
    list->attrib(C_OFFCFLBORDER);
    list->putch(0, 33 + strlen(list->lineBuf), ACS_LTEE);
    list->delay_update();
  }
}


void OfflineConfigListWindow::DestroyChain ()
{
  noOfListed = 0;
  delete[] listed;
  listed = NULL;
}


void OfflineConfigListWindow::Select ()
{
  extraline = -1;

  if (listed[active] < bm.areaList->getNoOfAreas())
    bm.areaList->gotoArea(listed[active]);
  else extraline = listed[active];
}


int OfflineConfigListWindow::noOfItems ()
{
  return noOfListed;
}


char OfflineConfigListWindow::SelectionMark ()
{
  // extra areas
  if (extraline != -1)
  {
    int extra = extraline - bm.areaList->getNoOfAreas();
    (void) bm.getExtraArea(extra);
    return (extra ? '+' : ' ');
  }

  // real areas

  if (!bm.areaList->isForced() && !bm.areaList->isSubscribed())
    return (bm.areaList->isAdded() ? '+' : ' ');

  if (!bm.areaList->isForced() && bm.areaList->isSubscribed())
    return (bm.areaList->isDropped() ? '-' : '*');

  if (bm.areaList->isForced() && !bm.areaList->isSubscribed())
    return (bm.areaList->isAdded() ? '+' : '!');

  return '!';
}


void OfflineConfigListWindow::oneLine (int i)
{
  const char *title;
  bool latin1;

  extraline = -1;

  // real areas
  if (listed[topline + i] < bm.areaList->getNoOfAreas())
  {
    bm.areaList->gotoArea(listed[topline + i]);
    title = bm.areaList->getTitle();
    latin1 = bm.areaList->isLatin1();
  }
  // extra areas
  else
  {
    extraline = listed[topline + i];
    int extra = extraline - bm.areaList->getNoOfAreas();
    title = bm.getExtraArea(extra);
    latin1 = bm.isLatin1();
  }

  char mark = SelectionMark();

  sprintf(list->lineBuf, format, mark, title);
  interface->charconv_in(list->lineBuf, latin1);
  list->PUTLINE(i, (mark == '+' ? C_OFFCFLISTADD
                                : (mark == '-' ? C_OFFCFLISTDROP
                                               : C_OFFCFLIST)));
}


searchtype OfflineConfigListWindow::oneSearch (int l, const char *what)
{
  const char *title;
  bool latin1;
  bool found1, found2;
  int isNot;

  extraline = -1;

  // real areas
  if (listed[l] < bm.areaList->getNoOfAreas())
  {
    bm.areaList->gotoArea(listed[l]);
    title = bm.areaList->getTitle();
    latin1 = bm.areaList->isLatin1();
  }
  // extra areas
  else
  {
    extraline = listed[l];
    int extra = extraline - bm.areaList->getNoOfAreas();
    title = bm.getExtraArea(extra);
    latin1 = bm.isLatin1();
  }

  found1 = (strexcmp(interface->charconv_in(latin1, title), what) != NULL);

  char mark[3];
  mark[0] = SelectionMark();
  mark[1] = ' ';
  mark[2] = '\0';

  found2 = (strexcmp(mark, what) != NULL);

  (void) strex(isNot, what);
  return ((isNot ? found1 && found2 : found1 || found2) ? FND_YES : FND_NO);
}


bool OfflineConfigListWindow::toggleSelection ()
{
  int extra = extraline - bm.areaList->getNoOfAreas();

  switch (SelectionMark())
  {
    case '!':
      if (!bm.areaList->isSubscribed()) bm.areaList->setAdded(true);
      else return false;
      break;

    case ' ':
      // real area
      if (extraline == -1)
      {
        // enter extra area
        if (strcmp(bm.areaList->getTitle(), AREA_EXTRA) == 0) askSubscribe();
        else bm.areaList->setAdded(true);
      }
      // extra area
      else bm.setExtraArea(extra, true);
      break;

    case '*':
      bm.areaList->setDropped(true);
      break;

    case '+':
      // real areas
      if (extraline == -1)
      {
        bm.areaList->setAdded(true);
        bm.areaList->setDropped();
      }
      // extra areas
      else bm.setExtraArea(extra, false);
      break;

    case '-':
      bm.areaList->setDropped(true);
      bm.areaList->setAdded();
      break;
  }

  return true;
}


void OfflineConfigListWindow::askSubscribe ()
{
  char input[NEWSGROUPLEN + 1];

  *input = '\0';

  if (interface->QueryBox("Subscribe:", input, NEWSGROUPLEN) != GOT_ESC)
  {
    if (isDupe(input)) interface->ErrorWindow("Area already exists!");
    else bm.setExtraArea(-1, true, input);

    interface->update();
    gotoArea(input);
  }
  else interface->update();
}


bool OfflineConfigListWindow::isDupe (const char *name)
{
  const char *title;

  for (int i = 0; i < noOfAreas; i++)
  {
    extraline = -1;

    // real areas
    if (i < bm.areaList->getNoOfAreas())
    {
      bm.areaList->gotoArea(i);
      title = bm.areaList->getTitle();
    }
    // extra areas
    else
    {
      extraline = i;
      int extra = extraline - bm.areaList->getNoOfAreas();
      title = bm.getExtraArea(extra);
    }

    if (strcmp(name, title) == 0) return true;
  }

  return false;
}


void OfflineConfigListWindow::gotoArea (const char *name)
{
  int p, oldtop = topline;
  const char *title;

  savePos();
  Move(HOME);

  for (p = 0; p < noOfListed; p++)
  {
    extraline = -1;

    if (listed[p] < bm.areaList->getNoOfAreas())
    {
      bm.areaList->gotoArea(listed[p]);
      title = bm.areaList->getTitle();
    }
    else
    {
      extraline = listed[p];
      int extra = extraline - bm.areaList->getNoOfAreas();
      title = bm.getExtraArea(extra);
    }

    if (strcmp(name, title) != 0) Move(DOWN);
    else break;
  }

  if (p == noOfListed) restorePos();
  else if (p >= oldtop && p < oldtop + list_max_y) topline = oldtop;
}


void OfflineConfigListWindow::askFilter ()
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
    if (noOfListed == 0)
    {
      if (*filter) interface->ErrorWindow("No match");
      delete[] filter;
      filter = NULL;
      MakeChain();
      restorePos();
    }
  }
}


void OfflineConfigListWindow::extrakeys (int key)
{
  Select();

  bool down = (extraline != -1 ||
               strcmp(bm.areaList->getTitle(), AREA_EXTRA) != 0);

  switch (key)
  {
    case META_SPACE:
      if (toggleSelection())
      {
        interface->setUnsaved();
        if (down) Move(DOWN);
        Draw();
      }
      else interface->delay_beep();
      break;

    case '|':
      askFilter();
      interface->update();
      break;

    case KEY_LEFT:
      Move(UP);
      Draw();
      break;

    case KEY_RIGHT:
      Move(DOWN);
      Draw();
      break;
  }
}
