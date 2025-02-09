/*
 * blueMail offline mail reader
 * line and letter handling (message display)

 Copyright (c) 1996 Kolossvary Tamas <thomas@tvnet.hu>
 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <ctype.h>
#include <fcntl.h>
#include <string.h>
#include "interfac.h"
#include "../common/auxil.h"
#include "../common/error.h"
#include "../common/mysystem.h"


// I need 2, too. Hopefully, nobody will notice or complain.
#define KLUDGE(c) (c == '\1' || c == '\2')


LetterWindow::Line::Line (int size)
{
  text = (size ? new char[size] : NULL);
  reply = true;
  qpos = -1;
  next = NULL;
}


LetterWindow::Line::~Line ()
{
  delete[] text;
}


LetterWindow::LetterWindow ()
{
  message = NULL;
  linelist = NULL;
  noOfLines = 0;
  in_chain.area = in_chain.letter = -1;
  *tagline = '\0';
  LetterParam.to = LetterParam.subject = LetterParam.tagline = NULL;
  showkludge = bm.resourceObject->isYes(DisplayKludgelines);
  rot13 = false;
  maxscroll = bm.resourceObject->isYes(LetterMaxScroll);
  matchCol = NULL;
  ReturnInfo.valid = false;
}


LetterWindow::~LetterWindow ()
{
  delete[] matchCol;
}


void LetterWindow::MakeActive (bool anew)
{
  maxlines = LINES - 6;

  header = new Win(5, COLS, 0, 0, C_LHTEXT);
  text = new Win(maxlines, COLS, 5, 0, C_LTEXT);
  statbar = new Win(1, COLS, LINES - 1, 0, C_LBOTTLINE);

  workbuf = new char[COLS + 1];

  if (ReturnInfo.valid && anew && !bm.areaList->isReplyArea())
  {
    position = ReturnInfo.position;
    savePos();
    setPos(-1, false);
    anew = false;
  }

  Draw(anew ? firstTime : againActPos);
}


void LetterWindow::Delete ()
{
  delete[] workbuf;
  delete statbar;
  delete text;
  delete header;
}


void LetterWindow::Touch ()
{
  header->touch();
  text->touch();
  statbar->cursor_on();     // to get it out of the way
  statbar->touch();
  statbar->cursor_off();
}


void LetterWindow::Reset ()
{
  DestroyChain();

  delete[] LetterParam.to;
  delete[] LetterParam.subject;
  delete[] LetterParam.tagline;
  LetterParam.to = NULL;
  LetterParam.subject = NULL;
  LetterParam.tagline = NULL;
}


int LetterWindow::areaOfMsg ()
{
  return (bm.areaList->isReplyArea() ? bm.areaList->getAreaNo()
                                     : bm.letterList->getAreaID());
}


void LetterWindow::Draw (drawtype t)
{
  if (t != againActPos)
  {
    position = 0;
    savePos();
    setPos(-1, false);
  }

  if (t == firstTime)
  {
    *tagline = '\0';
    hideReadFlag = !bm.letterList->isRead();
    if (hideReadFlag)
    {
      bm.letterList->setRead();
      interface->setAnyRead();
    }
    if (bm.letterList->isPersonal() && bm.resourceObject->isYes(BeepOnPersonalMail))
      interface->delay_beep();
  }

  if (in_chain.area != areaOfMsg() ||
      in_chain.letter != bm.letterList->getCurrent()) message = NULL;
  MakeChain(COLS);

  DrawHeader();
  DrawBody();
  DrawStatbar();
}


void LetterWindow::Linecounter ()
{
  if (!bm.resourceObject->isYes(SuppressLineCounter))
  {
    header->attrib(C_LHTEXT);
    header->putstring(0, COLS - 34, " Line: ");
    header->attrib(C_LHMSGNUM);
    sprintf(workbuf, "%4d/%d ", (noOfLines ? position + 1 : 0), noOfLines);
    header->putstring(0, COLS - 27, workbuf);
  }

  header->delay_update();
}


#ifdef WITH_CLOCK
void LetterWindow::Clock ()
{
  char mode = toupper(*(bm.resourceObject->get(ClockMode)));
  if (mode == 'O') return;

  char clock[8];
  time_t now = time(NULL);

  if (now - lasttime > 59)
  {
    if (mode == 'T')
      strftime(clock, sizeof(clock), " %H:%M ", localtime(&now));
    else
    {
      long elapsed = (now - starttime) / 60;
      sprintf(clock, " %02ld:%02ld ", (elapsed / 60) % 100, elapsed % 60);
    }

    header->attrib(C_LHCLOCK);
    header->putstring(0, COLS - 9, clock);
    header->delay_update();

    int now_sec = now % 60;
    int start_sec = starttime % 60;

    lasttime = now - (now_sec) +
               (mode == 'E' ? start_sec - (now_sec < start_sec ? 60 : 0) : 0);
  }
}
#endif


const char *LetterWindow::From ()
{
  if (bm.areaList->isNetmail(bm.letterList->getAreaID()))
    return bm.letterList->getFrom();
  else return interface->letters.From();
}


void LetterWindow::addNetAddr (char *name)
{
  char format[12];
  int maxlen = COLS - 41;
  int nlen = strlen(name);
  int alen = maxlen - nlen;

  bool isInternet = bm.areaList->isInternet(bm.letterList->getAreaID());
  char *addr = bm.letterList->getNetAddr().get(isInternet);

  if (addr)
  {
    // in case of empty "To:" (see [*]) name (= to) is a copy of addr
    // (we never want to see an addr identical to name)
    if (strcmp(name, addr) == 0) return;

    if (alen > 2)
    {
      sprintf(format, (isInternet ? "%%s%s%%.%ds" : "%%s%s%%.%ds"),
                      (nlen ? (isInternet ? " <" : ", ") : ""),
                      (nlen ? alen - 2 : alen));
      sprintf(name, format, name, addr);

      if (isInternet && nlen && ((int) strlen(name) < maxlen)) strcat(name, ">");
    }
  }
}


void LetterWindow::DrawHeader ()
{
  char format[7];
  int stat = bm.letterList->getStatus();

  header->Clear(C_LHTEXT);

  header->attrib(C_LHBORDER);
  header->Border();

  header->attrib(C_LHTEXT);
  header->putstring(0, 2, " Msg:");
  header->putstring(1, 2, "From:");
  header->putstring(2, 2, "  To:");
  header->putstring(3, 2, "Subj:");

  header->putstring(1, COLS - 33, "Date:");
  header->putstring(2, COLS - 33, "Stat:");

  header->attrib(bm.letterList->isPrivate() ? C_LHFLAGSHI : C_LHFLAGS);
  header->putstring(2, COLS - 27, "Pvt");

  if (!bm.areaList->isReplyArea())
  {
    header->attrib(!hideReadFlag && bm.letterList->isRead() ? C_LHFLAGSHI
                                                            : C_LHFLAGS);
    header->putstring(2, COLS - 23, "Read");
    header->attrib(stat & MSF_REPLIED ? C_LHFLAGSHI : C_LHFLAGS);
    header->putstring(2, COLS - 18, "Replied");
    header->attrib(stat & MSF_MARKED ? C_LHFLAGSHI : C_LHFLAGS);
    header->putstring(2, COLS - 10, "Marked");
  }

  char *flags = bm.letterList->Flags();
  if (*flags)
  {
    header->attrib(C_LHFLAGSHI);
    sprintf(workbuf, " %.25s ", flags);
    header->putstring(4, COLS - 28, workbuf);
  }

  header->attrib(C_LHMSGNUM);
  char *p = workbuf;
  p += sprintf(workbuf, " %d of %d ", bm.letterList->getCurrent() + 1,
                                      bm.areaList->getNoOfLetters());
  if (!bm.areaList->isReplyArea())
    sprintf(p, "(#%d) ", bm.letterList->getMsgNum());
  header->putstring(0, 7, workbuf);

  sprintf(format, "%%.%ds", COLS - 41);

  header->attrib(C_LHFROM);
  sprintf(workbuf, format, From());
  interface->charconv_in(workbuf, bm.letterList->isLatin1());

  if (!bm.areaList->isReplyArea() &&
      bm.areaList->isNetmail(bm.letterList->getAreaID())) addNetAddr(workbuf);

  header->putstring(1, 8, workbuf);

  header->attrib(C_LHTO);
  sprintf(workbuf, format, bm.letterList->getTo());
  interface->charconv_in(workbuf, bm.letterList->isLatin1());

  if (bm.areaList->isReplyArea()) addNetAddr(workbuf);

  header->putstring(2, 8, workbuf);

  header->attrib(C_LHSUBJ);
  sprintf(format, "%%.%ds", COLS - 9);
  sprintf(workbuf, format, bm.letterList->getSubject());
  interface->charconv_in(workbuf, bm.letterList->isLatin1());
  header->putstring(3, 8, workbuf);

  header->attrib(C_LHDATE);
  strcpy(workbuf, bm.letterList->getDate());
  interface->charconv_in(workbuf, bm.letterList->isLatin1());
  header->putstring(1, COLS - 27, workbuf);

  Linecounter();
#ifdef WITH_CLOCK
  lasttime = 0;
  Clock();
#endif
}


void LetterWindow::DrawBody ()
{
  for (int i = 0; i < maxlines; i++) oneLine(i);
  text->delay_update();

  Linecounter();
}


void LetterWindow::DrawStatbar ()
{
  const char *helpmsg = " F1 or ? - Help ", *in;
  char format[40];
  int maxw = COLS - 17;
  bool latin1;

  if (bm.areaList->isReplyArea() || bm.areaList->isCollection())
  {
    maxw -= strlen(bm.areaList->getShortName()) + 5;     // name plus " in: "
    sprintf(format, " %s in: %%-%d.%ds%%s", bm.areaList->getShortName(),
                                            maxw, maxw);
    in = bm.letterList->getReplyIn();
    latin1 = bm.letterList->isLatin1();
  }
  else
  {
    sprintf(format, " %%-%d.%ds%%s", maxw, maxw);
    in = NULL;
    latin1 = bm.areaList->isLatin1();
  }

  sprintf(workbuf, format,
          in && *in ? in
                      // letterList's AreaID is the correct title for both,
                      // normal and collection areas
                    : bm.areaList->getTitle(bm.letterList->getAreaID()),
          helpmsg);
  interface->charconv_in(workbuf, latin1);

  statbar->cursor_on();
  statbar->putstring(0, 0, workbuf);
  statbar->delay_update();
  statbar->cursor_off();
}


int LetterWindow::getLineQuotePos (const char *line)
{
  int qpos = -1;

  // classic style
  for (int i = 0; line[i] && (i < 5); i++)
  {
    if (line[i] == '<') break;     // no quote, HTML(-like)
    if (line[i] == '>')
    {
      if (i && (line[i - 1] == '-') &&                       // no quote,
          bm.resourceObject->isYes(ArrowNoQuote)) break;     // but arrow

      qpos = i + 1;
      break;
    }
  }

  // alternative style
  if (strlen(line) >= 2 && (*line == ':' || *line == '|') && line[1] == ' ')
    qpos = 0;

  return qpos;
}


chtype LetterWindow::getLineColour (const char *line, bool &signature,
                                                      bool &reply, char &qpos)
{
  int i;
  chtype attrib;

  // inside a signature
  if (signature)
  {
    attrib = C_LSIGNATURE;
    reply = false;
  }
  else
  {
    // normal text
    attrib = C_LTEXT;

    // quoted line
    if ((i = getLineQuotePos(line)) != -1)
    {
      attrib = C_LQTEXT;
      qpos = i;
    }

    // tagline
    if (strncmp(line, "... ", 4) == 0)
    {
      attrib = C_LTAGLINE;
      // steal it
      const char *t = &line[4];
      char *r = tagline;
      for (i = 0; *t && (*t != '\n') && (i < TAGLINE_LENGTH); t++, r++, i++)
        *r = *t;
      *r = '\0';
    }
  }

  // tearline
  if (strcmp(line, "---") == 0 ||
      strncmp(line, "--- ", 4) == 0 ||
      strncmp(line, "~~~ ", 4) == 0)
  {
    signature = false;
    attrib = C_LTEARLINE;
    reply = false;
  }
  // origin
  else if (strncmp(line, " * Origin: ", 11) == 0)
  {
    signature = false;
    attrib = C_LORIGIN;
    reply = false;
  }
  // signature
  else if (strcmp(line, "-- ") == 0)
  {
    signature = true;
    attrib = C_LSIGNATURE;
    reply = false;
  }
  // kludge line
  else if KLUDGE(*line)
  {
    signature = false;
    attrib = C_LKLUDGE;
  }

  return attrib;
}


void LetterWindow::MakeChain (int columns)
{
  Line head(0), *curr;
  char *c, *k, *lastword, chr;
  int length, tabc;
  bool end = false, splitit, splitted = false, signature = false,
       reformatted = false, overlong;
  chtype split_attrib = 0;
  bool split_reply = false;

  DestroyChain();
  in_chain.area = areaOfMsg();
  in_chain.letter = bm.letterList->getCurrent();
  curr = &head;

  if (!message) message = (char *) bm.letterList->getBody();

  c = message;
  while (!end)
  {
    if (!showkludge)
    {
      while KLUDGE(*c)
        while (*c && (*c++ != '\n')) ;

      if (*c == '\0') break;
    }

    curr->next = new Line(columns + 1);
    curr = curr->next;

    noOfLines++;

    lastword = NULL;
    k = curr->text;
    length = 0;
    tabc = 0;

    while (!end && (length < columns))
    {
      if ((chr = *c) == '\n')
      {
        char cdummy;
        bool brk = true, paragraph = (c[1] == '\n'), bdummy = false;

        char *nl = strchr(c + 1, '\n');
        // getLineColour() needs a null-terminated line (string)
        if (nl) *nl = '\0';

        if (splitted && !paragraph && (split_attrib == C_LTEXT) &&
            (getLineColour(c + 1, bdummy, bdummy, cdummy) == C_LTEXT))
        {
          // get rid of trailing blanks
          while (length > 0 && k[-1] == ' ')
          {
            k--;
            length--;
          }

          // skip blanks at beginning of next line
          while (c[1] && (c[1] == ' ')) c++;

          chr = ' ';
          reformatted = true;
          splitted = false;
          brk = false;
        }

        if (nl) *nl = '\n';
        if (brk) break;
      }

      if (tabc)
      {
        tabc--;
        chr = ' ';
      }
      else if (chr == '\t')
      {
        tabc = 7 - (length % 8);
        chr = ' ';
      }

      if (chr == ' ') lastword = NULL;
      else if (lastword == NULL) lastword = c;

      if (chr)
      {
        *k++ = chr;
        if (tabc == 0) c++;
        length++;
      }
      else end = true;
    }

    overlong = (length == columns);
    splitit = (overlong && (*c != '\n') && !end);

    // back to last word (if possible)
    if (splitit && lastword && (c - lastword < columns))
    {
      length -= c - lastword;
      c = lastword;
    }

    // get rid of trailing blanks
    if (overlong)
      while (length > 0 && curr->text[length - 1] == ' ') length--;

    // skip blanks at beginning of next line
    while (splitit && *c && (*c == ' ')) c++;

    if (*c == '\n') c++;

    curr->text[length] = '\0';
    curr->length = length;

    // handle splitted part(s) of lines
    if (splitted || reformatted)
    {
      curr->attrib = split_attrib;
      curr->reply = split_reply;
      reformatted = false;
      goto NEXTLINE;
    }

    curr->attrib = getLineColour(curr->text, signature, curr->reply, curr->qpos);

    split_attrib = curr->attrib;
    split_reply = curr->reply;

NEXTLINE:
    splitted = splitit;
  }

  // fill index array
  linelist = new Line *[noOfLines];
  curr = head.next;
  int i = 0;
  while (curr)
  {
    linelist[i] = curr;

    // character set conversion
    interface->charconv_in(linelist[i]->text, bm.letterList->isLatin1());

    // rot13 decryption
    if (rot13)
      for (unsigned int j = 0; j < linelist[i]->length; j++)
      {
        unsigned char c = linelist[i]->text[j];
        if (isalpha(c))
        {
          if (tolower(c) < 'n') c += 13;
          else c -= 13;
          linelist[i]->text[j] = c;
        }
      }

    i++;
    curr = curr->next;
  }

  // get rid of blank lines
  if (!showkludge)
    while (noOfLines && (linelist[noOfLines - 1]->length == 0))
      delete linelist[--noOfLines];
}


void LetterWindow::DestroyChain ()
{
  if (linelist)
  {
    while (noOfLines) delete linelist[--noOfLines];
    delete[] linelist;
    linelist = NULL;
  }

  in_chain.area = in_chain.letter = -1;
}


void LetterWindow::oneLine (int i)
{
  int l = position + i;
  chtype attrib;
  char *currtext;
  unsigned int length;

  if (l < noOfLines)
  {
    attrib = linelist[l]->attrib;
    currtext = linelist[l]->text;
    length = linelist[l]->length;
    text->attrib(attrib);

    if KLUDGE(*currtext)
    {
      currtext++;
      length--;
    }

    text->putstring(i, 0, currtext);

    // mark search results
    if (matchPos == l)
    {
      text->attrib(C_SEARCHRESULT);
      for (int j = 0; j < (int) length && j < COLS; j++)
        if (matchCol[j]) text->putchr(i, j, currtext[j]);
      text->attrib(attrib);
    }
  }
  else length = 0;

  text->clreol(i, length);
}


void LetterWindow::savePos ()
{
  savedPos = position;
}


void LetterWindow::restorePos ()
{
  position = savedPos;
}


void LetterWindow::setPos (int pos, bool realpos)
{
  if (realpos) position = pos;
  else
  {
    searchPos = pos;
    matchPos = -1;
    if (!matchCol) matchCol = new bool[COLS];
  }
}


searchtype LetterWindow::search (const char *what)
{
  char *match, *currtext;
  int dummy, whatlen = strlen(strex(dummy, what));
  searchtype found = FND_NO;

  for (int i = searchPos + 1; i < noOfLines && found == FND_NO; i++)
  {
    if (text->keypressed())
    {
      found = FND_STOP;
      break;
    }

    currtext = linelist[i]->text;
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

      for (int j = 0; j < COLS; j++) matchCol[j] = false;

      // mark all matches in line for oneLine()
      while (match)
      {
        int j = match - currtext - (KLUDGE(*currtext) ? 1 : 0);
        for (int k = 0; k < whatlen; k++) matchCol[j + k] = true;
        match = strexcmp(++match, what);
      }

      Draw(againActPos);
    }
  }

  return found;
}


void LetterWindow::Next ()
{
  if (bm.letterList->getActive() < bm.letterList->getNoOfActive() - 1)
  {
    interface->letters.Move(DOWN);
    bm.letterList->gotoActive(bm.letterList->getActive() + 1);
    Draw(firstTime);
  }
  else
  {
    bool regular = !ReturnInfo.valid;     // *must* be set *first*
    interface->back();
    interface->letters.Quit();
    interface->back();
    if (regular) interface->areas.KeyHandle(KEY_RIGHT);
  }
}


void LetterWindow::Previous ()
{
  if (bm.letterList->getActive() > 0)
  {
    interface->letters.Move(UP);
    bm.letterList->gotoActive(bm.letterList->getActive() - 1);
    Draw(firstTime);
  }
  else
  {
    bool regular = !ReturnInfo.valid;     // *must* be set *first*
    interface->back();
    interface->letters.Quit();
    interface->back();
    if (regular) interface->areas.KeyHandle(KEY_LEFT);
  }
}


void LetterWindow::Move (int dirkey)
{
  switch (dirkey)
  {
    case KEY_UP:
      if (position > 0)
      {
        position--;
        text->wscroll(-1);
        oneLine(0);
        text->delay_update();
        Linecounter();
      }
      break;

    case KEY_DOWN:
      if (position + (maxscroll ? 1 : maxlines) < noOfLines)
      {
        position++;
        text->wscroll(1);
        oneLine(maxlines - 1);
        text->delay_update();
        Linecounter();
      }
      break;

    case KEY_PPAGE:
      position -= (position > maxlines ? maxlines : position);
      DrawBody();
      break;

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

    case KEY_RIGHT:
      Next();
      break;

    case KEY_LEFT:
      Previous();
      break;

    case ' ':
      if (position + maxlines < noOfLines) Move(KEY_NPAGE);
      else Next();
      break;

    case KEY_TAB:
      const char *subj = strdupplus(stripRE(bm.letterList->getSubject()));
      do Next();
      while (interface->getstate() == letter &&
             strcasecmp(stripRE(bm.letterList->getSubject()), subj) == 0);
      delete[] subj;
      break;
  }
}


void LetterWindow::checkLetterFilter ()
{
  // see comment in LetterListWindow::MakeActive() why noOfActive may be 0
  if (bm.letterList->getNoOfActive() == 0)
  {
    if (interface->getstate() == letter) interface->back();

    if ((bm.areaList->getNoOfLetters() == 0) ||
        !bm.resourceObject->isYes(ClearFilter))
    {
      interface->letters.Quit();
      interface->back();
    }
  }
}


void LetterWindow::jumpReplies (int fromArea)
{
  // save return information
  ReturnInfo.fromArea = fromArea;
  interface->letters.getSortFilterOptions(ReturnInfo.netmail_sort,
                                          ReturnInfo.letter_sort,
                                          ReturnInfo.filter,
                                          ReturnInfo.filterLast,
                                          ReturnInfo.filterHeader);
  ReturnInfo.fromLetter = bm.letterList->getCurrent();
  ReturnInfo.position = position;
  ReturnInfo.SkipLetterList = strdupplus(bm.resourceObject->get(SkipLetterList));
  bm.resourceObject->set(SkipLetterList, "N");

  interface->back();
  interface->letters.Quit();
  interface->back();
  interface->areas.Move(HOME);   // REPLY_AREA is first and never filtered out
  interface->Select();

  ReturnInfo.valid = true;       // *must* be set *after* interface->back()
}


void LetterWindow::returnReplies (bool regular)
{
  if (ReturnInfo.valid)
  {
    if (regular)
    {
      bm.areaList->gotoArea(ReturnInfo.fromArea);
      interface->areas.ResetActive();
      interface->Select();

      interface->letters.setSortFilterOptions(ReturnInfo.netmail_sort,
                                              ReturnInfo.letter_sort,
                                              ReturnInfo.filter,
                                              ReturnInfo.filterLast,
                                              ReturnInfo.filterHeader);
      interface->letters.Delete();
      interface->letters.MakeActive();

      bm.letterList->gotoLetter(ReturnInfo.fromLetter);
      interface->letters.ResetActive();
      interface->Select();
      interface->InfoWindow("Returning from replies");
    }

    // clean up
    ReturnInfo.valid = false;
    delete[] ReturnInfo.filter;
    bm.resourceObject->set(SkipLetterList, ReturnInfo.SkipLetterList);
    delete[] ReturnInfo.SkipLetterList;
  }
}


void LetterWindow::KeyHandle (int key)
{
  int r, a;
  const char *to;
  net_address na;

  switch (key)   // common to reply and normal areas
  {
#ifdef WITH_CLOCK
    case ERR:
      Clock();
      break;

#endif
    case '!':
      if (interface->areas.makeReply()) Touch();
      break;

    case 'A':
      interface->changestate(addressbook);
      break;

    case 'C':
      interface->charsetToggle();
      break;

    case 'P':
    case 'S':
      if ((key == 'S') || okPrint())
      {
        Save(SAVE_THIS, (key == 'P'));
        Touch();
      }
      break;

    case 'V':
      interface->ansiView(message,
                          bm.letterList->getSubject(),
                          bm.letterList->isLatin1());
      break;

    case 'X':
      showkludge = !showkludge;
      Draw(againPos1);
      break;

    case '#':
      rot13 = !rot13;
      Draw(againActPos);
      break;

    case '=':
      bm.driverList->toggleQuotedPrintable();
      message = NULL;
      Draw(againActPos);
      break;

#ifdef WITH_CLOCK
    case ':':
      r = toupper(*(bm.resourceObject->get(ClockMode)));
      if (r != 'O')
      {
        bm.resourceObject->set(ClockMode, r == 'T' ? "E" : "T");
        Draw(againActPos);
      }
      break;
#endif

    default:
      if (bm.areaList->isReplyArea())
      {
        switch(key)
        {
          case 'D':
            if ((r = interface->selectArea(AREAS_ALL)) != -1)
            {
              setLetterParam(r, IS_EDIT | IS_DIFFAREA);
              setLetterParam(bm.letterList->getTo(),
                             bm.letterList->isLatin1(),
                             bm.letterList->getNetAddr());
              EditLetter();
              checkLetterFilter();
            }
            break;

          case 'E':
            if ((r = bm.letterList->getAreaID()))
            {
              setLetterParam(r, IS_EDIT);
              setLetterParam(bm.letterList->getTo(),
                             bm.letterList->isLatin1(),
                             bm.letterList->getNetAddr());
              EditLetter();
              checkLetterFilter();
            }
            else interface->InfoWindow("Cannot edit this letter");
            break;

          case KEY_CTRL_E:
            if (EditBody())
            {
              message = NULL;
              Draw(againPos1);
            }
            break;

          case 'J':
            if (ReturnInfo.valid)
            {
              interface->back();
              interface->letters.Quit();
              interface->back();
            }
            break;

          case 'K':
          case KEY_DC:
            if (interface->WarningWindow("Are you sure you want to delete "
                                         "this letter?") == WW_YES)
            {
              bool last = (bm.letterList->getActive() + 1 >=
                           bm.letterList->getNoOfActive());

              bm.areaList->killLetter(bm.letterList->getMsgNum());
              interface->setUnsaved();
              DestroyChain();
              interface->letters.MakeChain();

              if (bm.letterList->getNoOfActive() == 0) checkLetterFilter();
              else if (interface->getstate() == letter)
              {
                if (last) interface->letters.Move(UP);
                interface->letters.Select();
              }
            }
            interface->update();
            break;

          default:
            Move(key);
        }
      }
      else   // normal area only
      {
        switch (key)
        {
          case 'J':
            if (!bm.resourceObject->isYes(LongLetterList))
              interface->ErrorWindow("Option 'LongLetterList' must be set "
                                     "for this.");
            else if (bm.driverList->canReply())
            {
              a = bm.areaList->getAreaNo();
              bm.areaList->gotoArea(0);     // REPLY_AREA has area number 0
              r = bm.areaList->getNoOfLetters();
              bm.areaList->gotoArea(a);     // restore

              if (r > 0) jumpReplies(a);
              else interface->InfoWindow("There are no replies.");
            }
            break;

          case 'M':
            if (bm.areaList->supportedMSF() & MSF_MARKED)
            {
              bm.letterList->setStatus(bm.letterList->getStatus() ^ MSF_MARKED);
              interface->setAnyRead();
              DrawHeader();
            }
            break;

          case 'U':
            if (bm.areaList->supportedMSF() & MSF_READ)
            {
              if (hideReadFlag)
                interface->InfoWindow("Message now marked as unread", 1);
              else interface->setAnyRead();

              bm.letterList->setStatus(bm.letterList->getStatus() ^ MSF_READ);
              hideReadFlag = false;
              DrawHeader();
            }
            break;

          case 'T':
            interface->changestate(taglinebook);
            break;

          case KEY_CTRL_T:
            interface->taglines.EnterTagline(interface->charconv_in(bm.letterList->isLatin1(),
                                                                    tagline));
            Touch();
            break;

          case 'R':
          case 'O':
          case 'D':
            a = bm.letterList->getAreaID();
            r = (key == 'D' ? interface->selectArea(AREAS_ALL) : a);

            if (r != -1)
            {
              setLetterParam(r, IS_REPLY | (key == 'D' && r != a ? IS_DIFFAREA
                                                                 : 0));

              if ((key == 'R') || (key == 'D') || bm.areaList->isNetmail(r))
                to = bm.letterList->getFrom();
              else to = bm.letterList->getTo();

              if (bm.areaList->isNetmail(r)) na = bm.letterList->getNetAddr();

              setLetterParam(to, bm.letterList->isLatin1(), na);
              EnterLetter();
            }
            break;

          case 'E':
          case 'F':
            if ((r = interface->selectArea(AREAS_ALL)) != -1)
            {
              setLetterParam(r, (key == 'E' ? IS_NEW : IS_FORWARD));
              if (bm.areaList->isNetmail(r)) interface->changestate(addressbook);
              EnterLetter();
            }
            break;

          case 'N':
            if (bm.areaList->findNetmail(&a))
            {
              if (a == -1) a = interface->selectArea(AREAS_NETMAIL);

              if (a != -1)
              {
                setLetterParam(a, IS_REPLY);
                setLetterParam(bm.letterList->getFrom(),
                               bm.letterList->isLatin1(),
                               bm.letterList->getNetAddr());
                EnterLetter();
              }
            }
            else interface->ErrorWindow("Netmail is not supported");
            break;

          default:
            Move(key);
        }
      }
  }
}


/*
   letter editing
*/

