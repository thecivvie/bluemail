/*
 * blueMail offline mail reader
 * file list

 Copyright (c) 1996 Kolossvary Tamas <thomas@vma.bme.hu>
 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <ctype.h>
#include <string.h>
#include "interfac.h"
#include "../common/auxil.h"
#include "../common/error.h"
#include "../common/mysystem.h"


FileListWindow::FileListWindow ()
{
  noOfListed = 0;
  fileList = NULL;
  listed = NULL;
  filter = NULL;
  fileType = FLT_UNKNOWN;
  fileStat = NULL;

  const char *spb = bm.resourceObject->get(SortFilesBy);
  namesort = (spb ? toupper(*spb) == 'N' : true);
}


FileListWindow::~FileListWindow ()
{
  delete fileStat;
  delete[] filter;
  DestroyChain();
}


void FileListWindow::MakeActive ()
{
  list_max_y = LINES - 18;     // 9 lines from top, 3 lines border and header,
                               // 6 lines to bottom
  list_max_x = 62;

  list = new Window(list_max_y + 3, list_max_x + 2,
                    9, (COLS - list_max_x - 2) >> 1, C_FLBORDER);
  list->attrib(C_FLHEADER);
  list->putstring(1, 3, (fileType == FLT_PACKETS ? "Packet" : "File"));
  list->putstring(1, 19, "Date");
  list->putstring(1, 42, "Size");
  list->putstring(1, 49, "Unread");
  list->putstring(1, 56, "Total");

  if (bm.resourceObject->isYes(DrawSortMark))
  {
    const char *smark = "( )";

    if (namesort)
    {
      list->putstring(1, 9, smark);
      list->putch(1, 10, ACS_DARROW);
    }
    else
    {
      list->putstring(1, 24, smark);
      list->putch(1, 25, ACS_UARROW);
    }
  }

  // MakeChain() is public should have been called prior to MakeActive()
  if (!listed) MakeChain();

  // list may indeed be empty, because the contents of the list
  // may have changed (new file name) while a filter is active,
  // but we know that items exist
  if (noOfItems() == 0)
  {
    delete[] filter;
    filter = NULL;

    MakeChain();
  }

  if (filter)
  {
    chtype fmark[2] = {' ' | C_FLBORDER, FILTER_SIGN | C_FLBORDER};
    list->putchnstr(0, list_max_x, fmark, 2);
  }

  list->delay_update();

  relist();
  Draw();
  Select();
}


void FileListWindow::Delete ()
{
  delete list;
}


void FileListWindow::Quit ()
{
  fileType = FLT_UNKNOWN;
  delete[] filter;
  filter = NULL;
}


void FileListWindow::MakeChain ()
{
  const char *dir = NULL;
  int dummy;

  DestroyChain();

  if (!fileStat) fileStat = new file_stat(&bm);

  switch (fileType)
  {
    case FLT_PACKETS:
      dir = bm.resourceObject->get(PacketDir);
      break;

    case FLT_MBOXES:
      dir = bm.resourceObject->get(MboxesDir);
      break;
  }

  fileList = new file_list(dir);

  int noOfFiles = fileList->getNoOfFiles();

  // sort
  fileList->sort(namesort ? FL_SORT_BY_NAME : FL_SORT_BY_NEWEST);

  // filter
  listed = new int[noOfFiles];
  for (int i = 0; i < noOfFiles; i++)
  {
    fileList->gotoFile(i);
    // mark as used
    fileStat->getStat(fileList->getID(), fileList->getDate(), &dummy, &dummy);

    // initialize listed, so that oneSearch() can be used to check filter
    listed[i] = i;

    if (!filter || (oneSearch(i, filter) == FND_YES))
      listed[noOfListed++] = i;
  }
}


void FileListWindow::DestroyChain ()
{
  delete fileList;
  fileList = NULL;

  noOfListed = 0;
  delete[] listed;
  listed = NULL;
}


void FileListWindow::Select ()
{
  fileList->gotoFile(listed[active]);
}


void FileListWindow::setFileType (int type)
{
  fileType = type;
}


bool FileListWindow::OpenFile ()
{
  Select();

  switch (fileType)
  {
    case FLT_PACKETS:
      return bm.selectPacket(fileList->getName());

    case FLT_MBOXES:
      return bm.selectMbox(fileList->getName());

    default:
      return false;
  }
}


int FileListWindow::noOfItems ()
{
  return noOfListed;
}


char *FileListWindow::NameDate (char *p)
{
  const char *fname = fileList->getName();
  time_t ftime = fileList->getDate();

  strcpy(p, "          ");
  for (int j = 2;
       *fname && ((fileType == FLT_MBOXES || *fname != '.') && j < 10);
       j++) p[j] = *fname++;
  p += 10;

  p += sprintf(p, (fileType == FLT_PACKETS ? "%-4.4s    " : "%-6.6s  "), fname);
  p += strftime(p, 18, "%d %b %Y %H:%M", localtime(&ftime));

  return p;
}


void FileListWindow::oneLine (int i)
{
  int unread, total;
  char *p = list->lineBuf;

  fileList->gotoFile(listed[topline + i]);

  p = NameDate(p);
  p += sprintf(p, "%10ld    ", (long) fileList->getSize());

  if (fileStat->getStat(fileList->getID(), fileList->getDate(), &total, &unread))
    sprintf(p, "%5d %5d  ", unread, total);
  else strcat(p, "             ");

  list->PUTLINE(i, C_FLIST);
}


searchtype FileListWindow::oneSearch (int l, const char *what)
{
  fileList->gotoFile(listed[l]);
  (void) NameDate(list->lineBuf);
  return (strexcmp(list->lineBuf, what) ? FND_YES : FND_NO);
}


void FileListWindow::renameFile ()
{
  int num = 0;
  char msg[27], input[QUERYINPUT + 1];
  const char *fname = fileList->getName();

  sprintf(msg, "Rename %.12s%s to:", fname, (strlen(fname) > 12 ? "..." : ""));

  if (getNumExt(fname) != -1 || (num = fileList->nextNumExt(fname)) == -1)
    strncpy(input, fname, QUERYINPUT);
  else
  {
    sprintf(input, "%03d", num);
    strncpy(input, ext(fname, input), QUERYINPUT);
  }

  input[QUERYINPUT] = '\0';

  if (interface->QueryBox(msg, input, QUERYINPUT, false) != GOT_ESC &&
      strcmp(fname, input) != 0 && *input != '\0')
  {
    if (fileList->exists(input))
      interface->ErrorWindow("Such a file already exists");
    else
    {
      Select();

      if (fileList->changeName(input)) MakeChain();
      else interface->ErrorWindow("Rename failed");
    }
  }
}


void FileListWindow::askFilter ()
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


void FileListWindow::extrakeys (int key)
{
  char fname[MYMAXPATH];

  Select();

  switch (key)
  {
    case 'A':
      interface->changestate(addressbook);
      break;

    case 'K':
    case KEY_DC:
      char msg[46];
      sprintf(msg, "Do you really want to delete %.12s%s?",
                   fileList->getName(),
                   (strlen(fileList->getName()) > 12 ? "..." : ""));
      if (interface->WarningWindow(msg) == WW_YES)
      {
        mkfname(fname,
                (fileType == FLT_PACKETS ? bm.resourceObject->get(PacketDir)
                                         : bm.resourceObject->get(MboxesDir)),
                fileList->getName());
        remove(fname);
        checkDel();
        MakeChain();
      }
      if (noOfListed == 0)
      {
        Quit();
        interface->back();
      }
      else interface->update();
      break;

    case 'O':
      namesort = !namesort;
      // no break here!
    case '!':
      MakeChain();
      interface->update();
      break;

    case 'R':
      renameFile();
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
