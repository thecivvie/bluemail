/*
 * blueMail offline mail reader
 * Interface (high level part of interface)

 Copyright (c) 1996 Kolossvary Tamas <thomas@tvnet.hu>
 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <ctype.h>
#include <string.h>
#include "interfac.h"
#include "../common/error.h"
#include "../common/mysystem.h"
#include "../bluemail/service.h"


Interface::Interface ()
{
  colorlist = new Color();
  // so far, curses only
  ui = new UserInterface(bm.resourceObject->isYes(Transparency),
                         colorlist->getBackground());
  colorlist->convert(ui);

  prevstate = state = nostate;
  beeper = exitNow = quitNow = false;
  input = screen = welcome = NULL;

  isoDisplay = isIso(bm.resourceObject->get(ConsoleCharset));
  isoToggle = false;
  charconv_buf = NULL;
  keyHandler = KH_MAIN;
  *search_for = '\0';
  shadow = true;
}


Interface::~Interface ()
{
  delete welcome;
  delete screen;
  delete input;
  delete colorlist;
  delete ui;
  delete[] charconv_buf;
}


void Interface::mainwin (bool initial)
{
  if (COLS < 80 || LINES < 24)
    fatalError("A screen at least 80x24 is required.");

  if (COLS > 999) fatalError("Too many columns.");

  bool trans = bm.resourceObject->isYes(Transparency);

  // this is a dummy, assistant window for key input only
  // which will not be changed nor destroyed during SIGWINCH
  // (because doing so would cause a segmentation fault)
  if (initial) input = new Win(1, 1, 1, 1, C_MBACK | (trans ? 0 : ACS_BOARD));

  // background window, filled with ACS_BOARD characters
  screen = new Win(0, 0, 0, 0, C_MBACK | (trans ? 0 : ACS_BOARD));

  // border and header
  char bmtitle[60];
  sprintf(bmtitle, BM_TOPHEADER, BM_NAME, BM_MAJOR, BM_MINOR);
  screen->boxtitle(C_MBORDER, bmtitle, C_MBACK | A_BOLD);

  // help window area
  screen->attrib(C_MSEPBOTT);
  screen->horizline(LINES - 5, COLS - 2);

  screen->delay_update();

  // welcome screen
  welcome = new Win(6, 56, 2, COLS / 2 - 28, C_WELCBORDER);
  welcome->attrib(C_WELCBORDER);
  welcome->Border();
  welcome->attrib(C_WELCHEADER);
  welcome->putstring(1, 8, "Welcome to " BM_NAME " " BM_IS_WHAT "!");
  welcome->attrib(C_WELCTEXT);
  welcome->putstring(3, 2, "Written by Ingo Brueckl, William McBrine, John Zero,");
  welcome->putstring(4, 2, "Kolossvary Tamas, Toth Istvan, and others.");
  welcome->delay_update();
}


int Interface::WarningWindow (const char *warning, const char **selectors,
                              int items)
{
  static const char *yesno[] = {"Yes", "No"};     // default options

  int x, y, z, o = 0, itemlen, curitem, defitem = 0, c;
  bool result = false;

  if (!selectors) selectors = yesno;

  // calculate the window width and maximum item width
  for (itemlen = 0, curitem = 0; curitem < items; curitem++)
  {
    z = strlen(selectors[curitem]);
    if (z > itemlen) itemlen = z;
  }
  itemlen += 2;
  x = itemlen * (items + 1);
  z = strlen(warning);
  if (z + 4 > x) x = z + 4;

  ShadowedWin warn(7, x, (LINES >> 1) - 4, (COLS - x) >> 1, C_WARNTEXT, NULL,
                   0, 3, shadow);
  warn.putstring(2, (x - z) >> 1, warning);

  y = x / (items + 1);

  // check balance
  if (items)
  {
    int x0 = y - (strlen(selectors[0]) >> 1);
    int x1 = items * y - (strlen(selectors[items - 1]) >> 1);
    if (x - x1 - strlen(selectors[items - 1]) - x0 >= 2) o = 1;
  }

  // display each item
  const char *p;
  for (curitem = 0; curitem < items; curitem++)
  {
    z = strlen(selectors[curitem]) >> 1;
    x = (curitem + 1) * y - z + o;
    p = selectors[curitem] + 1;

    warn.attrib(C_WARNTEXT);
    warn.putstring(4, x + 1, p--);

    warn.attrib(C_WARNKEYS);
    warn.putch(4, x, *p);
  }

  // main loop - highlight selected item and process keystrokes
  while (!result)
  {
    for (curitem = 0; curitem < items; curitem++)
    {
      z = strlen(selectors[curitem]) >> 1;
      x = (curitem + 1) * y - z + o;
      warn.putch(4, x - 1, (defitem == curitem ? '[' : ' '));
      warn.putch(4, x + strlen(selectors[curitem]), (defitem == curitem ? ']'
                                                                        : ' '));
    }
    warn.update();

    c = input->inkey();

    for (curitem = 0; (curitem < items) && !result; curitem++)
    {
      p = selectors[curitem];
      if (c == tolower(*p) || c == toupper(*p))
      {
        defitem = curitem;
        result = true;
      }
    }

    if (!result)
      switch (c)
      {
        case KEY_ESC:
          defitem = items;     // will force return-value to be -1
          result = true;
          break;

        case KEY_LEFT:
          if (defitem == 0) defitem = items;
          defitem--;
          break;

        case KEY_RIGHT:
        case KEY_TAB:
          if (++defitem == items) defitem = 0;
          break;

        case KEY_ENTER:
          result = true;
      }
  }

  // the leftmost item is (items - 1), the rightmost item is 0, ESC is -1
  return (items - 1) - defitem;
}


void Interface::ErrorWindow (const char *err, bool isState)
{
  static const char *ok[] = {"Ok"};

  WarningWindow(err, ok, 1);

  if (isState) update(state != letter);
  else
  {
    switch (state)
    {
      case arealist:
      case letterlist:
        currlist->Touch();
        break;

      case letter:
        letterwin.Touch();
        break;

      default:
        ;
    }
  }
}


void Interface::InfoWindow (const char *msg, int seconds)
{
  int len = strlen(msg) + 6;

  ShadowedWin info(3, len, (LINES >> 1) - 4, (COLS - len) >> 1, C_INFOTEXT);
  info.putstring(1, 3, msg);
  info.update();
  mysleep(seconds);
  update(false);
}


int Interface::QueryBox (const char *header, char *input, int inputlen,
                         bool query)
{
  chtype C_BOXBORDER = (query ? C_QUERYBOXBORDER : C_FILEBOXBORDER);
  chtype C_BOXHEADER = (query ? C_QUERYBOXHEADER : C_FILEBOXHEADER);
  chtype C_BOXINP = (query ? C_QUERYBOXINP : C_FILEBOXINP);

  ShadowedWin querybox(4, 60, (LINES - 4) >> 1, (COLS - 60) >> 1, C_BOXBORDER,
                       NULL, 0, 3, shadow);
  querybox.attrib(C_BOXHEADER);
  querybox.putstring(1, 2, header);
  querybox.delay_update();

  return querybox.getstring(2, 2, input, inputlen,
                            C_BOXINP, C_BOXINP,
                            bm.resourceObject->isYes(Pos1Input));
}


statetype Interface::getstate () const
{
  return state;
}


statetype Interface::getprevstate () const
{
  return prevstate;
}


void Interface::endstate (statetype oldst)
{
  helpwin.Delete();

  switch (oldst)
  {
    case nostate:
      break;

    case servicelist:
      services.Delete();
      break;

    case packetlist:
    case mboxlist:
      files.Delete();
      break;

    case filedblist:
      filedbs.Delete();
      break;

    case replymgrlist:
      systems.Delete();
      break;

    case arealist:
      areas.Delete();
      break;

    case letterlist:
      letters.Delete();
      break;

    case letter:
#ifdef WITH_CLOCK
      input->inkey_wait(true);
#endif
      letterwin.Delete();
      break;

    case littlearealist:
      areas.Select();
      // no break here!
    case bulletinlist:
    case offlineconflist:
    case addressbook:
    case taglinebook:
      currlist->Delete();
      endstate(prevstate);
      break;

    case ansiview:
      ansiwin.Delete();
      break;
  }

  screen->touch();
  welcome->touch();
}


void Interface::newstate (statetype newst)
{
  currlist = NULL;

  if (state != newst)
  {
    prevstate = state;
    state = newst;

    helpwin.Reset();
  }

  switch (newst)
  {
    case nostate:
      break;

    case servicelist:
      currlist = &services;
      services.MakeActive();
      break;

    case packetlist:
    case mboxlist:
      currlist = &files;
      files.MakeActive();
      break;

    case filedblist:
      currlist = &filedbs;
      filedbs.MakeActive();
      break;

    case replymgrlist:
      currlist = &systems;
      systems.MakeActive();
      break;

    case arealist:
      currlist = &areas;
      areas.MakeActive();
      break;

    case bulletinlist:
      // if we come back from ansiview, prevstate is destroyed,
      // so make sure, it's always arealist
      prevstate = arealist;
      newKeyHandle(bulletinlist, &bulletins);
      break;

    case offlineconflist:
      newKeyHandle(offlineconflist, &offlineconfs);
      break;

    case letterlist:
      currlist = &letters;
      letters.MakeActive();
      break;

    case letter:
#ifdef WITH_CLOCK
      if (toupper(*(bm.resourceObject->get(ClockMode))) != 'O')
        input->inkey_wait(false);
#endif
      letterwin.MakeActive(prevstate == letterlist);     // if true, letter
      break;                                             // starts from line 1

    case littlearealist:
      newKeyHandle(littlearealist, &littleareas);
      break;

    case addressbook:
      newKeyHandle(addressbook, &addresses);
      break;

    case taglinebook:
      newKeyHandle(taglinebook, &taglines);
      break;

    case ansiview:
      ansiwin.MakeActive();
      if (!(keyHandler & KH_ANSI))
      {
        keyHandler |= KH_ANSI;     // begin an additional
        KeyHandle();               // KeyHandle loop
      }
      break;
  }

  helpwin.MakeActive();
}


void Interface::changestate (statetype newst)
{
  endstate(state);
  newstate(newst);
}


void Interface::newKeyHandle (statetype newst, ListWindow *listwin)
{
  newstate(prevstate);
  prevstate = state;
  state = newst;
  helpwin.exchangeActive(newst);
  currlist = listwin;
  currlist->MakeActive();
  if (!(keyHandler & KH_LIST))
  {
    keyHandler |= KH_LIST;     // begin an additional
    KeyHandle();               // KeyHandle loop
  }
}


void Interface::update (bool regular)
{
  if (!regular)
    /* The following is a dirty trick, but prevstate must not be letterlist
       to prevent letter's MakeActive from redrawing the letter from line 1
       and setting the read flag! */
    if (state == letter) prevstate = littlearealist;

  changestate(state);
}


