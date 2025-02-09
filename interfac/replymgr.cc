/*
 * blueMail offline mail reader
 * reply manager list

 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <ctype.h>
#include <string.h>
#include "interfac.h"
#include "../common/auxil.h"


ReplyMgrListWindow::ReplyMgrListWindow ()
{
  noOfListed = 0;
  fileList = NULL;
  listedInf = NULL;
  filter = NULL;

  const char *ssb = bm.resourceObject->get(SortSystemsBy);
  infsort = (ssb ? toupper(*ssb) == 'N' : true);
}


ReplyMgrListWindow::~ReplyMgrListWindow ()
{
  delete[] filter;
  DestroyChain();
}


void ReplyMgrListWindow::MakeActive ()
{
  list_max_y = LINES - 18;     // 9 lines from top, 3 lines border and header,
                               // 6 lines to bottom
  list_max_x = 62;

  list = new Window(list_max_y + 3, list_max_x + 2,
                    9, (COLS - list_max_x - 2) >> 1, C_REPMLBORDER);
  list->attrib(C_REPMLHEADER);
  list->putstring(1, 3, "System");

  if (bm.resourceObject->isYes(DrawSortMark) && infsort)
  {
    list->putstring(1, 9, "( )");
    list->putch(1, 10, ACS_DARROW);
  }

  if (filter)
  {
    chtype fmark[2] = {' ' | C_REPMLBORDER, FILTER_SIGN | C_REPMLBORDER};
    list->putchnstr(0, list_max_x, fmark, 2);
  }

  list->delay_update();

  // MakeChain() is public should have been called prior to MakeActive()
  if (!listedInf) MakeChain();

  relist();
  Draw();
  Select();
}


void ReplyMgrListWindow::Delete ()
{
  delete list;
}


void ReplyMgrListWindow::Quit ()
{
  delete[] filter;
  filter = NULL;
}


void ReplyMgrListWindow::MakeChain ()
{
  DestroyChain();

  fileList = new file_list(bm.resourceObject->get(InfDir));

  int noOfFiles = fileList->getNoOfFiles();

  // sort
  fileList->sort(infsort ? FL_SORT_BY_NAME : FL_SORT_BY_NEWEST);

  // filter
  listedInf = new int[noOfFiles];
  for (int i = 0; i < noOfFiles; i++)
  {
    // initialize listed, so that oneSearch() can be used to check filter
    listedInf[i] = i;

    fileList->gotoFile(i);

    if (bm.service->isSystemInfFile(fileList->getName()) &&
        (!filter || (oneSearch(i, filter) == FND_YES)))
      listedInf[noOfListed++] = i;
  }
}


void ReplyMgrListWindow::DestroyChain ()
{
  delete fileList;
  fileList = NULL;

  noOfListed = 0;
  delete[] listedInf;
  listedInf = NULL;
}


void ReplyMgrListWindow::Select ()
{
  fileList->gotoFile(listedInf[active]);
}


bool ReplyMgrListWindow::OpenReply ()
{
  Select();
  return bm.selectReply(fileList->getName());
}


int ReplyMgrListWindow::noOfItems ()
{
  return noOfListed;
}


void ReplyMgrListWindow::oneLine (int i)
{
  fileList->gotoFile(listedInf[topline + i]);
  const char *name = fileList->getName();

  bool oldReplyPacket = (replyPacket() != NULL);
  bool mark = oldReplyPacket && bm.resourceObject->isYes(DrawReplyMark);

  sprintf(list->lineBuf, "%c %-8.8s   %-48.48s ",
          (mark ? '~' : ' '), strupper(stem(name)),
          bm.service->getSystemName(name));

  list->PUTLINE(i, (oldReplyPacket ? C_REPMLPACKET : C_REPMLIST));
}


searchtype ReplyMgrListWindow::oneSearch (int l, const char *what)
{
  bool found1, found2, found3, found;
  int isNot;

  fileList->gotoFile(listedInf[l]);
  const char *name = fileList->getName();

  found1 = (strexcmp(stem(name), what) != NULL);
  found2 = (strexcmp(bm.service->getSystemName(name), what) != NULL);

  (void) strex(isNot, what);
  found = (isNot ? found1 && found2 : found1 || found2);

  if (replyPacket() && bm.resourceObject->isYes(DrawReplyMark))
  {
    found3 = (strexcmp("~ ", what) != NULL);
    found = (isNot ? found && found3 : found || found3);
  }

  return (found ? FND_YES : FND_NO);
}


const char *ReplyMgrListWindow::replyPacket ()
{
  static char fname[MYMAXPATH];
  const char *infName = fileList->getName();
  const char *replyName = bm.service->getReplyName(infName);

  if (*replyName == '\0') replyName = infName;

  mkfname(fname, bm.resourceObject->get(ReplyDir),
                 ext(replyName, bm.service->getReplyExtension(infName)));

  return (access(fname, R_OK | W_OK) == 0 ? fname : NULL);
}


void ReplyMgrListWindow::kill ()
{
  char fname[MYMAXPATH];
  const char *reply = replyPacket();
  const char *name = fileList->getName();
  const char *killopts[] = {"System", "Reply", "Both", "Quit"};
  int r = DEL_INF;

  if (reply) r = interface->WarningWindow("Which files do you want to delete?",
                                          killopts, 4);
  if (r > WW_NO)
  {
    if (r == DEL_INF || r == DEL_BOTH)
    {
      sprintf(fname, "Do you really want to delete system file %.8s?",
                     strupper(stem(name)));

      int w = (reply ? WW_YES : interface->WarningWindow(fname));

      if (w == WW_YES)
      {
        bm.service->killSystemInfFile(name);

        mkfname(fname, bm.resourceObject->get(InfDir), bm.getAreaMarksFile(name));

        if (access(fname, R_OK | W_OK) == 0)
          if (interface->WarningWindow("Information about marked areas exists. "
                                       "Delete it, too?") == WW_YES)
            remove(fname);

        checkDel();
        MakeChain();
      }
    }

    if (r == DEL_REPLY || r == DEL_BOTH) remove(reply);
  }
}


void ReplyMgrListWindow::askFilter ()
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


void ReplyMgrListWindow::extrakeys (int key)
{
  Select();

  switch (key)
  {
    case 'A':
      interface->changestate(addressbook);
      break;

    case 'K':
    case KEY_DC:
      kill();
      if (noOfListed == 0)
      {
        Quit();
        interface->back();
      }
      else interface->update();
      break;

    case 'O':
      infsort = !infsort;
      MakeChain();
      interface->update();
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
