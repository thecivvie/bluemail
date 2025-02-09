/*
 * blueMail offline mail reader
 * ANSI image/text viewer

 Copyright (c) 1999 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <ctype.h>
#include <string.h>
#include "interfac.h"
#include "../common/auxil.h"


/*
   ANSI line
*/

AnsiWindow::AnsiLine::AnsiLine (int space, AnsiLine *parent)
{
  prev = parent;
  next = NULL;

  if (space)
  {
    text = new chtype[space];
    for (int i = 0; i < (space - 1); i++) text[i] = ' ' | C_ANSIVIEW;
    text[space - 1] = '\0';
  }
  else text = NULL;

  length = 0;
}


AnsiWindow::AnsiLine::~AnsiLine ()
{
  delete[] text;
}


AnsiWindow::AnsiLine *AnsiWindow::AnsiLine::getprev () const
{
  return prev;
}


AnsiWindow::AnsiLine *AnsiWindow::AnsiLine::getnext (int space)
{
  if (!next)
    if (space) next = new AnsiLine(space, this);

  return next;
}


/*
   string stream
*/

void AnsiWindow::stringstream::init (FILE *file)
{
  this->file = file;
  string = NULL;

  rewind();
}


void AnsiWindow::stringstream::init (const unsigned char *string)
{
  this->string = string;
  file = NULL;

  rewind();
}


void AnsiWindow::stringstream::rewind ()
{
  if (file) fseek(file, 0, SEEK_SET);
  else pos = string;
}


unsigned char AnsiWindow::stringstream::fetc ()
{
  int result;

  if (file)
  {
    result = fgetc(file);
    if (result == EOF) result = 0;
  }
  else
  {
    result = *pos++;
    if (result == 0) pos--;
  }

  return result;
}


void AnsiWindow::stringstream::retc (unsigned char c)
{
  if (file) ungetc(c, file);
  else
  {
    if (pos > string) pos--;
  }
}


bool AnsiWindow::stringstream::isEnd () const
{
  if (file) return (feof(file) != 0);
  else return (*pos == '\0');
}


/*
   ANSI window
*/

AnsiWindow::AnsiWindow ()
{
  matchCol = NULL;
}


AnsiWindow::~AnsiWindow ()
{
  delete[] matchCol;
}


void AnsiWindow::set (const char *ansiSource, const char *what, bool latin1)
{
  source.init((const unsigned char *) ansiSource);
  description = what;
  this->latin1 = latin1;
  position = 0;
  savePos();
  setPos(-1, false);
}


void AnsiWindow::set (FILE *ansiSource, const char *what, bool latin1)
{
  source.init(ansiSource);
  description = what;
  this->latin1 = latin1;
  position = 0;
  savePos();
  setPos(-1, false);
}


void AnsiWindow::MakeActive ()
{
  maxlines = LINES - 1;

  header = new Win(1, COLS, 0, 0, C_ANSIVHEADER);
  text = new Win(maxlines, COLS, 1, 0, C_ANSIVIEW);

  anim = false;
  // see comment to the ANSI functions in WINCURS.CC
  interface->getUI()->ansi_init(C_ANSIVHEADER, C_ANSIVIEW, C_SEARCHRESULT);

  MakeChain();

  // see comment to the ANSI functions in WINCURS.CC
  if (interface->getUI()->ansi_remap())
  {
    for (int i = 0; i < noOfLines; i++)
      interface->getUI()->ansi_remap(linelist[i]->text, linelist[i]->length);
  }

  interface->shadowedWin(false);     // because we remap the color
                                     // used for shadow
  DrawHeader();
  DrawBody();
}


void AnsiWindow::Delete ()
{
  // see comment to the ANSI functions in WINCURS.CC
  interface->getUI()->ansi_done();

  interface->shadowedWin(true);

  DestroyChain();

  delete text;
  delete header;
}


void AnsiWindow::DrawHeader (const char *intro)
{
  char format[15], *workbuf = new char[COLS + 1];

  if (!intro) intro = "ANSI View";

  int col = COLS - strlen(intro) - 4;     // 4 = length of ": "
                                          //     plus space at beginning & end
  sprintf(format, "%%s: %%-%d.%ds", col, col);
  sprintf(workbuf, format, intro, description);

  header->putstring(0, 1, workbuf);
  header->delay_update();

  delete[] workbuf;
}


