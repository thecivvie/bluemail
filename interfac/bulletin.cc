/*
 * blueMail offline mail reader
 * bulletin list

 Copyright (c) 2003 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "interfac.h"
#include "../common/auxil.h"


BulletinListWindow::BulletinListWindow ()
{
  listed = NULL;
  filter = NULL;
}


BulletinListWindow::~BulletinListWindow ()
{
  delete[] filter;
  DestroyChain();
}


void BulletinListWindow::set (const char **list)
{
  topline = active = 0;
  bulletin = list;
  MakeChain();
}


void BulletinListWindow::MakeActive ()
{
  list_max_y = LINES - 19;     // 6 lines from top, 3 lines border and header,
                               // 10 lines to bottom
  list_max_x = 16;
  sprintf(format, "  %%-%ds", list_max_x - 2);

  list = new Window(list_max_y + 3, list_max_x + 2,
                    6, (COLS - list_max_x - 2) >> 1,
                    C_BULLLBORDER, "Bulletins", C_BULLLTOPTEXT);

  if (filter)
  {
    chtype fmark[2] = {' ' | C_BULLLBORDER, FILTER_SIGN | C_BULLLBORDER};
    list->putchnstr(0, list_max_x, fmark, 2);
  }

  list->delay_update();

  MakeChain();
  relist();
  Draw();
}


void BulletinListWindow::Delete ()
{
  delete list;
}


void BulletinListWindow::Quit ()
{
  delete[] filter;
  filter = NULL;
}


void BulletinListWindow::MakeChain ()
{
  const char **list = bulletin;

  DestroyChain();

  while (list && *list)
  {
    noOfBulletins++;
    list++;
  }

  // filter
  listed = new int[noOfBulletins];
  for (int i = 0; i < noOfBulletins; i++)
  {
    // initialize listed, so that oneSearch() can be used to check filter
    listed[i] = i;

    if (!filter || (oneSearch(i, filter) == FND_YES))
      listed[noOfListed++] = i;
  }
}


void BulletinListWindow::DestroyChain ()
{
  noOfBulletins = noOfListed = 0;
  delete[] listed;
  listed = NULL;
}


void BulletinListWindow::OpenBulletin (int n, bool msg)
{
  FILE *file;

  if ((n < noOfListed) &&
      (file = bm.fileList->ftryopen(bulletin[listed[n]], "rt")))
  {
    interface->ansiView(file, bulletin[listed[n]], bm.isLatin1());
    fclose(file);
  }
  else if (msg) interface->ErrorWindow("Could not open bulletin file");
}


void BulletinListWindow::OpenBulletin ()
{
  OpenBulletin(active, true);
}


int BulletinListWindow::noOfItems ()
{
  return noOfListed;
}


void BulletinListWindow::oneLine (int i)
{
  sprintf(list->lineBuf, format, bulletin[listed[topline + i]]);
  list->PUTLINE(i, C_BULLLIST);
}


searchtype BulletinListWindow::oneSearch (int l, const char *what)
{
  return (strexcmp(bulletin[listed[l]], what) ? FND_YES : FND_NO);
}


void BulletinListWindow::askFilter ()
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


void BulletinListWindow::extrakeys (int key)
{
  switch (key)
  {
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