bool Interface::Select ()
{
  switch (state)
  {
    case servicelist:
      switch (services.OpenService())
      {
        case SRVC_NO_PACKET:
          ErrorWindow("No packets found");
          break;

        case SRVC_NO_FILEDB:
          ErrorWindow("No files/databases found");
          break;

        case SRVC_NO_REPLY_INF:
          ErrorWindow("No reply information found");
          break;

        case SRVC_ERROR:
          ErrorWindow(bm.getSelectError());
          break;

        case SRVC_OK:
          startReading();
          break;

        case SRVC_DONE:
          break;
      }
      break;

    case packetlist:
    case mboxlist:
      if (files.OpenFile()) startReading();
      else ErrorWindow(bm.getSelectError());
      break;

    case filedblist:
      switch (filedbs.OpenFileDB())
      {
        case SRVC_NO_PACKET:
          ErrorWindow("No files found");
          break;

        case SRVC_ERROR:
          ErrorWindow(bm.getSelectError());
          break;

        case SRVC_OK:
          startReading();
          break;

        default:
          ;
      }
      break;

    case replymgrlist:
      if (systems.OpenReply()) startReading();
      else ErrorWindow(bm.getSelectError());
      break;

    case arealist:
      areas.Select();
      if (bm.areaList->getNoOfLetters() > 0)
      {
        bm.areaList->getLetterList();
        letters.initSort();
        changestate(letterlist);
        letters.FirstUnread();
        if (Key != 'S')
          if (bm.areaList->getNoOfUnread() &&
              bm.resourceObject->isYes(SkipLetterList)) Select();
      }
      break;

    case bulletinlist:
      bulletins.OpenBulletin();
      break;

    case letterlist:
      letters.Select();
      changestate(letter);
      break;

    case letter:
      letterwin.Next();
      break;

    case littlearealist:
      lalreturn = littleareas.tellArea();
      return back();

    case addressbook:
    case taglinebook:
      currlist->KeyHandle(KEY_ENTER);
      return back();

    case ansiview:
      if (prevstate == arealist) return back();
      break;

    default:
      ;
  }

  return false;
}