void AnsiWindow::DrawBody ()
{
  for (int i = 0; i < maxlines; i++) oneLine(i);
  text->delay_update();
}


void AnsiWindow::pos_reset ()
{
  cpx = cpy = lpy = spx = spy = 0;
}


void AnsiWindow::attrib_reset ()
{
  a_bold = a_blink = a_reverse = false;

  // default colors for ANSI code "reset color"
  a_fg = COLOR_WHITE;
  a_bg = COLOR_BLACK;

  // default color as long as ANSI code "reset color" isn't used
  attrib = C_ANSIVIEW;
}


void AnsiWindow::MakeChain ()
{
  unsigned char c;

  animBreak = false;

  if (!anim)
  {
    // initialize
    head = new AnsiLine();
    curr = head;
    curr = curr->getnext(COLS + 1);
    pos_reset();
    noOfLines = 1;
    baseline = 0;
  }

  attrib_reset();

  source.rewind();

  do
  {
    c = source.fetc();

    switch (c)
    {
      case 0:
        break;

      // hidden line (only in pos 0)
      case 1:
        if (cpx == 0)
          while (!source.isEnd() && (c != '\n')) c = source.fetc();
        break;

      // beep
      case 7:
        break;

      // backspace
      case 8:
        if (cpx) cpx--;
        break;

      // tab
      case 9:
        cpx = ((cpx / 8) + 1) * 8;
        while (cpx >= COLS)
        {
          cpx -= COLS;
          cpy++;
        }
        break;

      // line feed
      case 10:
        cpy++;
        // no break here!
      // carriage return
      case 13:
        cpx = 0;
        break;

      // form feed
      case 12:
        if (anim)
        {
          animtext->Clear(C_ANSIVIEW);
          pos_reset();
        }
        break;

// unprintable control codes
#ifndef ANSI_ALL_CHRS
      // double musical note
      case 14:
        output(19);
        break;

      // much like an asterisk
      case 15:
        output('*');
        break;

      // slash-o (except in CP 437)
      case 155:
        output('o');
        break;
#endif

      case 26: // EOF for DOS
        break;

      case 27: // ESC
      case '`':
        c = source.fetc();
        if (c == '[')
        {
          escparm[0] = '\0';
          esc();
        }
        else
        {
          source.retc(c);
          output('`');
        }
        break;

      default:
        output(c);
    }
  }
  while (c && !animBreak);

  // fill index array
  if (!anim)
  {
    linelist = new AnsiLine *[noOfLines];
    curr = head->getnext();
    int i = 0;

    while (curr)
    {
      linelist[i++] = curr;
      curr = curr->getnext();
    }

    delete head;
  }
}


void AnsiWindow::DestroyChain ()
{
  while (noOfLines) delete linelist[--noOfLines];
  delete[] linelist;
}


void AnsiWindow::oneLine (int i)
{
  int l = position + i;

  if (l < noOfLines)
  {
    text->putchnstr(i, 0, linelist[l]->text, COLS);

    // mark search results
    if (matchPos == l)
    {
      text->attrib(C_SEARCHRESULT);
      for (int j = 0; j < linelist[l]->length && j < COLS; j++)
        if (matchCol[j]) text->putchr(i, j, linelist[l]->text[j] & A_CHARTEXT);
      text->attrib(attrib);
    }
  }
  else text->clreol(i, 0);
}


void AnsiWindow::savePos ()
{
  savedPos = position;
}


void AnsiWindow::restorePos ()
{
  position = savedPos;
}


void AnsiWindow::setPos (int pos, bool realpos)
{
  if (realpos) position = pos;
  else
  {
    searchPos = pos;
    matchPos = -1;
    if (!matchCol) matchCol = new bool[COLS];
  }
}