void LetterWindow::setLetterParam (int reply_area, int type)
{
  LetterParam.reply_area = reply_area;
  LetterParam.type = type;
}


void LetterWindow::setLetterParam (const char *to, bool to_latin1,
                                   net_address &na,
                                   const char *subj, bool subj_latin1)
{
  delete[] LetterParam.to;
  LetterParam.to = strdupplus(to);   // needed if isReply
  LetterParam.to_latin1 = to_latin1;

  LetterParam.na = na;               // needed in netmail area

  delete[] LetterParam.subject;
  LetterParam.subject = strdupplus(subj);
  LetterParam.subj_latin1 = subj_latin1;
}


void LetterWindow::setLetterParam (const char *tag)
{
  delete[] LetterParam.tagline;
  LetterParam.tagline = strdupplus(tag);
}


bool LetterWindow::EnterHeader (char *from, char *to, char *subj,
                                bool f_latin1, bool t_latin1, bool s_latin1,
                                net_address *na, int *flags, bool regular)
{
  ShadowedWin *replybox, *flagbox = NULL;
  int nof = 0;
  // number of available message status flags
  // 0: none
  // 1: private flag
  // 6: FidoNet style flags
  int items = 3, ysubj = 3;
  // items = 3, ysubj = 3: from, to/in, subject
  // items = 4, ysubj = 3: from, to, subject, flags
  // items = 4, ysubj = 4: from, to, address, subject
  // items = 5, ysubj = 4: from, to, address, subject, flags
  char *netAddr = NULL;
  HOTKEY h[6];
  HOTKEYS hotkeys;
  static struct
  {
    int keycode;
    const char *description;
    int flagvalue;
  } keydata[] = {{KEY_CTRL_C, CRASH_TXT, CRASH},
                 {KEY_CTRL_D, DIRECT_TXT, DIRECT},
                 {KEY_CTRL_F, FILEATTACHED_TXT, FILEATTACHED},
                 {KEY_CTRL_K, KILL_TXT, KILL},
                 {KEY_CTRL_H, HOLD_TXT, HOLD},
                 {KEY_CTRL_R, FILEREQUEST_TXT, FILEREQUEST}};
  int current, result, slen;
  bool end = false;
  char *string;

  int maxFromToLen = bm.areaList->getMaxFromToLen();
  int maxSubjLen = bm.areaList->getMaxSubjLen();
  int maxAddrLen = (bm.areaList->isInternet() ? OTHERADDRLEN : FIDOADDRLEN);

  if (na)
  {
    string = na->get(bm.areaList->isInternet());
    netAddr = new char[maxAddrLen + 1];
    strncpy(netAddr, (string ? string : ""), maxAddrLen);
    netAddr[maxAddrLen] = '\0';
    items++;   // plus address
    ysubj++;
    if (!bm.areaList->isInternet())
    {
      nof = 6;
      items++;   // plus flags
    }
  }
  else if (bm.areaList->allowPrivate())
  {
    nof = 1;
    items++;   // plus flags
  }

  replybox = new ShadowedWin(items + 2, COLS - 2, (LINES >> 1) - 5, 1,
                             C_REPBOXBORDER, NULL, 0, 2);

  interface->charconv_in(from, f_latin1);
  interface->charconv_in(to, t_latin1);
  interface->charconv_in(subj, s_latin1);

  replybox->attrib(C_REPBOXDESCR);
  replybox->putstring(1, 2, "From:");
  replybox->putstring(2, 2, regular ? "  To:" : "  In:");
  replybox->attrib(C_REPBOXTEXT);
  replybox->putstring(1, 8, from, maxFromToLen);
  replybox->putstring(2, 8, to, maxFromToLen);

  if (ysubj == 4)
  {
    replybox->attrib(C_REPBOXDESCR);
    replybox->putstring(3, 2, "Addr:");
    replybox->attrib(C_REPBOXTEXT);
    replybox->putstring(3, 8, netAddr, maxAddrLen);
  }

  replybox->attrib(C_REPBOXDESCR);
  replybox->putstring(ysubj, 2, "Subj:");
  replybox->attrib(C_REPBOXTEXT);
  replybox->putstring(ysubj, 8, subj, maxSubjLen);

  replybox->delay_update();

  if (nof)
  {
    replybox->attrib(C_REPBOXDESCR);
    replybox->putstring(items, 2, "Stat:");

    flagbox = new ShadowedWin(nof + 2, 27, (LINES >> 1) + items - 3, 6,
                              C_REPBOXHELP, "Message Status Flags",
                              C_REPBOXBORDER);
    if (nof == 1)
    {
      h[0].keycode = KEY_CTRL_P;
      h[0].description = "Pvt";
      h[0].selected = ((*flags & PRIVATE) != 0);
      h[0].xorindex = -1;

      hotkeys.maxkey = 1;

      flagbox->putstring(1, 2, "Ctrl-P: Private");
    }
    else
    {
      for (int i = 0; i < 6; i++)
      {
        h[i].keycode = keydata[i].keycode;
        h[i].description = keydata[i].description;
        h[i].selected = ((*flags & keydata[i].flagvalue) != 0);
        h[i].xorindex = -1;
      }

      h[2].xorindex = 5;     // either "file attached"
      h[5].xorindex = 2;     // or "file request"

      hotkeys.maxkey = 6;

      flagbox->putstring(1, 2, "Ctrl-C: Crash");
      flagbox->putstring(2, 2, "Ctrl-D: Direct");
      flagbox->putstring(3, 2, "Ctrl-F: File Attached");
      flagbox->putstring(4, 2, "Ctrl-K: Kill when Sent");
      flagbox->putstring(5, 2, "Ctrl-H: Hold for Pickup");
      flagbox->putstring(6, 2, "Ctrl-R: Request a File");
    }

    hotkeys.ypos = items;
    hotkeys.xpos = 8;
    hotkeys.hotkey = h;

    flagbox->delay_update();
  }

  if (!*to) current = 1;
  else if ((ysubj == 4) && !*netAddr) current = 2;
  else current = ysubj - 1;

  do
  {
    replybox->attrib(C_REPBOXBORDER);

    if (current >= 1)
      replybox->putstring(items + 1, COLS - 23, " ? for addressbook ");
    else replybox->Border();

    slen = maxFromToLen;

    if (current == ysubj - 1)
    {
      string = subj;
      slen = maxSubjLen;
    }
    else if (current == 2 && ysubj == 4)
    {
      string = netAddr;
      slen = maxAddrLen;
    }
    else if (current == 1) string = to;
    else string = from;

    char *last_string = strdupplus(string);

    result = replybox->getstring(current + 1, 8, string, slen,
                                 C_REPBOXINP, C_REPBOXTEXT,
                                 bm.resourceObject->isYes(Pos1Input),
                                 (nof ? &hotkeys : NULL), flagbox);

    // check for empty input
    if (result != GOT_ESC && *string == '\0')
    {
      // [*] allow empty "To:" if there is an address item and it isInternet
      if (current == 1 && ysubj == 4 && bm.areaList->isInternet()) /* ok */;
      else result = GOT_NOTHING;
    }

    // check for addressbook call
    if (result != GOT_ESC && current >= 1 && strcmp(string, "?") == 0)
    {
      interface->changestate(addressbook);
      *string = '\0';

      if (LetterParam.to)
      {
        replybox->attrib(C_REPBOXTEXT);

        strncpy(to, LetterParam.to, maxFromToLen);
        to[maxFromToLen] = '\0';
        replybox->clreol(2, 8);
        replybox->putstring(2, 8, to, maxFromToLen);

        if (ysubj == 4)
        {
          string = LetterParam.na.get(bm.areaList->isInternet());
          strncpy(netAddr, (string ? string : ""), maxAddrLen);
          netAddr[maxAddrLen] = '\0';
          replybox->clreol(3, 8);
          replybox->putstring(3, 8, netAddr, maxAddrLen);
          current = 2;   // to force netmail address checking (below)
        }

        if (LetterParam.subject && (subj == NULL || *subj == '\0'))
        {
          strncpy(subj, LetterParam.subject, maxSubjLen);
          subj[maxSubjLen] = '\0';
        }
        replybox->clreol(ysubj, 8);
        replybox->putstring(ysubj, 8, subj, maxSubjLen);

        result = GOT_DOWN;
      }
      else
      {
        strcpy(string, last_string);
        result = GOT_NOTHING;
      }

      replybox->touch();
      if (flagbox) flagbox->touch();
    }

    delete[] last_string;

    // check netmail address
    if (result != GOT_ESC && current == 2 && ysubj == 4)
    {
      net_address addr;

      addr.set(string);
      bool isInternet = bm.areaList->isInternet();
      string = addr.get(isInternet);
      if (!string || (isInternet && !isEmailAddress(string, strlen(string))))
      {
        interface->ErrorWindow("Invalid address", false);
        replybox->touch();
        if (flagbox) flagbox->touch();
        result = GOT_NOTHING;
      }
      else
      {
        strcpy(netAddr, string);
        replybox->attrib(C_REPBOXTEXT);
        replybox->clreol(current + 1, 8);
        replybox->putstring(current + 1, 8, netAddr, maxAddrLen);
        replybox->delay_update();
      }

    }

    switch (result)
    {
      case GOT_ESC:
        end = true;
        break;

      case GOT_ENTER:
        current++;
        if (current == ysubj) end = true;
        break;

      case GOT_UP:
        if (current > 0) current--;
        break;

      case GOT_DOWN:
        if (current < ysubj - 1) current++;
        break;
    }

  }
  while (!end);

  if (result)
  {
    if (na) na->set(netAddr);

    *flags = 0;

    if (nof == 1)
    {
      if (h[0].selected) *flags = PRIVATE;
    }
    else if (nof == 6)
    {
      for (int i = 0; i < 6; i++)
        if (h[i].selected) *flags |= keydata[i].flagvalue;
    }
  }

  delete flagbox;
  delete replybox;
  delete[] netAddr;

  interface->charconv_out(from, bm.areaList->isLatin1());
  interface->charconv_out(to, bm.areaList->isLatin1());
  interface->charconv_out(subj, bm.areaList->isLatin1());

  return (result != GOT_ESC);
}


