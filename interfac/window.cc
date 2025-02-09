/*
 * blueMail offline mail reader
 * Window, ListWindow

 Copyright (c) 1996 Kolossvary Tamas <thomas@tvnet.hu>
 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2002 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include "interfac.h"


/*
   window
*/

Window::Window (int l, int c, int y, int x, chtype backg, const char *title,
                chtype titleAttrib, int bottx, int topx, int limit)
      : ShadowedWin(l, c, y, x, backg, title, titleAttrib, limit)
{
  lineBuf = new char[c + 1];
  // content
  window = new Win(l - bottx, c - 2, y + topx, x + 1, backg);
  // (default is bottx = 3 for 2 lines border and one line title, and
  // topx = 2 for 1 line border and one line title; c - 2 for left and
  // right border and x + 1 for left border)

}


Window::~Window ()
{
  delete window;
  delete[] lineBuf;
}


void Window::delay_update (bool complete)
{
  if (complete) ShadowedWin::delay_update();
  window->delay_update();
}


void Window::touch (bool complete)
{
  if (complete) ShadowedWin::touch();
  window->touch();
}


void Window::putline (int i, chtype ch)
{
  window->attrib(ch);
  window->putstring(i, 0, lineBuf);
}


void Window::Scroll (int i)
{
  window->wscroll(i);
}


/*
   list window
*/

ListWindow::ListWindow ()
{
  topline = active = 0;
  savePos();
  relist();
  smartScroll = false;
}


ListWindow::~ListWindow ()
{
}


void ListWindow::relist ()
{
  oldTop = oldActive = -2;
}


void ListWindow::checkPos (int limit)
{
  if (active >= limit) active = limit - 1;

  if (active < 0) active = 0;

  if (active < topline) topline = active;

  if (active >= topline + list_max_y) topline = active - list_max_y + 1;

  if (limit > list_max_y)
  {
    if (topline < 0) topline = 0;
    if (topline > limit - list_max_y) topline = limit - list_max_y;
  }
  else topline = 0;
}


void ListWindow::checkDel ()
{
  if (topline > 0 && topline + list_max_y >= noOfItems())
  {
    topline--;
    active--;
  }
}


void ListWindow::Draw ()
{
  int i, j, limit = noOfItems();

  checkPos(limit);

  if (limit > 0)
  {
    i = topline - oldTop;

    switch (i)
    {
      case -1:
      case 1:
        list->Scroll(i);
        // no break, must go through

      case 0:
        if (active != oldActive)
        {
          j = oldActive - topline;
          if (j >= 0 && j < list_max_y) oneLine(j);     // de-select old line
        }
        oneLine(active - topline);                      // select new line
        break;

      default:
        for (j = 0; j < list_max_y; j++)
        {
          oneLine(j);
          if (topline + j == limit - 1) j = list_max_y;
        }
    }

    list->delay_update(false);
  }

  oldTop = topline;
  oldActive = active;
}


void ListWindow::Touch ()
{
  list->touch();
}


void ListWindow::Move (direction dir)
{
  int limit = noOfItems();

  switch (dir)
  {
    case UP:
      active--;
      break;

    case DOWN:
      active++;
      break;

    case PGUP:
      topline -= list_max_y;
      active -= list_max_y;
      break;

    case PGDN:
      topline += list_max_y;
      active += list_max_y;
      break;

    case HOME:
      active = 0;
      break;

    case END:
      active = limit - 1;
      break;

    case UPP:
      active--;
      if (active < topline) topline -= list_max_y;
      break;

    case DWNP:
      active++;
      if (active >= topline + list_max_y) topline = active;
      break;
  }

  checkPos(limit);
}


void ListWindow::savePos ()
{
  savedTop = topline;
  savedActive = active;
}


void ListWindow::restorePos ()
{
  topline = savedTop;
  active = savedActive;
}


void ListWindow::setPos (int pos)
{
  active = pos;
}


searchtype ListWindow::search (const char *what)
{
  int limit = noOfItems();
  searchtype found = FND_NO;

  for (int i = active + 1; i < limit && found == FND_NO; i++)
  {
    if (list->keypressed())
    {
      found = FND_STOP;
      break;
    }

    found = oneSearch(i, what);

    if (found == FND_YES)
    {
      active = i;
      Draw();
    }
  }

  return found;
}


void ListWindow::KeyHandle (int key)
{
  bool draw = true;

  switch (key)
  {
    case KEY_DOWN:
      if (smartScroll) Move(DWNP);
      else Move(DOWN);
      break;

    case KEY_UP:
      if (smartScroll) Move(UPP);
      else Move(UP);
      break;

    case KEY_HOME:
      Move(HOME);
      break;

    case KEY_END:
    case KEY_LL:
      Move(END);
      break;

    case KEY_PPAGE:
      Move(PGUP);
      break;

    case ' ':
    case KEY_NPAGE:
      Move(PGDN);
      break;

    default:
      draw = false;
      extrakeys(key);
  }

  if (draw) Draw();
}