searchtype AnsiWindow::search (const char *what)
{
  char *match;
  int j, dummy, whatlen = strlen(strex(dummy, what));
  searchtype found = FND_NO;

  char *currtext = new char[COLS + 1];

  for (int i = searchPos + 1; i < noOfLines && found == FND_NO; i++)
  {
    if (text->keypressed())
    {
      found = FND_STOP;
      break;
    }

    for (j = 0; j < linelist[i]->length && j < COLS; j++)
      currtext[j] = linelist[i]->text[j] & A_CHARTEXT;
    currtext[j] = '\0';

    match = strexcmp(currtext, what);
    found = (match ? FND_YES : FND_NO);

    if (found == FND_YES)
    {
      searchPos = matchPos = i;

      if (i < position || i >= position + maxlines)
      {
        // match is out of current screen
        position = i;

        // show as much lines as possible from new position
        while (position + maxlines > noOfLines)
        {
          if (position) position--;
          else break;
        }
      }

      for (j = 0; j < COLS; j++) matchCol[j] = false;

      // mark all matches in line for oneLine()
      while (match)
      {
        j = match - currtext;
        for (int k = 0; k < whatlen; k++) matchCol[j + k] = true;
        match = strexcmp(++match, what);
      }

      DrawBody();
    }
  }

  delete[] currtext;

  return found;
}


int AnsiWindow::getparm ()
{
  char *parm;
  int value;

  if (escparm[0])
  {
    // find paramter separator
    for (parm = escparm; *parm && (*parm != ';'); parm++) ;

    if (*parm == ';')
    {
      // more parameters after
      *parm++ = '\0';

      if (parm == escparm + 1) value = 1;     // empty parameter
      else value = atoi(escparm);

      strcpy(escparm, parm);
    }
    else
    {
      // last parameter
      value = atoi(escparm);
      escparm[0] = '\0';
    }
  }
  // empty (last) parameter
  else value = 1;

  return value;
}


void AnsiWindow::check_pos ()
{
  if (cpy > lpy)
  {
    // move down and allocate space
    while (lpy != cpy)
    {
      curr = curr->getnext(COLS + 1);
      lpy++;
    }
  }
  else if (cpy < lpy)
  {
    // move up
    while (lpy != cpy)
    {
      curr = curr->getprev();
      lpy--;
    }
  }

  if (noOfLines < cpy + 1 + baseline) noOfLines = cpy + 1 + baseline;
}


void AnsiWindow::attrib_set ()
{
  int parm;
  static const chtype ansicolor_table[8] = {COLOR_BLACK, COLOR_RED,
                                            COLOR_GREEN, COLOR_YELLOW,
                                            COLOR_BLUE, COLOR_MAGENTA,
                                            COLOR_CYAN, COLOR_WHITE};

  while (escparm[0])
  {
    parm = getparm();

    switch (parm)
    {
      // reset colors
      case 0:
        attrib_reset();
        break;

      // bright
      case 1:
        a_bold = true;
        break;

      // flashing
      case 5:
        a_blink = true;
        break;

      // reverse
      case 7:
        a_reverse = true;
        break;

      default:
        // foreground color
        if (parm >= 30 && parm <= 37) a_fg = ansicolor_table[parm - 30];
        // background color
        else if (parm >= 40 && parm <= 47) a_bg = ansicolor_table[parm - 40];
    }
  }

  attrib = COL(a_fg, a_bg);     // blueMail internal color

  if (a_bold) attrib |= A_BOLD;
  if (a_blink) attrib |= A_BLINK;
  if (a_reverse) attrib |= A_REVERSE;

  // convert to UserInterface color
  attrib = interface->getUI()->ansi_color(attrib, !anim);
}


