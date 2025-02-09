/*
 * blueMail offline mail reader
 * mail file/database list

 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "interfac.h"
#include "../bluemail/driverl.h"
#include "../common/auxil.h"


FILEDB FileDBListWindow::filedb[] =
{
#ifdef DRV_BBBS
  {BBBSMsgBaseDir, NULL, "BBBS Message Base", false},
#endif
#ifdef DRV_HMB
  {HudsonMsgBaseDir, NULL, "Hudson Message Base", false},
#endif
#ifdef DRV_MBOX
  {mboxFile, NULL, "Mail File", true},
  {MboxesDir, NULL, "Mail File Archive", false},
#endif
  {internalFIRST, NULL, NULL, false}
};


FileDBListWindow::FileDBListWindow ()
{
  noOfFileDBs = noOfListed = 0;
  listed = NULL;
  filter = NULL;

  for (int i = 0; filedb[i].name != NULL; i++)
    if ((filedb[i].defined = bm.resourceObject->get(filedb[i].id)))
      noOfFileDBs++;

  MakeChain();
}


FileDBListWindow::~FileDBListWindow ()
{
  delete[] filter;
  DestroyChain();
}


void FileDBListWindow::MakeActive ()
{
  list_max_y = LINES - 18;     // 9 lines from top, 3 lines border and header,
                               // 6 lines to bottom
  list_max_x = 26;
  sprintf(format, "  %%-%ds", list_max_x - 2);

  list = new Window(list_max_y + 3, list_max_x + 2,
                    9, (COLS - list_max_x - 2) >> 1, C_FLBORDER);
  list->attrib(C_FLHEADER);
  list->putstring(1, 3, "File or Database");

  if (filter)
  {
    chtype fmark[2] = {' ' | C_FLBORDER, FILTER_SIGN | C_FLBORDER};
    list->putchnstr(0, list_max_x, fmark, 2);
  }

  list->delay_update();

  MakeChain();
  relist();
  Draw();
}


void FileDBListWindow::Delete ()
{
  delete list;
}


void FileDBListWindow::Quit ()
{
  delete[] filter;
  filter = NULL;
}


void FileDBListWindow::MakeChain ()
{
  DestroyChain();

  // filter
  listed = new int[noOfFileDBs];
  for (int i = 0; i < noOfFileDBs; i++)
  {
    // initialize listed, so that oneSearch() can be used to check filter
    listed[i] = i;

    if (!filter || (oneSearch(i, filter) == FND_YES))
      listed[noOfListed++] = i;
  }
}


void FileDBListWindow::DestroyChain ()
{
  noOfListed = 0;
  delete[] listed;
  listed = NULL;
}


int FileDBListWindow::getFileDBItem (int i)
{
  int j = 0;

  for (j = 0; filedb[j].name != NULL; j++)
    if (filedb[j].defined)
      if (i-- == 0) break;

  return j;
}


srvcerror FileDBListWindow::OpenFileDB ()
{
  srvcerror result = SRVC_DONE;

  int a = getFileDBItem(listed[active]);

  if (filedb[a].id == MboxesDir)
  {
    interface->files.setFileType(FLT_MBOXES);
    interface->files.MakeChain();
    if (interface->files.noOfItems() == 0) result = SRVC_NO_PACKET;
    else interface->changestate(mboxlist);
  }
  else result = (bm.selectFileDB(filedb[a].defined, filedb[a].isFile) ? SRVC_OK
                                                                      : SRVC_ERROR);
  return result;
}


int FileDBListWindow::noOfItems ()
{
  return noOfListed;
}


void FileDBListWindow::oneLine (int i)
{
  sprintf(list->lineBuf, format, filedb[getFileDBItem(listed[topline + i])].name);
  list->PUTLINE(i, C_FLIST);
}


searchtype FileDBListWindow::oneSearch (int l, const char *what)
{
  return (strexcmp(filedb[getFileDBItem(listed[l])].name, what) ? FND_YES
                                                                : FND_NO);
}


void FileDBListWindow::askFilter ()
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


void FileDBListWindow::extrakeys (int key)
{
  switch (key)
  {
#ifdef DRV_MBOX
    case META_S1:
      for (int i = 0; i < noOfListed; i++)
        if (filedb[getFileDBItem(listed[i])].id == MboxesDir)
        {
          active = i;
          Draw();
          interface->Select();
          break;
        }
      break;
#endif
    case 'A':
      interface->changestate(addressbook);
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