bool Interface::back ()
{
  switch (state)
  {
    case servicelist:
      if (exitNow || (WarningWindow("Do you really want to quit "
                                     BM_NAME "?") == WW_YES))
      {
        endstate(state);
        return true;             // program ends now
      }
      else update();
      break;

    case packetlist:
    case filedblist:
    case replymgrlist:
      changestate(servicelist);
      break;

    case mboxlist:
      changestate(filedblist);
      break;

    case arealist:
      if (!saveLastread()) break;
      if (saveReplies())
      {
        bm.Delete();
        offlineconfs.setPos(0);
        changestate(fromstate);
      }
      else
      {
        exitNow = quitNow = false;
        update();
      }
      break;

    case letterlist:
      delete bm.letterList;
      bm.letterList = NULL;
      letters.initFilterFlag();
      letters.resetSubjOffset();
      letterwin.Reset();
      changestate(arealist);
      // only in case we came from letterwin.jumpReplies():
      letterwin.returnReplies(!exitNow && !quitNow);
      break;

    case letter:
      changestate(letterlist);
      break;

    case littlearealist:
      littleareas.Quit();
      // no break here!
    case bulletinlist:
    case offlineconflist:
    case addressbook:
    case taglinebook:
      changestate(prevstate);
      keyHandler &= ~KH_LIST;     // finish the additional
      return true;                // KeyHandle loop

    case ansiview:
      changestate(prevstate);
      keyHandler &= ~KH_ANSI;     // finish the additional
      return true;                // KeyHandle loop

    default:
      ;
  }

  return false;
}