bool LetterWindow::okArea ()
{
  if (!bm.driverList->canReply())
  {
    interface->areas.Select();   // return to current (active) area
    interface->ErrorWindow("Replies are not supported");
    return false;
  }

  if (bm.areaList->isReadonly())
  {
    interface->areas.Select();   // return to current (active) area
    interface->InfoWindow("Sorry, this is a read-only area.");
    return false;
  }

  if (bm.driverList->useEmail() && !bm.resourceObject->get(EmailAddress))
  {
    interface->areas.Select();   // return to current (active) area
    interface->ErrorWindow("Configure your e-mail address first");
    return false;
  }

  return true;
}


int LetterWindow::QuotedTextPercentage (const char *reply_filename)
{
  FILE *file;
  char buffer[MYMAXLINE];
  size_t blen, qlen = 0, len = 0;

  if ((file = fopen(reply_filename, "rt")))
  {
    while ((blen = fgetsnl(buffer, sizeof(buffer), file)))
    {
      if (getLineQuotePos(buffer) != -1) qlen += blen;
      len += blen;
    }
    fclose(file);
  }

  return (len == 0 ? 0 : qlen * 100 / len);
}


void LetterWindow::EnterLetter ()
{
  const char *recipient;
  bool f_latin1, t_latin1, s_latin1;
  int relen = 0, privat = 0, flags = 0, replyto, qtp;
  FILE *reply;

  bm.areaList->gotoArea(LetterParam.reply_area);

  if (!okArea()) return;

  int maxFromToLen = bm.areaList->getMaxFromToLen();
  int maxSubjLen = bm.areaList->getMaxSubjLen();

  char *from = new char[maxFromToLen + 1];
  char *to = new char[maxFromToLen + 1];
  char *subj = new char[maxSubjLen + 1];

  // header

  bool useAlias = bm.areaList->useAlias();

  if (useAlias)
  {
    strncpy(from, bm.resourceObject->get(AliasName), maxFromToLen);
    from[maxFromToLen] = '\0';

    if (!*from) useAlias = false;
  }

  f_latin1 = (bm.resourceObject->isYes(hasLoginName) ? bm.isLatin1()
                                                     : interface->isLatin1());

  if (!useAlias)
  {
    strncpy(from,
            (bm.driverList->useEmail() ? bm.resourceObject->get(EmailAddress)
                                       : bm.resourceObject->get(LoginName)),
            maxFromToLen);
    from[maxFromToLen] = '\0';
    if (bm.driverList->useEmail()) f_latin1 = interface->isLatin1();
  }

  if (bm.driverList->allowCrossPost() && !bm.areaList->hasTo())
  {
    bool isReply = (LetterParam.type == IS_REPLY);

    recipient = (isReply ? bm.letterList->getReplyIn()
                         : bm.areaList->getTitle());

    if (isReply && (strcasecmp(recipient, "poster") == 0))
    {
      interface->InfoWindow("You should reply via e-mail.");
      recipient = "";
    }

    t_latin1 = (isReply ? bm.letterList->isLatin1() : bm.areaList->isLatin1());
  }
  else
  {
    recipient = LetterParam.to;
    t_latin1 = LetterParam.to_latin1;
  }

  if (!recipient)
  {
    if (bm.areaList->isNetmail()) recipient = "";
    else
    {
      recipient = bm.resourceObject->get(ToAll);
      t_latin1 = interface->isLatin1();
    }
  }

  strncpy(to, recipient, maxFromToLen);
  to[maxFromToLen] = '\0';

  strncpy(subj, (LetterParam.subject ? LetterParam.subject : ""), maxSubjLen);
  subj[maxSubjLen] = '\0';
  s_latin1 = LetterParam.subj_latin1;

  if (LetterParam.type & (IS_REPLY | IS_FORWARD))
  {
    int subjlen = strlen(bm.letterList->getSubject());

    if ((LetterParam.type & IS_REPLY) &&
        !bm.resourceObject->isYes(OmitReplyRe) &&
        (strncasecmp(bm.letterList->getSubject(), "RE: ", 4) != 0) &&
        (subjlen + 4 <= maxSubjLen))
    {
      strcpy(subj, "Re: ");
      relen = 4;
    }

    strncat(subj, bm.letterList->getSubject(), maxSubjLen - relen);
    s_latin1 = bm.letterList->isLatin1();
  }

  if (bm.areaList->isNetmail()) privat = PRIVATE;
  else if ((LetterParam.type & IS_REPLY) && bm.areaList->allowPrivate() &&
           bm.letterList->isPrivate()) flags = PRIVATE;

  if ((LetterParam.type & IS_REPLY) && bm.letterList->getReplyAddr())
    LetterParam.na.set(bm.letterList->getReplyAddr());

  net_address *na = (bm.areaList->isNetmail() ? &LetterParam.na : NULL);

  if (EnterHeader(from, to, subj, f_latin1, t_latin1, s_latin1, na, &flags,
                  bm.areaList->hasTo() || !bm.driverList->allowCrossPost()))
  {
    replyto = (LetterParam.type & IS_REPLY ? bm.letterList->getMsgNum() : 0);

    // body

    char reply_filename[MYMAXPATH];
    mkrepfname(reply_filename);

    // return to current (active) area
    interface->areas.Select();

    if (LetterParam.type & IS_REPLY) QuoteText(reply_filename);
    else if (LetterParam.type & IS_FORWARD) ForwardText(reply_filename);

    // reply file creation date
    time_t creatime = fmtime(reply_filename);

    int qom = atoi(bm.resourceObject->get(QuoteOMeter));

    int r = WW_ESC;
    while (r == WW_ESC)
    {
      // edit the reply
      Editor(reply_filename);

      if (fmtime(reply_filename) == creatime)
      {
        r = interface->WarningWindow("Letter was not edited! Cancel?");
        interface->update();
        if (r != WW_NO) continue;
      }

      if ((qtp = QuotedTextPercentage(reply_filename)) > qom)
      {
        char msg[53];
        sprintf(msg, "%d%% of your reply consists of quoted text! Re-edit?", qtp);
        r = interface->WarningWindow(msg);
        interface->update();
        if (r == WW_YES)
        {
          r = WW_ESC;
          continue;
        }
      }

      r = WW_NO;
    }

    if (r == WW_YES) remove(reply_filename);
    else   // continue with reply
    {
      if (LetterParam.type & IS_REPLY)
      {
        int stat = bm.letterList->getStatus();
        if ((stat & MSF_REPLIED) == 0)
        {
          bm.letterList->setStatus(stat | MSF_REPLIED);
          interface->setAnyRead();
        }
      }

      if ((reply = fopen(reply_filename, "at")))
      {
        FILE *sigfile = NULL;
        const char *sigfname = bm.resourceObject->get(signatureFile);
        int r_a = LetterParam.reply_area;

        // signature
        if (bm.areaList->isInternet(r_a) &&
            (sigfname) && ((sigfile = fopen(sigfname, "rt"))))
        {
          fputs("\n-- \n", reply);
          while ((r = fgetc(sigfile)) != EOF) fputc(r, reply);
        }

        // tagline
        if ((bm.areaList->isInternet(r_a) && !sigfile) ||
            (!bm.areaList->isInternet(r_a) && !bm.areaList->isNetmail(r_a)))
        {
          if (!bm.resourceObject->isYes(SkipTaglineBox))
            interface->changestate(taglinebook);

          if (LetterParam.tagline)
            fprintf(reply,
                    (bm.areaList->isInternet(r_a) && !sigfile ? "\n-- \n%s\n"
                                                              : "\n... %s\n"),
                    LetterParam.tagline);
        }

        // tearline
        if (bm.driverList->useTearline() && !bm.areaList->isNetmail(r_a) &&
            !bm.areaList->isInternet(r_a))
        {
          if (!LetterParam.tagline) fputc('\n', reply);
          fprintf(reply, "--- %s\n", bm.version());
        }

        if (sigfile) fclose(sigfile);
        fclose(reply);
      }

      // reconvert the text
      long replylen = fileconv(reply_filename, F_OUT);

      bm.areaList->enterLetter(LetterParam.reply_area, from, to, subj,
                               replyto, flags | privat, na, to,
                               (LetterParam.type == IS_NEW ? NULL
                                                           : bm.letterList->getReplyID()),
                               reply_filename, replylen);
      interface->setUnsaved();
    }
  }
  else
  {
    // return to current (active) area
    interface->areas.Select();

    // may be called from arelist or letterlist
    if (interface->getstate() == letter) Touch();
  }

  delete[] subj;
  delete[] to;
  delete[] from;

  // clear LetterParam
  delete[] LetterParam.to;
  delete[] LetterParam.subject;
  delete[] LetterParam.tagline;
  LetterParam.to = NULL;
  LetterParam.na.set(NULL);
  LetterParam.subject = NULL;
  LetterParam.tagline = NULL;
}


