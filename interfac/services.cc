/*
 * blueMail offline mail reader
 * service list

 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "interfac.h"
#include "../common/auxil.h"


SERVICE ServiceListWindow::service[] =
{
  {'P', "Offline Mail Packets"},
  {'F', "Mail Files/Databases"},
//  {'C', "Online Connections"},
  {'R', "Reply Packet Manager"},
  {'D', "Demo Service"}              // must be last
};


ServiceListWindow::ServiceListWindow ()
{
  noOfServices = (bm.resourceObject->isYes(OmitDemoService) ? 3 : 4);
  noOfListed = 0;
  listed = NULL;
  filter = NULL;
}


ServiceListWindow::~ServiceListWindow ()
{
  delete[] filter;
  DestroyChain();
}


void ServiceListWindow::MakeActive ()
{
  list_max_y = LINES - 18;     // 9 lines from top, 3 lines border and header,
                               // 6 lines to bottom
  list_max_x = 28;
  sprintf(format, "   %%-%ds", list_max_x - 3);

  list = new Window(list_max_y + 3, list_max_x + 2,
                    9, (COLS - list_max_x - 2) >> 1,
                    C_SLBORDER, "Services available", C_SLTOPTEXT);

  if (filter)
  {
    chtype fmark[2] = {' ' | C_SLBORDER, FILTER_SIGN | C_SLBORDER};
    list->putchnstr(0, list_max_x, fmark, 2);
  }

  list->delay_update();

  MakeChain();
  relist();
  Draw();
}


void ServiceListWindow::Delete ()
{
  delete list;
}


void ServiceListWindow::Quit ()
{
  // quitting this list window will quit the program, too,
  // thus calling ~ServiceListWindow, which is sufficient
}


void ServiceListWindow::MakeChain ()
{
  DestroyChain();

  // filter
  listed = new int[noOfServices];
  for (int i = 0; i < noOfServices; i++)
  {
    // initialize listed, so that oneSearch() can be used to check filter
    listed[i] = i;

    if (!filter || (oneSearch(i, filter) == FND_YES))
      listed[noOfListed++] = i;
  }
}


void ServiceListWindow::DestroyChain ()
{
  noOfListed = 0;
  delete[] listed;
  listed = NULL;
}


srvcerror ServiceListWindow::OpenService ()
{
  srvcerror result = SRVC_DONE;

  switch(service[listed[active]].key)
  {
//    case 'C':
//      interface->changestate(connectlist) OR error
//      break;

    case 'D':
      result = (bm.selectDemo() ? SRVC_OK : SRVC_ERROR);
      break;

    case 'F':
      if (interface->filedbs.noOfItems() == 0) result = SRVC_NO_FILEDB;
      else interface->changestate(filedblist);
      break;

    case 'P':
      interface->files.setFileType(FLT_PACKETS);
      interface->files.MakeChain();
      if (interface->files.noOfItems() == 0)
      {
        if (bm.resourceObject->isYes(CallReplyMgr)) extrakeys('R');
        else result = SRVC_NO_PACKET;
      }
      else interface->changestate(packetlist);
      break;

    case 'R':
      interface->systems.MakeChain();
      if (interface->systems.noOfItems() == 0) result = SRVC_NO_REPLY_INF;
      else interface->changestate(replymgrlist);
      break;
  }

  return result;
}


int ServiceListWindow::noOfItems ()
{
  return noOfListed;
}


void ServiceListWindow::oneLine (int i)
{
  sprintf(list->lineBuf, format, service[listed[topline + i]].name);
  list->PUTLINE(i, C_SLIST);
}


searchtype ServiceListWindow::oneSearch (int l, const char *what)
{
  return (strexcmp(service[listed[l]].name, what) ? FND_YES : FND_NO);
}


void ServiceListWindow::askFilter ()
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


void ServiceListWindow::extrakeys (int key)
{
  switch (key)
  {
    case 'A':
      interface->changestate(addressbook);
      break;

    case 'D':
      if (bm.resourceObject->isYes(OmitDemoService)) break;
      // no break here!
//    case 'C':
    case 'F':
    case 'P':
    case 'R':
      for (int i = 0; i < noOfListed; i++)
        if (service[listed[i]].key == key)
        {
          active = i;
          Draw();
          interface->Select();
          break;
        }
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