#ifdef SIGWINCH
// high level (application) SIGWINCH handler - called by UserInterface
void Interface::sigwinch ()
{
  delete welcome;
  welcome = NULL;
  delete screen;
  screen = NULL;
  ui->resize();
  mainwin(false);
  update();
  ui->doUpdate();
}
#endif


void Interface::startReading ()
{
  const char **b = bm.getBulletins();

  bulletins.set(b);
  int noOfBulletins = bulletins.noOfItems();

  unsaved_reply = any_read = any_marked = false;
  fromstate = state;
  changestate(arealist);

  if (bm.resourceObject->isYes(OmitBulletins))
  {
    if (noOfBulletins > 0) InfoWindow("Bulletins available");
  }
  else
  {
    int i = 0;

    while (i < noOfBulletins)
    {
      bulletins.OpenBulletin(i, false);

      switch (Key)
      {
        case KEY_ENTER:
        case KEY_RIGHT:
          i++;
          break;

        case KEY_LEFT:
          if (i > 0) i--;
          break;

        default:
          i = noOfBulletins;
      }
    }
  }

  if (bm.areaList->getNoOfAreas() == 0)
  {
    ErrorWindow("No areas available");
    back();
  }

  if (bm.areaList->isAnyMarked() && (bm.getServiceType() != ST_REPLY) &&
      !bm.resourceObject->isYes(OmitAreaMarkInfo))
  {
    bool mark = false;

    for (int a = 0; a < bm.areaList->getNoOfActive(); a++)
    {
      bm.areaList->gotoActive(a);
      mark = (mark || bm.areaList->isMarked());
      if (mark) break;
    }

    if (!mark) InfoWindow("Area marks exist");
  }

  areas.FirstUnread();
}


void Interface::ansiView (const char *source, const char *title, bool latin1)
{
  ansiwin.set(source, title, latin1);
  changestate(ansiview);
}


void Interface::ansiView (FILE *file, const char *title, bool latin1)
{
  ansiwin.set(file, title, latin1);
  changestate(ansiview);
}


bool Interface::isLatin1 () const
{
  return isoDisplay;
}


void Interface::charsetToggle ()
{
  isoToggle = !isoToggle;
  update(false);
}


void Interface::setUnsaved ()
{
  unsaved_reply = true;
}


void Interface::setSaved ()
{
  unsaved_reply = false;
}