void LetterWindow::mkrepfname (char *fname)
{
  int f = 0;
  char rname[13];

  const char *number = bm.areaList->getNumber();

  for (int i = 1; i < 1000; i++)
  {
    sprintf(rname, "%.8s.%03d", (*number ? number : "reply"), i);
    mkfname(fname, bm.resourceObject->get(WorkDir), rname);
    if ((f = open(fname, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR)) != -1)
    {
      close(f);
      return;
    }
  }

  fatalError("Out of reply filenames.");
}


void LetterWindow::QuoteText (const char *reply_filename)
{
  FILE *reply;
  #define MAXQUOTE 255
  char qstring[MAXQUOTE + 1], *qpos;
  const char *content;

  bool isInternet = bm.areaList->isInternet(LetterParam.reply_area);
  const char *qstr = bm.resourceObject->get(isInternet ? QuoteHeaderInternet
                                                       : QuoteHeaderFido);
  char *from = (char *) From();
  char *to = (char *) bm.letterList->getTo();
  qpos = qstring;

  if ((reply = fopen(reply_filename, "wt")))
  {
    if (LetterParam.type & IS_DIFFAREA)
      fprintf(reply, " * Reply to message originally in %s\n\n",
              interface->charconv_in(bm.areaList->isLatin1(),
                                     bm.areaList->getTitle(bm.letterList->getAreaID())));

    while (*qstr)
    {
      if (*qstr == '@')
      {
        switch (*++qstr)
        {
          case 'N':
            content = "\n";
            break;

          case ' ':
            content = " ";
            break;

          case 'f':
            content = strtokl(from, ' ');
            break;

          case 'F':
            content = strtokr(from, ' ');
            break;

          case 'O':
            content = from;
            break;

          case 'A':
            content = bm.letterList->getNetAddr().get(bm.areaList->isInternet(bm.letterList->getAreaID()));
            if (!content) content = "";
            break;

          case 't':
            content = strtokl(to, ' ');
            break;

          case 'T':
            content = strtokr(to, ' ');
            break;

          case 'R':
            content = to;
            break;

          case 'S':
            content = bm.letterList->getSubject();
            break;

          case 'D':
            content = bm.letterList->getDate();
            break;

          default:
            content = "";
            qpos = charcpy(qstring, qpos, MAXQUOTE, '@');
            if (*qstr != '@')
              qpos = charcpy(qstring, qpos, MAXQUOTE,
                             interface->charconv_out(*qstr,
                                                     bm.letterList->isLatin1()));
        }
        while (*content) qpos = charcpy(qstring, qpos, MAXQUOTE, *content++);
        if (*qstr) qstr++;
      }
      else qpos = charcpy(qstring, qpos, MAXQUOTE,
                          interface->charconv_out(*qstr++,
                                                  bm.letterList->isLatin1()));
    }
    *qpos = '\0';

    interface->charconv_in(qstring, bm.letterList->isLatin1());
    fprintf(reply, "%s\n", qstring);
    WriteText(reply, true, isInternet);

    fclose(reply);
  }
}


