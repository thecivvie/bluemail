/*
 * blueMail offline mail reader
 * tagline window

 Copyright (c) 1996 Kolossvary Tamas <thomas@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <stdlib.h>
#include <string.h>
#include "interfac.h"
#include "../common/auxil.h"


TaglineWindow::Tagline::Tagline ()
{
  *text = '\0';
  next = NULL;
}


TaglineWindow::TaglineWindow ()
{
  isActive = false;     // not yet made active
  tagsort = false;
  fake = false;         // see EnterTagline() and oneLine() for this

  noOfTaglines = noOfListed = 0;
  tagline = NULL;
  first = NULL;
  listed = NULL;
  filter = NULL;

  MakeChain();
}


TaglineWindow::~TaglineWindow ()
{
  delete[] filter;
  DestroyChain();
}


void TaglineWindow::MakeActive ()
{
  isActive = true;

  interface->letterwin.setLetterParam(NULL);

  list_max_x = COLS - 4;
  list_max_y = LINES - 15;

  int maxLen = (list_max_x < TAGLINE_LENGTH ? list_max_x : TAGLINE_LENGTH);
  sprintf(format, "%%-%d.%ds", maxLen, maxLen);

  static char title[] = "Select Tagline( )";
  bool smark = (tagsort && bm.resourceObject->isYes(DrawSortMark));
  title[14] = (smark ? '(' : '\0');

  int x = (COLS - list_max_x - 2) >> 1;

  list = new Window(list_max_y + 5, list_max_x + 2, 5, x,
                    C_TAGBOXBORDER, title, C_TAGBOXTTEXT, 5, 1);

  if (smark)
  {
    list->attrib(C_TAGBOXTTEXT);
    list->putch(0, 18, ACS_DARROW);
  }

  list->attrib(C_TAGBOXBORDER);
  list->horizline(list_max_y + 1, list_max_x);

  int midpos = interface->midpos((x + 1) << 1, 21, 20, 17) + 1;
  int endpos = interface->endpos((x + 1) << 1, 17) + 1;
  // 21 is max length of all strings (left column) below
  // 20 is max length of all strings (middle column) below
  // 17 is max length of all strings (right column) below

  list->attrib(C_TAGBOXKEYS);
  list->putstring(list_max_y + 2, 3, "E");
  list->putstring(list_max_y + 3, 2, "^E");
  list->putstring(list_max_y + 2, midpos, "K");
  list->putstring(list_max_y + 3, midpos, "O");
  list->putstring(list_max_y + 2, endpos, "R");
  list->putstring(list_max_y + 3, endpos, "Q");

  list->attrib(C_TAGBOXDESCR);
  list->putstring(list_max_y + 2, 4, ": Enter new tagline");
  list->putstring(list_max_y + 3, 4, ": Edit tagline");
  list->putstring(list_max_y + 2, midpos + 1, ": Kill tagline");
  list->putstring(list_max_y + 3, midpos + 1, ": change sort Order");
  list->putstring(list_max_y + 2, endpos + 1, ": Random tagline");
  list->putstring(list_max_y + 3, endpos + 1, ": Quit taglines");

  if (filter)
  {
    chtype fmark[2] = {' ' | C_TAGBOXBORDER, FILTER_SIGN | C_TAGBOXBORDER};
    list->putchnstr(0, list_max_x, fmark, 2);
  }

  list->delay_update();

  relist();
  Draw();
}


void TaglineWindow::Delete ()
{
  delete list;
  isActive = false;
}


void TaglineWindow::Quit ()
{
  if (filter)
  {
    delete[] filter;
    filter = NULL;
    MakeChain();     // call only, when filter changes, not in MakeActive()
  }
}


void TaglineWindow::MakeChain ()
{
  FILE *f;
  Tagline head, *curr;

  if ((f = fopen(bm.resourceObject->get(taglineFile), "rt")))
  {
    DestroyChain();
    curr = &head;

    while (fgetsnl(head.text, TAGLINE_LENGTH + 1, f))
    {
      mkstr(head.text);

      // no blank lines
      if (*head.text)
      {
        curr->next = new Tagline;
        curr = curr->next;
        strcpy(curr->text, head.text);
        noOfTaglines++;
      }
    }

    fclose(f);
  }

  // fill index array
  if (noOfTaglines)
  {
    tagline = new Tagline *[noOfTaglines];
    first = curr = head.next;
    int i = 0;

    while (curr)
    {
      tagline[i++] = curr;
      curr = curr->next;
    }

    // sort
    if (tagsort) qsort(tagline, noOfTaglines, sizeof(Tagline *), tsortbytext);

    // filter
    listed = new int[noOfTaglines];
    for (i = 0; i < noOfTaglines; i++)
    {
      // initialize listed, so that oneSearch() can be used to check filter
      listed[i] = i;

      if (!filter || (oneSearch(i, filter) == FND_YES))
        listed[noOfListed++] = i;
    }
  }
}


void TaglineWindow::DestroyChain ()
{
  while (noOfTaglines) delete tagline[--noOfTaglines];
  delete[] tagline;
  tagline = NULL;
  first = NULL;

  noOfListed = 0;
  delete[] listed;
  listed = NULL;
}


int TaglineWindow::noOfItems ()
{
  return noOfListed;
}


void TaglineWindow::oneLine (int i)
{
  int l = topline + i;

  // may be displayed with faked noOfListed
  Tagline *curr = (fake && (l == noOfListed - 1) ? NULL : tagline[listed[l]]);

  sprintf(list->lineBuf, format, (curr ? curr->text : ""));
  list->PUTLINE(i, C_TAGBOXLIST);
}


searchtype TaglineWindow::oneSearch (int l, const char *what)
{
  return (strexcmp(tagline[listed[l]]->text, what) ? FND_YES : FND_NO);
}


void TaglineWindow::gotoTagline (const char *tag)
{
  int t, oldtop = topline;

  savePos();
  Move(HOME);

  if ((noOfListed == 0) && bm.resourceObject->isYes(ClearFilter)) Quit();

  for (t = 0; t < noOfListed; t++)
  {
    if (strcmp(tag, tagline[listed[t]]->text) != 0) Move(DOWN);
    else break;
  }

  if (t == noOfListed) restorePos();
  else if (t >= oldtop && t < oldtop + list_max_y) topline = oldtop;
}


void TaglineWindow::EnterTagline (const char *tag)
{
  int inp_y;
  Win *input;
  char newtag[TAGLINE_LENGTH + 1];
  Tagline *curr;

  savePos();

  if (isActive)
  {
    // edit
    if (tag)
    {
      inp_y = active - topline + 1;
    }
    // enter
    else
    {
      inp_y = noOfListed + 1;
      if (inp_y > list_max_y) inp_y = list_max_y;

      fake = true;
      noOfListed++;   // to get an input line after the last entry in list
      Move(END);
      Draw();
    }

    input = new Win(1, list_max_x, 5 + inp_y, (COLS - list_max_x) >> 1, C_TAGBOXLIST);
  }
  else
  {
    input = new Window(4, COLS - 2, 11, 1, C_TAGBOXBORDER, NULL, 0, 3, 2, 2);
    input->attrib(C_TAGBOXTTEXT);
    input->putstring(1, 1, "Enter new tagline:");
  }

  strncpy(newtag, (tag ? tag : ""), TAGLINE_LENGTH);
  newtag[TAGLINE_LENGTH] = '\0';

  // get input
  int inp = ((ShadowedWin *) input)->getstring((isActive ? 0 : 2),
                                               (isActive ? 0 : 1),
                                               newtag, TAGLINE_LENGTH,
                                               C_TAGBOXINP, C_TAGBOXLIST,
                                               bm.resourceObject->isYes(Pos1Input),
                                               NULL, NULL, !isActive);

  if (fake)
  {
    noOfListed--;   // correct it
    fake = false;
  }

  if (inp != GOT_ESC && *cropesp(newtag) == '\0')
  {
    if (isActive && tag) extrakeys('K');
    inp = GOT_ESC;
  }

  if ((inp != GOT_ESC) &&
      // if isActive, newtag must be different from original tag
      !(isActive && (strcmp(newtag, (tag ? tag : "")) == 0)))
  {
    // check dupe
    for (curr = first; curr; curr = curr->next)
    {
      if (strcmp(newtag, curr->text) == 0)
      {
        interface->ErrorWindow("Tagline already exists!", isActive);
        break;
      }
    }

    // no dupe
    if (!curr)
    {
      // change tagline
      if (isActive && tag)
      {
        writeTaglines(active, true, newtag);
      }
      // append tagline to file
      else
      {
        FILE *f;

        if ((f = fopen(bm.resourceObject->get(taglineFile), "at")))
        {
          fprintf(f, "%s\n", newtag);
          fclose(f);
        }
      }

      MakeChain();
    }

    gotoTagline(newtag);
  }
  // no input
  else restorePos();

  if (isActive)
  {
    delete input;
    interface->update();
  }
  else delete (Window *) input;
}


void TaglineWindow::writeTaglines (int num, bool keep, const char *text)
{
  FILE *f;

  if ((f = fopen(bm.resourceObject->get(taglineFile), "wt")))
  {
    // access and write in original (unsorted, unfiltered) order
    for (Tagline *curr = first; curr; curr = curr->next)
    {
      if (curr == tagline[listed[num]])
      {
        if (keep) fprintf(f, "%s\n", text);
      }
      else fprintf(f, "%s\n", curr->text);
    }
    fclose (f);
  }
}


void TaglineWindow::RandomTagline ()
{
  int r = 0;

  if (noOfListed >= 2)
  {
    // try to get a different tagline than the active one
    for (int j = 0; j < 1000; j++)
    {                              // the lower-order bits may be less
      r = rand() % noOfListed;     // random, but this should be good
                                   // enough for our purpose
      if (r != active) break;
    }

    active = r;
    Draw();
  }
}


void TaglineWindow::askFilter ()
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


void TaglineWindow::extrakeys (int key)
{
  switch (key)
  {
    case 'E':
      EnterTagline();
      break;

    case KEY_CTRL_E:
      if (noOfListed) EnterTagline(tagline[listed[active]]->text);
      break;

    case 'K':
    case KEY_DC:
      if (noOfListed)
      {
        if (interface->WarningWindow("Do you really want to delete this "
                                     "tagline?") == WW_YES)
        {
          writeTaglines(active, false);
          checkDel();
          MakeChain();
        }

        if ((noOfListed == 0) && bm.resourceObject->isYes(ClearFilter)) Quit();

        interface->update();
      }
      break;

    case 'O':
      tagsort = !tagsort;
      MakeChain();
      interface->update();
      break;

    case 'R':
      RandomTagline();
      break;

    case '|':
      askFilter();
      interface->update();
      break;

    case KEY_ENTER:
      if (noOfListed)
      {
        interface->letterwin.setLetterParam(tagline[listed[active]]->text);
        Quit();
      }
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


/*
   tagline qsort compare function
*/

int tsortbytext (const void *a, const void *b)
{
  const char *ta = (*(TaglineWindow::Tagline **) a)->text;
  const char *tb = (*(TaglineWindow::Tagline **) b)->text;

  int result = strcasecoll(ta, tb);

  return (result ? result : strcoll(ta, tb));
}