void AnsiWindow::esc ()
{
  int x, y;
  bool weird;

  unsigned char c = source.fetc();

  switch (c)
  {
    // cursor up
    case 'A':
      cpy -= getparm();
      if (cpy < 0) cpy = 0;
      break;

    // cursor down
    case 'B':
      cpy += getparm();
      if (anim && (cpy > LINES - 2)) cpy = LINES - 2;
      break;

    // cursor right
    case 'C':
      cpx += getparm();
      if (cpx > COLS - 1) cpx = COLS - 1;
      break;

    // cursor left
    case 'D':
      cpx -= getparm();
      if (cpx < 0) cpx = 0;
      break;

    // set cursor position
    case 'H':
    case 'f':
      cpy = getparm() - 1;
      cpx = getparm() - 1;
      if (cpx < 0) cpx = 0;
      if (cpx > COLS - 1) cpx = COLS - 1;
      if (cpy < 0) cpy = 0;
      break;

    // clear screen
    case 'J':
      if (getparm() == 2)
      {
        if (anim) animtext->Clear(C_ANSIVIEW);
        else
        {
          cpy = noOfLines - baseline - 1;
          check_pos();
          if (baseline < noOfLines - 1) baseline = noOfLines - 1;
        }

        pos_reset();
      }
      break;

    // erase end of line
    case 'K':
      x = cpx;
      y = cpy;
      for (int i = cpx; i < COLS; i++) output(' ');
      cpx = x;
      cpy = y;
      break;

    // set attributes (colors etc.)
    case 'm':
      attrib_set();
      break;

    // save cursor position
    case 's':
      spx = cpx;
      spy = cpy;
      break;

    // restore cursor position
    case 'u':
      cpx = spx;
      cpy = spy;
      break;

    default:
      // some weird mutant codes
      weird = (c == '=' || c == '?');

      if (is_digit(c) || (c == ';') || weird)
      {
        if ((unsigned int) (x = strlen(escparm)) < sizeof(escparm) - 1)
        {
          escparm[x++] = (weird ? ' ' : c);
          escparm[x] = '\0';
        }
        esc();
      }
  }
}


void AnsiWindow::output (unsigned char c)
{
  if (!animBreak)
  {
    chtype cout = interface->charconv_in(latin1, c) | attrib;

    if (anim)
    {
      int limit = LINES - 2;
      int lines = cpy - limit;     // lines beyond limit

      if (lines > 0)
      {
        animtext->wscroll(lines);
        for (int i = lines; i; i--) animtext->clreol(limit - i + 1, 0);
        cpy = limit;
      }

      animtext->putch(cpy, cpx++, cout);
      animtext->update();
      animBreak = animtext->keypressed();
    }
    else
    {
      check_pos();
      curr->text[cpx++] = cout;
      if (cpx > curr->length) curr->length = cpx;
    }

    if (cpx == COLS)
    {
      cpx = 0;
      cpy++;
    }
  }
}


void AnsiWindow::animate ()
{
  animtext = new Win(LINES - 1, COLS, 1, 0, C_ANSIVIEW);

  // slow drawing down
  animtext->cursor_on(0);

  pos_reset();
  attrib_reset();

  anim = true;
  DrawHeader("Animating");

  MakeChain();

  anim = false;
  animtext->cursor_off();

  if (!animBreak)
  {
    DrawHeader("     Done");
    header->update();
  }

  // don't leave animation mode automatically
  if (!animBreak) (void) animtext->inkey();

  delete animtext;
  text->touch();
  DrawHeader();
}


void AnsiWindow::KeyHandle (int key)
{
  bool maxscroll = bm.resourceObject->isYes(LetterMaxScroll);

  switch (key)
  {
    case KEY_UP:
      if (position > 0)
      {
        position--;
        text->wscroll(-1);
        oneLine(0);
        text->delay_update();
      }
      break;

    case KEY_DOWN:
      if (position + (maxscroll ? 1 : maxlines) < noOfLines)
      {
        position++;
        text->wscroll(1);
        oneLine(maxlines - 1);
        text->delay_update();
      }
      break;

    case KEY_PPAGE:
      position -= (position > maxlines ? maxlines : position);
      DrawBody();
      break;

    case ' ':
    case KEY_NPAGE:
      if (position + maxlines < noOfLines)
      {
        position += maxlines;
        if (!maxscroll && (position + maxlines > noOfLines))
          position = noOfLines - maxlines;
        DrawBody();
      }
      break;

    case KEY_HOME:
      position = 0;
      DrawBody();
      break;

    case KEY_END:
    case KEY_LL:
      if (noOfLines > maxlines)
      {
        position = noOfLines - maxlines;
        DrawBody();
      }
      break;

    case 'C':
      interface->charsetToggle();
      break;

    case 'V':
      animate();
      break;
  }
}