void LetterWindow::ForwardText (const char *reply_filename)
{
  FILE *reply;
  static const char *suffix[4] = {"ly on", "ly by", "ly to", " subj"};
  const char *item[4] = {bm.letterList->getDate(), From(),
                         bm.letterList->getTo(), bm.letterList->getSubject()};
  char *out;
  bool isInternet = bm.areaList->isInternet();

  if ((reply = fopen(reply_filename, "wt")))
  {
    fprintf(reply, " * Forward from area %s\n\n",
            interface->charconv_in(bm.areaList->isLatin1(),
                                   bm.areaList->getTitle(bm.letterList->getAreaID())));

    for (int i = 0; i < 4; i++)
    {
      out = interface->charconv_in(bm.letterList->isLatin1(), item[i]);
      fprintf(reply, " * Original%s: %s", suffix[i], out);

      if (i == 1 && bm.areaList->isNetmail())
      {
        char *addr = bm.letterList->getNetAddr().get(isInternet);
        if (addr) fprintf(reply, (isInternet ? " <%s>" : ", %s"), addr);
      }

      fputc('\n', reply);
    }

    fputc('\n', reply);
    WriteText(reply, false, isInternet);
    fputs("\n * End of Forward\n", reply);

    fclose(reply);
  }
}


void LetterWindow::WriteText (FILE *reply_file, bool quote, bool isInternet)
{
  char qfrom[4];

  char *from = (char *) bm.letterList->getFrom();
  char *froml = strtokl(from, ' ');
  char *fromr = strtokr(from, ' ');

  qfrom[0] = ' ';
  qfrom[1] = *froml;
  qfrom[2] = (*fromr || !*froml ? *fromr : *++froml);
  qfrom[3] = '\0';

  interface->charconv_in(qfrom, bm.letterList->isLatin1());

  char overlong = toupper(*(bm.resourceObject->get(OverlongReplyLines)));
  bool qempty = !bm.resourceObject->isYes(OmitEmptyQuotes);

  int outcols = (quote ? 73 : 78);
  MakeChain(outcols);

  for (int i = 0; i < noOfLines; i++)
  {
    // neutralize kludge lines
    if (*linelist[i]->text == '\1') *linelist[i]->text = '@';

    if (quote)
    {
      if (linelist[i]->reply)
      {
        if (linelist[i]->attrib == C_LQTEXT)
        {
          int qpos = linelist[i]->qpos;

          if (qpos != -1)
          {
            for (int j = 0; j < qpos; j++) putc(linelist[i]->text[j],
                                                reply_file);
            fputc('>', reply_file);
          }
          else
          {
            qpos = 0;

            if (overlong == 'P')
            {
              // determine length of written '\n' character
              long fpos = ftell(reply_file);
              fputc('\n', reply_file);
              fpos = ftell(reply_file) - fpos;

              // eat up both '\n' (the one for testing and the last eol)
              fseek(reply_file, -(fpos << 1), SEEK_END);
              // prepare last line for joining with this part
              fputc(' ', reply_file);
            }
            else if (overlong == 'Q') fprintf(reply_file, " >> ");
          }

          fprintf(reply_file, "%s\n", linelist[i]->text + qpos);
        }
        else fprintf(reply_file, (*linelist[i]->text || qempty ? "%s> %s\n"
                                                               : "\n"),
                                 (isInternet ? "" : qfrom), linelist[i]->text);
      }
    }
    else
    {
      // neutralize tearline, origin and signature
      if (!linelist[i]->reply &&
          (linelist[i]->text[1] == '-' || linelist[i]->text[1] == '*'))
        linelist[i]->text[1] = '!';

      fprintf(reply_file, "%s\n", linelist[i]->text);
    }
  }

  MakeChain(COLS);
}