bool Interface::isUnsaved () const
{
  return unsaved_reply;
}


void Interface::setAnyRead ()
{
  any_read = true;
}


void Interface::setAnyMarked ()
{
  any_marked = true;
}


bool Interface::saveLastread ()
{
  int total = 0, unread = 0;
  bool isFile = (fromstate == packetlist || fromstate == mboxlist);
  bool save = (isFile || (fromstate == filedblist));

  for (int a = 0; a < bm.areaList->getNoOfAreas(); a++)
  {
    bm.areaList->gotoArea(a);

    if (!bm.areaList->isReplyArea() && !bm.areaList->isCollection())
    {
      total += bm.areaList->getNoOfLetters();
      unread += bm.areaList->getNoOfUnread();
    }
  }

  int r = WW_YES;

  /* lastread pointers */

  if (save && any_read)
  {
    if (bm.resourceObject->isYes(SaveLastreadPointers) ||
        ((r = WarningWindow("Save lastread pointers?")) == WW_YES))
    {
      lrtype lr = bm.saveLastread();

      if (lr == LR_OK)
      {
        if (isFile)
          files.fileStat->addStat(bm.getLastFileID(), bm.getLastFileMtime(),
                                  total, unread);
      }
      else
      {
        ErrorWindow("Unable to save lastread pointers!");
        // give the user a chance to leave the program
        bm.resourceObject->set(SaveLastreadPointers, NULL);
        // if LR_ERROR, return false - if LR_NORETRY, return true
        r = (lr == LR_ERROR ? WW_ESC : WW_NO);
      }
    }

    if (r == WW_ESC)
    {
      exitNow = quitNow = false;
      update();
    }
    else any_read = false;
  }
  else if (isFile)
    files.fileStat->addStat(bm.getLastFileID(), bm.getLastFileMtime(),
                            total, unread);

  /* area marks */

  if ((r != WW_ESC) && (bm.getServiceType() != ST_DEMO) && any_marked)
  {
    if (bm.resourceObject->isYes(SaveAreaMarks) ||
        ((r = WarningWindow("Save area marks?")) == WW_YES))
    {
      if (!bm.saveAreaMarks())
      {
        ErrorWindow("Unable to save area marks!");
        // give the user a chance to leave the program
        bm.resourceObject->set(SaveAreaMarks, NULL);
        // return false
        r = WW_ESC;
      }
    }

    if (r == WW_ESC)
    {
      exitNow = quitNow = false;
      update();
    }
    else any_marked = false;
  }

  return (r != WW_ESC);
}


int Interface::checkReplies ()
{
  int r;

  if (!bm.areaList->repliesOK())
  {
    r = WarningWindow("Cannot save reply in unknown area! Discard changes?");
    return (r == WW_YES ? REP_DISCARD : REP_RETRY);
  }
  else return REP_OK;
}


bool Interface::saveReplies ()
{
  int r = WW_YES;

  if (unsaved_reply)
  {
    switch (checkReplies())
    {
      case REP_DISCARD:
        unsaved_reply = false;
        return true;

      case REP_RETRY:
        return false;

      default:
        ;
    }

    if (bm.resourceObject->isYes(SaveReplies) ||
        ((r = WarningWindow("The content of the reply packet has changed. "
                            "Save new replies?")) == WW_YES))
    {
      if (bm.areaList->makeReply()) unsaved_reply = false;
      else
      {
        ErrorWindow("Unable to save new replies!");
        // give the user a chance to leave the program
        bm.resourceObject->set(SaveReplies, NULL);
        r = WW_ESC;
      }
    }
  }

  return (r != WW_ESC);
}


void Interface::delay_beep ()
{
  beeper = true;
}


void Interface::search ()
{
  searchtype result = FND_NO;

//  InfoWindow("Searching...  (Press any key to abort)", 0);

  switch (state)
  {
    case letter:
      if (*search_for) result = letterwin.search(search_for);
      break;

    case ansiview:
      if (*search_for) result = ansiwin.search(search_for);
      break;

    default:
      if (search_first)
      {
        currlist->savePos();
        currlist->setPos(-1);
      }
      if (*search_for) result = currlist->search(search_for);
      if ((result != FND_YES) && search_first) currlist->restorePos();
  }

  search_first = false;

  if (*search_for)
  {
    if (result == FND_NO) ErrorWindow("No further matches");
  }
  else
  {
    if (state == letter || state == ansiview) update(state != letter);
  }
}