void LetterWindow::Editor (const char *reply_filename)
{
  mysystem(makecmd(bm.resourceObject->get(editorCommand), canonize(reply_filename)));
}


long LetterWindow::fileconv (const char *reply_filename, fconvtype type)
{
  char *tempfile;
  FILE *reply, *temp;
  size_t rlen = 0, len;

  if (type == F_IN) interface->InfoWindow("Preparing letter...", 0);
  else interface->InfoWindow("Saving letter...", 0);

  tempfile = mymktmpfile(bm.resourceObject->get(WorkDir));
  time_t replyMtime = fmtime(reply_filename);

  if ((reply = fopen(reply_filename, "rt")))
  {
    if ((temp = fopen(tempfile, "wt")))
    {
      char *body = new char[BLOCKLEN + 1];

      while ((len = fread(body, sizeof(char), BLOCKLEN, reply)))
      {
        rlen += len;
        body[len] = '\0';

        switch (type)
        {
          case F_IN:
            interface->charconv_in(body, bm.letterList->isLatin1());
            break;

          case F_OUT:
            interface->charconv_out(body, bm.areaList->isLatin1(LetterParam.reply_area));
            break;

          case F_NONE:
            break;
        }

        fwrite(body, sizeof(char), len, temp);
      }

      delete[] body;
      fclose(temp);
      fclose(reply);

      remove(reply_filename);
      rename(tempfile, reply_filename);

      // preserve reply's mtime
      fmtime(reply_filename, replyMtime);
    }
    else fclose(reply);
  }

  return (long) rlen;
}


void LetterWindow::EditLetter ()     // basically the same as EnterLetter()
{
  const char *To;
  bool f_latin1, t_latin1, s_latin1;
  int flags, privat = 0;
  char *body = message;
  FILE *reply;

  statetype state = interface->getstate();

  bm.areaList->gotoArea(LetterParam.reply_area);

  if (!okArea()) return;

  int maxFromToLen = bm.areaList->getMaxFromToLen();
  int maxSubjLen = bm.areaList->getMaxSubjLen();

  char *from = new char[maxFromToLen + 1];
  char *to = new char[maxFromToLen + 1];
  char *subj = new char[maxSubjLen + 1];

  // header

  strncpy(from, bm.letterList->getFrom(), maxFromToLen);
  from[maxFromToLen] = '\0';
  f_latin1 = bm.letterList->isLatin1();

  t_latin1 = bm.letterList->isLatin1();

  if (bm.driverList->allowCrossPost() && !bm.areaList->hasTo())
  {
    To = (LetterParam.type == IS_EDIT ? bm.letterList->getReplyIn()
                                      : bm.areaList->getTitle());
    if (LetterParam.type != IS_EDIT) t_latin1 = bm.areaList->isLatin1();
  }
  else To = LetterParam.to;

  strncpy(to, To, maxFromToLen);
  to[maxFromToLen] = '\0';

  strncpy(subj, bm.letterList->getSubject(), maxSubjLen);
  subj[maxSubjLen] = '\0';
  s_latin1 = bm.letterList->isLatin1();

  flags = bm.letterList->getFlags();
  if (bm.areaList->isNetmail()) privat = PRIVATE;

  net_address *na = (bm.areaList->isNetmail() ? &LetterParam.na : NULL);

  // in case of empty "To:" (see [*]) to is a copy of addr
  if (na && (strcmp(to, na->get(bm.areaList->isInternet())) == 0)) *to = '\0';

  if (EnterHeader(from, to, subj, f_latin1, t_latin1, s_latin1, na, &flags,
                  bm.areaList->hasTo() || !bm.driverList->allowCrossPost()))
  {
    // body

    if (state != letter) body = (char *) bm.letterList->getBody();

    char reply_filename[MYMAXPATH];
    mkrepfname(reply_filename);

    // return to reply area
    interface->areas.Select();

    if ((reply = fopen(reply_filename, "wt")))
    {
      // only edit if IS_EDIT from letterlist (not in letter)
      bool editit = (LetterParam.type == IS_EDIT && state != letter);

      if (editit) interface->charconv_in(body, bm.letterList->isLatin1());

      size_t msglen = strlen(body);
      fwrite(body, msglen, 1, reply);
      if (msglen > 0 && body[msglen - 1] != '\n') fputc('\n', reply);
      fclose(reply);

      // edit the reply
      if (editit) Editor(reply_filename);

      // reconvert the text
      long replylen = fileconv(reply_filename, editit ? F_OUT : F_NONE);

      int replyto = bm.letterList->getReplyTo();
      char *replyID = strdupplus(bm.letterList->getReplyID());

      bm.areaList->killLetter(bm.letterList->getMsgNum());
      bm.areaList->enterLetter(LetterParam.reply_area, from, to, subj,
                               replyto, flags | privat, na, to, replyID,
                               reply_filename, replylen);
      interface->setUnsaved();

      delete[] replyID;

      DestroyChain();
      interface->letters.MakeChain();
      interface->letters.Move(END);
      interface->letters.Select();
      // may be called from letterlist
      if (state == letter) Draw(againPos1);
    }
  }
  else
  {
    // return to reply area
    interface->areas.Select();

    // may be called from letterlist
    if (state == letter) Touch();
  }

  delete[] subj;
  delete[] to;
  delete[] from;

  // clear LetterParam
  delete[] LetterParam.to;
  delete[] LetterParam.subject;
  delete[] LetterParam.tagline;
  LetterParam.to = NULL;
  LetterParam.na.set(NULL);
  LetterParam.subject = NULL;
  LetterParam.tagline = NULL;
}