void Interface::user_program ()
{
  int r = GOT_ENTER;
  char input[QUERYINPUT + 1], *p, *cmd;

  const char *pgm = bm.resourceObject->get(userpgmCommand);

  if (pgm)
  {
    cmd = new char[strlen(pgm) + QUERYINPUT + 1];
    strcpy(cmd, pgm);
    *input = '\0';

    if ((p = strstr(cmd, "@P")) && ((r = QueryBox("Parameter:", input,
                                                   QUERYINPUT)) != GOT_ESC))
    {
      strcpy(p, input);
      strcat(cmd, pgm + (p - cmd + 2));
    }

    if (r == GOT_ESC) update();
    else mysystem(cmd);

    delete[] cmd;
  }
}


void Interface::KeyHandle ()     // main loop
{
  int inp;
  bool end = false;

  while (!end)                   // all statetypes (windows) loop here...
  {
    if (exitNow) Key = 'Q';
    else if (quitNow && (state != servicelist)) Key = 'Q';
    else
    {
      quitNow = false;
      ui->doUpdate();            // ...so this is a good time to perform
                                 // all the (delayed) updates
      if (beeper)
      {
        beeper = false;
        ui->Beep();
      }

      Key = input->inkey();
    }

    if (Key >= 'a' && Key <= 'z') Key = toupper(Key);

#ifdef SIGWINCH
    if (ui->sigwinched()) sigwinch();
#endif

    switch (Key)
    {
      case '?':
      case KEY_F(1):
        helpwin.help(state);
        update(state != letter);
        break;

      case 'Q':
      case KEY_ESC:
      case KEY_CTRL_H:
      case KEY_BACKSPACE:
        if (currlist) currlist->Quit();
        end = back();
        break;

      case KEY_ENTER:
        end = Select();
        break;

      case META_D:
        myshell();
        break;

      case META_P:
        user_program();
        break;

      case META_Q:
        quitNow = true;
        break;

      case META_X:
        exitNow = true;
        break;

      case '/':
        if ((inp = QueryBox("Search for:", search_for, QUERYINPUT)) != GOT_ESC)
        {
          // setPos() for these two before update() to clear old search mark,
          // for list windows it's done in search() (after the update())
          if (state == letter) letterwin.setPos(-1, false);
          if (state == ansiview) ansiwin.setPos(-1, false);
        }
        update(state != letter);
        if (inp == GOT_ESC) break;
        search_first = true;
        // no break here!
      case '.':
        search();
        break;

      case '>':
        if (state == arealist || state == letterlist) helpwin.More();
        break;

      default:
        switch (state)
        {
          case offlineconflist:
            if (Key == ' ') Key = META_SPACE;     // allow use of space key
            currlist->KeyHandle(Key);
            break;

          case letter:
            letterwin.KeyHandle(Key);
            break;

          case ansiview:
            if (prevstate == arealist &&
                (Key == KEY_LEFT || Key == KEY_RIGHT)) end = back();
            else ansiwin.KeyHandle(Key);
            break;

          default:
            currlist->KeyHandle(Key);
        }
    }
  }
}


int Interface::midpos (int cols, int max_l, int max_m, int max_r) const
{
  return max_l + 1 + ((COLS - cols - max_l - 1 - max_r - 1 - max_m) >> 1);
}


int Interface::endpos (int cols, int max_r) const
{
  return COLS - cols - 1 - max_r;
}


void Interface::syscallwin ()
{
  ui->Clear();
  Win sysout(3, 15, 0, (COLS - 15) / 2, C_SYSCALLHEAD);
  sysout.Border();
  sysout.putstring(1, 2, "System Call");
  sysout.update();
  sysout.cursor_on();
  ui->gotoyx(4, 0);
  ui->breakPgm();
}


void Interface::endsyscall ()
{
  ui->resumePgm();
  update(false);          // restore screen
}


UserInterface *Interface::getUI () const
{
  return ui;
}


int Interface::selectArea (int type)
{
  littleareas.MakeChain(type);
  lalreturn = -1;
  changestate(littlearealist);
  return lalreturn;     // contains selected area now
}


void Interface::shadowedWin (bool shadow)
{
  this->shadow = shadow;
}