bool LetterWindow::EditBody ()
{
  const char *filename =
    bm.areaList->newLetterBody(bm.letterList->getMsgNum(), -1);
  time_t creatime = fmtime(filename);

  fileconv(filename, F_IN);
  Editor(filename);

  LetterParam.reply_area = bm.letterList->getAreaID();     // for fileconv()
  long replylen = fileconv(filename, F_OUT);

  if (fmtime(filename) == creatime) return false;
  else
  {
    bm.areaList->newLetterBody(bm.letterList->getMsgNum(), replylen);
    interface->setUnsaved();
    return true;
  }
}


bool LetterWindow::okPrint ()
{
  if (bm.resourceObject->get(printCommand)) return true;
  else
  {
    interface->ErrorWindow("Configure your print command first");
    return false;
  }
}


bool LetterWindow::Save (int stype, bool prt)
{
  int inp;
  FILE *file;
  static char savefile[QUERYINPUT + 1];
  char fname[MYMAXPATH], *sname = NULL;
  bool written = false;

  if (prt)
  {
    mkfname(fname, bm.resourceObject->get(WorkDir), BM_NAME ".prt");
    sname = fname;
    inp = GOT_ENTER;
  }
  else inp = interface->QueryBox("Append to file:", savefile, QUERYINPUT, false);

  if (inp != GOT_ESC)
  {
    if (!prt)
    {
      if (*savefile == '/'
#ifdef DOSPATHTYPE
                   || *savefile == '\\' || savefile[1] == ':'
#endif
                                                           ) sname = savefile;
      else
      {
        mkfname(fname, bm.resourceObject->get(SaveDir), savefile);
        sname = fname;
      }
    }

    if ((file = fopen(sname, "at")))
    {
      switch (stype)
      {
        case SAVE_LISTED:
          for (int i = 0; i < bm.letterList->getNoOfActive(); i++)
          {
            bm.letterList->gotoActive(i);
            message = NULL;
            SaveText(file, prt);
            written = true;
          }
          break;

        case SAVE_MARKED:
        case SAVE_PERSONAL:
          for (int i = 0; i < bm.areaList->getNoOfLetters(); i++)
          {
            bm.letterList->gotoLetter(i);
            if (((stype == SAVE_MARKED) && (bm.letterList->getStatus() & MSF_MARKED)) ||
                ((stype == SAVE_PERSONAL) && bm.letterList->isPersonal()))
            {
              message = NULL;
              SaveText(file, prt);
              written = true;
            }
          }
          break;

        case SAVE_THIS:
          if (interface->getstate() != letter) message = NULL;
          SaveText(file, prt);
          written = true;
          break;
      }

      fclose(file);

      if (written && prt)
      {
        mysystem(makecmd(bm.resourceObject->get(printCommand), canonize(fname)));
        remove(sname);
      }

      if (!written)
      {
        char msg[26];
        sprintf(msg, "No %.8s message found", llopts[SAVE_MAX - stype]);
        msg[3] = tolower(msg[3]);
        interface->ErrorWindow(msg);
        if (fsize(sname) == 0) remove(sname);
      }

      return written;
    }
    else
    {
      interface->ErrorWindow("Could not append to file", false);
      return false;
    }
  }
  else return false;
}


void LetterWindow::SaveText (FILE *save_file, bool prt)
{
  int i;
  const char *Flags = bm.letterList->Flags();
  enum {bbs, area, date, from, to, subj, stat};
  const char *desc[7] = {"BBS", "Area", "Date", "From", "To", "Subj", "Stat"};
  const char *item[7] = {bm.resourceObject->get(BBSName),
                         bm.areaList->getTitle(bm.letterList->getAreaID()),
                         bm.letterList->getDate(), From(),
                         bm.letterList->getTo(), bm.letterList->getSubject(),
                         Flags};
  char *out, *flags;
  bool isInternet = bm.areaList->isInternet(bm.letterList->getAreaID());

  if (!prt) fputc('\n', save_file);
  for (i = 0; i < 75; i++) fputc('=', save_file);
  fputc('\n', save_file);

  if (bm.letterList->isPrivate())
  {
    flags = new char[strlen(Flags) + 5];
    sprintf(flags, "Pvt %s", Flags);
  }
  else flags = (char *) Flags;

  for (i = bbs; i < stat || ((i == stat) && *flags); i++)
  {
    if (i == stat) out = flags;
    else out = interface->charconv_in(i == bbs ? bm.isLatin1()
                                               : (i == area ? bm.areaList->isLatin1()
                                                            : bm.letterList->isLatin1()),
                                      item[i]);

    fprintf(save_file, "%5s: %s", desc[i], out);

    if ((i == area) && bm.areaList->isReplyArea()) fputs(" (Reply)", save_file);

    if (((i == from) && bm.areaList->isNetmail()) ||
        ((i == to) && bm.areaList->isReplyArea()))
    {
      char *addr = bm.letterList->getNetAddr().get(isInternet);
      if (addr) fprintf(save_file, (isInternet ? " <%s>" + (*out ? 0 : 1)
                                               : ", %s"), addr);
    }

    fputc('\n', save_file);
  }

  if (bm.letterList->isPrivate()) delete[] flags;

  for (i = 0; i < 75; i++) fputc('-', save_file);
  fputc('\n', save_file);

  WriteText(save_file, false, isInternet);

  if (prt) fputc('\n', save_file);
}
