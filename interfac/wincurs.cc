/*
 * blueMail offline mail reader
 * UserInterface, Win and ShadowedWin (low level curses window definitions)

 Copyright (c) 1996 Kolossvary Tamas <thomas@tvnet.hu>
 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <ctype.h>
#include <string.h>
#include "wincurs.h"
#include "color.h"
#include "../common/auxil.h"

#ifdef SIGWINCH
#include <sys/ioctl.h>
#endif

#ifdef XCURSES
  char *XCursesProgramName = BM_NAME;
#endif


#if defined(SIGWINCH) && !defined(XCURSES) && !defined(__CYGWIN__)
// reference to high level SIGWINCH handler
class Interface {public: void sigwinch();};
extern Interface *interface;

void sigwinchHandler (int sig)
{
  if (sig == SIGWINCH) interface->sigwinch();
  signal(SIGWINCH, sigwinchHandler);
}
#endif


/*
   user interface
*/

// initialize
UserInterface::UserInterface (bool transparency, const chtype transcolor)
{
  int canTrans = ERR;

#ifdef XCURSES
  int argc = 1;
  char *argv[] = { XCursesProgramName };
  Xinitscr(argc, argv);
#else
  initscr();
#endif

  refresh();
  start_color();
#ifdef NCURSES_VERSION
  canTrans = use_default_colors();
#endif
  init_colors((canTrans == OK) && transparency, transcolor);
  raw();
  noecho();
  nonl();
#ifdef __PDCURSES__
  raw_output(TRUE);
#endif

#if defined(SIGWINCH) && !defined(XCURSES) && !defined(__CYGWIN__)
  signal(SIGWINCH, sigwinchHandler);
#endif
}


// terminate
UserInterface::~UserInterface ()
{
  Clear();
  endwin();
#ifdef XCURSES
  XCursesExit();
#endif
}


// color initialization
void UserInterface::init_colors (bool transparency, const chtype transcolor)
{
  // transcolor is in blueMail internal color type!
  int bgcolor = transcolor & 7;

  for (int b = COLOR_BLACK; b <= (COLOR_WHITE); b++)
    for (int f = COLOR_BLACK; f <= (COLOR_WHITE); f++)
      init_pair((f << 3) + b, f, (transparency && (b == bgcolor) ? -1 : b));

  // Color pair 0 (BLACK on BLACK) cannot be set, it defaults
  // to the color the terminal had before libcurses started.
  // So we use WHITE on WHITE for BLACK on BLACK which is needed
  // for the window's shadows. As a result, WHITE on WHITE can't
  // be used. :-(

  init_pair(((COLOR_WHITE) << 3) + (COLOR_WHITE), COLOR_BLACK, COLOR_BLACK);
}


// clear screen
void UserInterface::Clear ()
{
  touchwin(stdscr);
  refresh();
}


// update now
void UserInterface::doUpdate ()
{
  doupdate();
}


// some noise
void UserInterface::Beep ()
{
  beep();
}


// move cursor
void UserInterface::gotoyx (int y, int x)
{
  wmove(stdscr, y, x);
  wrefresh(stdscr);
}


// break from program to shell mode
void UserInterface::breakPgm ()
{
#if !defined(XCURSES)
  reset_shell_mode();
#endif
}


// resume from shell to program mode
void UserInterface::resumePgm ()
{
#if !defined(XCURSES)
  reset_prog_mode();
#endif
#ifdef __PDCURSES__
  raw_output(TRUE);
#endif
  Clear();
}


#ifdef SIGWINCH
// check whether terminal size has changed
bool UserInterface::sigwinched ()
{
#if defined(XCURSES) || defined(__CYGWIN__)
  return is_termresized();
#else
  return false;
#endif
}


// terminal size has changed
void UserInterface::resize ()
{
#if defined(XCURSES) || defined(__CYGWIN__)
  resize_term(0, 0);
#else
  struct winsize size;
  if (ioctl(fileno(stdout), TIOCGWINSZ, &size) == 0) resizeterm(size.ws_row,
                                                                size.ws_col);
#endif
}
#endif


// convert internal bmcolor type into user interface chtype
chtype UserInterface::color (chtype bmcolor)
{
  chtype fg, bg, attr;

  // bmcolor is in blueMail internal color type!
  fg = (bmcolor >> 3) & 7;
  bg = bmcolor & 7;
  attr = bmcolor & (chtype) ~COL(7, 7);

  // remember: we defined BLACK on BLACK as color pair WHITE on WHITE
  if (fg == (COLOR_BLACK) && bg == (COLOR_BLACK))
    return COLOR_PAIR(((COLOR_WHITE) << 3) + (COLOR_WHITE)) | attr;

  // WHITE on WHITE (as user color string) is the terminal's default color
  if (fg == (COLOR_WHITE) && bg == (COLOR_WHITE))
    return COLOR_PAIR(((COLOR_BLACK) << 3) + (COLOR_BLACK)) | attr;

  return COLOR_PAIR((fg << 3) + bg) | attr;
}


// better color choices for REVERSE and STANDOUT
#ifdef FIXSTANDOUT
chtype UserInterface::colormode (const chtype color, const chtype mode)
{
  chtype fg, bg, bold;

  fg = PAIR_NUMBER(color) >> 3;
  bg = PAIR_NUMBER(color) & 7;
  bold = color & A_BOLD;

  if (mode == A_REVERSE) return COLOR_PAIR((bg << 3) + fg) | bold;

  if (mode == A_STANDOUT)
  {
    if (fg == (COLOR_WHITE) && bg == (COLOR_WHITE)) return color ^ A_BOLD;

    if (fg == (COLOR_BLACK) && bg == (COLOR_WHITE))
      return COLOR_PAIR(((COLOR_WHITE) << 3) + (COLOR_BLACK)) | bold;

    if (bg == (COLOR_WHITE))
      return COLOR_PAIR((fg << 3) + (COLOR_BLACK)) | bold;

    if (bold) return COLOR_PAIR(((COLOR_BLACK) << 3) + (COLOR_WHITE));
    else return COLOR_PAIR((bg << 3) + (COLOR_WHITE)) |
                (bg == (COLOR_BLACK) ? A_BOLD : A_NORMAL);
  }

  return color | mode;
}
#endif


// The ANSI functions are only necessary because curses isn't able to
// display all its 64 color pairs; color pair 0 is unavailable. :-(
// To give the ANSI viewer a chance to use color pair 0 if not all of the
// color pairs are used, we temporarily will remap it onto an unsed color
// pair (if any) for the time the ANSI viewer is active.

// initialize the ANSI handling
void UserInterface::ansi_init (const chtype used1, const chtype used2,
                               const chtype used3)
{
  for (int i = 0; i < 64; i++) pair_used[i] = false;
  // curses type colors already in use
  pair_used[PAIR_NUMBER(used1)] = true;
  pair_used[PAIR_NUMBER(used2)] = true;
  pair_used[PAIR_NUMBER(used3)] = true;

  // BLACK on BLACK not yet remapped
  f_remap = b_remap = 0;

  // if all 64 color pairs will be used, it is (a little bit) less annoying
  // if BLACK on BLACK rather than WHITE on WHITE will be displayed wrongly,
  // so we temporarily enable the use of WHITE on WHITE
  init_pair(((COLOR_WHITE) << 3) + (COLOR_WHITE), COLOR_WHITE, COLOR_WHITE);
}


// convert an ANSI (blueMail internal) color type to a curses one
chtype UserInterface::ansi_color (const chtype bmcolor, bool mark_used)
{
  chtype fg, bg, attr, result;

  // bmcolor is in blueMail internal color type!
  fg = (bmcolor >> 3) & 7;
  bg = bmcolor & 7;
  attr = bmcolor & (chtype) ~COL(7, 7);

  // in PDCurses, the A_REVERSE attribute resets the colors,
  // so we avoid using it
#ifdef __PDCURSES__
  if (attr & A_REVERSE)
  {
    chtype c = fg;
    fg = bg;
    bg = c;
    attr &= ~A_REVERSE;
  }
#endif

  // COLOR_PAIR, not color() here, as WHITE on WHITE is enabled
  result = COLOR_PAIR((fg << 3) + bg) | attr;

  // ANSI viewing
  if (mark_used) pair_used[PAIR_NUMBER(result)] = true;
  // ANSI animating
  else ansi_remap(&result, 1);

  return result;
}


// check whether remapping is necessary and possible
bool UserInterface::ansi_remap ()
{
  bool found = false;

  if (pair_used[0])
    for (int i = 1; i < 64; i++)
      if (!pair_used[i])
      {
        found = true;
        f_remap = i >> 3;
        b_remap = i & 7;
        init_pair(i, COLOR_BLACK, COLOR_BLACK);
        break;
      }

  return found;
}


// accomplish remapping
void UserInterface::ansi_remap (chtype *text, int len)
{
  for (int i = 0; i < len; i++)
    if (PAIR_NUMBER(text[i]) == 0)
    {
      text[i] &= ~A_COLOR;
      text[i] |= COLOR_PAIR((f_remap << 3) + b_remap);
    }
}


// clean up
void UserInterface::ansi_done ()
{
  if (f_remap || b_remap)
    init_pair((f_remap << 3) + b_remap, f_remap, b_remap);

  init_pair(((COLOR_WHITE) << 3) + (COLOR_WHITE), COLOR_BLACK, COLOR_BLACK);
}


/*
   win
*/

// create new window (and fill with background)
Win::Win (int l, int c, int y, int x, chtype backg)
{
  win = newwin(l, c, y, x);
  Clear(backg);
  keypad(win, TRUE);
  cursor_off();
  hasBorder = false;
  keywait = true;
}


// delete window
Win::~Win ()
{
  delwin(win);
}


// clear window (by filling it with backg)
void Win::Clear (chtype backg)
{
  wbkgdset(win, backg);
  werase(win);
  wbkgdset(win, ' ');       // no background character allowed from now on,
  wattrset(win, backg);     // only the backgound attribute(s) will be used
}


// put a character with attribute(s)
void Win::putch (int y, int x, chtype z)
{
  mvwaddch(win, y, x, z);
}


// put a character
void Win::putchr (int y, int x, char c)
{
  mvwaddch(win, y, x, (unsigned char) c);
}


// put a string with attribute(s)
void Win::putchnstr (int y, int x, const chtype *z, int len)
{
  mvwaddchnstr(win, y, x, (chtype *) z, len);
}


// put a string
void Win::putstring (int y, int x, const char *z, int len)
{
  int maxlen = getmaxx(win) - x;
  if (hasBorder) maxlen--;

  if (len == -1 || len > maxlen) len = maxlen;
  mvwaddnstr(win, y, x, (char *) z, len);
}


// clear until end of line
void Win::clreol (int y, int x)
{
  int end = getmaxx(win);
  if (hasBorder) end--;

  for (int i = x; i < end; i++) putchr(y, i, ' ');
}


// set window attribute
void Win::attrib (chtype z)
{
  wattrset(win, z);
}


// move cursor
void Win::gotoyx (int y, int x)
{
  wmove(win, y, x);
}


// draw a line
void Win::horizline (int y, int len)
{
  wmove(win, y, 1);
  whline(win, ACS_HLINE, len);
}


// prepare window for (a partial) update
void Win::delay_update ()
{
  wnoutrefresh(win);
}


// update window now
void Win::update ()
{
  wrefresh(win);
}


// prepare window for a complete update
void Win::touch ()
{
  touchwin(win);
  wnoutrefresh(win);
}


// scroll window
void Win::wscroll (int i)
{
  scrollok(win, TRUE);
  wscrl(win, i);
  scrollok(win, FALSE);
}


// make cursor visible
void Win::cursor_on (int visibility)
{
  leaveok(win, FALSE);
  curs_set(visibility);
}


// make cursor invisible
void Win::cursor_off ()
{
  leaveok(win, TRUE);
  curs_set(0);
}


// check whether a key is pressed (but don't wait)
bool Win::keypressed ()
{
  nodelay(win, TRUE);
  return (wgetch(win) != ERR);
}


#ifdef WITH_CLOCK
// set or unset whether inkey() should wait until a key is pressed
// or return after a while even if no key has been pressed
void Win::inkey_wait (bool wait)
{
  keywait = wait;
}
#endif


// wait for a key input
// (As a side effect, wgetch does a wrefresh first!)
int Win::inkey ()
{
  nodelay(win, keywait ? FALSE : TRUE);
  // wait 5 seconds for a key - if no key pressed,
  // return with key = ERR from wgetch()
  if (!keywait) halfdelay(50);

#ifdef __PDCURSES__
  PDC_save_key_modifiers(TRUE);
#endif

  int key = wgetch(win);

  // reset halfdelay
  if (!keywait)
  {
    nocbreak();
    cbreak();
    raw();
  }

  if (key == '\n' || key == '\r') key = KEY_ENTER;

#ifdef __PDCURSES__
  if (key == KEY_CTRL_H &&
      (PDC_get_key_modifiers() & PDC_KEY_MODIFIER_CONTROL) == 0)
    key = KEY_BACKSPACE;
#endif

#ifdef __PDCURSES__
  if (key == ALT_D) return META_D;
  if (key == ALT_P) return META_P;
  if (key == ALT_Q) return META_Q;
  if (key == ALT_X) return META_X;
#else
  nodelay(win, TRUE);

  if (key == KEY_ESC)
  {
    key = wgetch(win);
    if (key >= 'a' && key <= 'z') key = toupper(key);

    if (key == 'D') return META_D;
    else if (key == 'P') return META_P;
    else if (key == 'Q') return META_Q;
    else if (key == 'X') return META_X;
    else return (key == ERR ? KEY_ESC : ERR);
  }
#endif
  else return key;
}


// set window title (and draw a border around window)
void Win::boxtitle (chtype backg, const char *title, chtype titleAttrib)
{
  wattrset(win, backg);
  Border();

  if (title)
  {
    putch(0, 2, ACS_RTEE);
    putch(0, 3 + strlen(title), ACS_LTEE);
    wattrset(win, titleAttrib);
    putstring(0, 3, title);
    wattrset(win, backg);
  }
}


// draw a border
void Win::Border ()
{
  box(win, 0, 0);
  hasBorder = true;
}


/*
   shadowed win
*/

// window, casting a shadow
ShadowedWin::ShadowedWin (int l, int c, int y, int x, chtype backg,
                          const char *title, chtype titleAttrib,
#ifdef USE_SHADOWS
                          int limit, bool shadowed)
#else
                          int, bool)
#endif
           : Win(l, c, y, x, backg)
{
  // The text to be in "shadow" is taken from different windows,
  // depending on the curses implementation. Note that the default,
  // "stdscr", just makes the shadowed area solid black; only with
  // ncurses and PDCurses does it draw proper shadows.

#ifdef USE_SHADOWS

#define SHADOW(c) (shadowed ? (c & (A_CHARTEXT | A_ALTCHARSET)) | C_SHADOW : c)

  int i, j;
  chtype *right, *lower;

#ifndef NCURSES_VERSION
#ifdef __PDCURSES__
  WINDOW *newscr = curscr;
#else
  WINDOW *newscr = stdscr;
#endif
#endif

  // If there are 'limit' columns or more free space to the right margin of
  // the screen, make a normal, 2 column right shadow. If there are only
  // 'limit - 1' columns free space, make a 1 column right shadow (i.e. 1
  // column will be inside the window and 1 column outside; the inside part
  // will be overwritten later by the normal window content). Make no shadow
  // (i.e. 2 columns inside the window) if only 'limit - 2' columns or less
  // are left. (Default 'limit' is 3 to protect the border. If no border is
  // to protect, 'limit' should be 2.)
  while ((c + x) > (COLS - limit)) c--;
  // Same for lines: If 2 lines or more free space is left to the lower
  // bottom, there will be a normal, 1 line lower shadow. No shadow (i.e.
  // shadow inside) if 1 line or less is free.
  while ((l + y) > (LINES - 2)) l--;

  right = new chtype[(l - 1) << 1];
  lower = new chtype[c + 1];

  // gather the old text and attribute info

  for (i = 0; i < l - 1; i++)
    for (j = 0; j <= 1; j++)
      right[(i << 1) + j] = mvwinch(newscr, y + i + 1, x + c + j);

  mvwinchnstr(newscr, y + l, x + 2, lower, c);

  // redraw it in darkened form

  shadow = newwin(l, c, y + 1, x + 2);
  leaveok(shadow, TRUE);
  curs_set(0);

  for (i = 0; i < l - 1; i++)
    for (j = 0; j <= 1; j++)
      mvwaddch(shadow, i, c - 2 + j, SHADOW(right[(i << 1) + j]));

  for (i = 0; i < c; i++) mvwaddch(shadow, l - 1, i, SHADOW(lower[i]));
#endif

  boxtitle(backg, title, titleAttrib);

#ifdef USE_SHADOWS
  delete[] lower;
  delete[] right;
#endif
}


// delete window
ShadowedWin::~ShadowedWin ()
{
#ifdef USE_SHADOWS
  delwin(shadow);
#endif
}


// prepare window for (a partial) update
void ShadowedWin::delay_update ()
{
#ifdef USE_SHADOWS
  wnoutrefresh(shadow);
#endif
  Win::delay_update();
}


// update window now
void ShadowedWin::update ()
{
#ifdef USE_SHADOWS
  wrefresh(shadow);
#endif
  Win::update();
}


// prepare window for a complete update
void ShadowedWin::touch ()
{
#ifdef USE_SHADOWS
  touchwin(shadow);
  wnoutrefresh(shadow);
#endif
  Win::touch();
}


// string input (editable, scrollable, with hotkeys)
int ShadowedWin::getstring (int y, int x, char *string, int n,
                            chtype inp_color, chtype norm_color,
                            bool pos1, HOTKEYS *hotkeys, ShadowedWin *subwin,
                            bool shadowed)
{
  int index, j, endkey, key, offset, max, klen, koffset;
  char *inp = new char[n + 1];
  bool first_key, insmode = true, hotkey = false;

#define APPROPRIATE(function) shadowed ? function : Win::function

  cropesp(string);
  attrib(inp_color);

  max = getmaxx(win) - x;
  if (hasBorder) max--;
  max = (n > max ? max : n);

  for (j = 0; j < n; j++)
  {
    if (j < max) putch(y, x + j, ' ');
    inp[j] = '\0';
  }
  inp[n] = '\0';

  j = strlen(string);
  offset = ((j >= max) && !pos1 ? j - max + 1 : 0);

  putstring(y, x, string + offset, max);
  if (pos1) gotoyx(y, x);

  cursor_on();
  APPROPRIATE(update());

  index = endkey = 0;
  first_key = true;

  while (!endkey)
  {

    do
    {
      if (hotkeys)   // draw descriptions
      {
        attrib(norm_color);

        klen = koffset = 0;
        for (j = 0; j < hotkeys->maxkey; j++)
        {
          int len = strlen(hotkeys->hotkey[j].description) + 1;
          klen += len;
          if (hotkeys->hotkey[j].selected)
          {
            putstring(hotkeys->ypos, hotkeys->xpos + koffset,
                      hotkeys->hotkey[j].description);
            koffset += len;
            putchr(hotkeys->ypos, hotkeys->xpos + koffset - 1, ' ');
          }
        }
        for (j = koffset; j < klen - 1; j++)
          putchr(hotkeys->ypos, hotkeys->xpos + j, ' ');

        attrib(inp_color);
        gotoyx(y, x + index);
      }

      key = inkey();

      if (key == META_D)
      {
        myshell();
        cursor_on();
        APPROPRIATE(touch());
        if (subwin) shadowed ? subwin->touch() : ((Win *) subwin)->touch();
        APPROPRIATE(update());
      }
      else if (hotkeys)   // check whether pressed
      {
        hotkey = false;
        for (j = 0; j < hotkeys->maxkey; j++)
          if (key == hotkeys->hotkey[j].keycode)
          {
            hotkeys->hotkey[j].selected = !hotkeys->hotkey[j].selected;
            hotkey = true;
            int xorindex = hotkeys->hotkey[j].xorindex;
            if (hotkeys->hotkey[j].selected && (xorindex >= 0))
              hotkeys->hotkey[xorindex].selected = false;
            break;
          }
      }
    }
    while ((key == META_D) || hotkey);

    // these keys make the string "accepted" and editable
    if (first_key)
    {
      if (key == KEY_HOME || key == KEY_END || key == KEY_LL ||
          key == KEY_LEFT || key == KEY_RIGHT ||
          key == KEY_DEL || key == KEY_DC || key == KEY_IC ||
          key == KEY_CTRL_H || key == KEY_BACKSPACE ||
          key == KEY_DOWN || key == KEY_TAB || key == KEY_UP ||
          key == KEY_ENTER || key == KEY_ESC)
      {
        strcpy(inp, string);
        if (!pos1) index = strlen(string);
      }
      else offset = 0;
    }

    first_key = false;

    switch (key)
    {
      case KEY_DOWN:
      case KEY_TAB:
        endkey++;
        // no break here!
      case KEY_UP:
        endkey++;
        // no break here!
      case KEY_ENTER:
        endkey++;
        // no break here!
      case KEY_ESC:
        endkey++;
        break;

      case KEY_LEFT:
        if (index > 0) index--;
        break;

      case KEY_RIGHT:
        if (inp[index]) index++;
        break;

      case KEY_DEL:
      case KEY_DC:
        strcpy(&inp[index], &inp[index + 1]);
        break;

      case KEY_IC:
        insmode = !insmode;
        cursor_on(insmode ? 1 : 2);
        break;

      case KEY_CTRL_H:
      case KEY_BACKSPACE:
        if (index > 0)
        {
          strcpy(&inp[index - 1], &inp[index]);
          index--;
        }
        break;

      case KEY_HOME:
        index = 0;
        break;

      case KEY_END:
      case KEY_LL:
        while (inp[index]) index++;
        break;

      default:
        if ((unsigned char) key >= ' ')
        {
          if (insmode) for (j = n - 1; j > index; j--) inp[j] = inp[j - 1];
          inp[index++] = key;
        }
    }

    if (index >= n) index = n - 1;

    if (index >= max + offset) offset = index - max + 1;
    if (index < offset) offset = index;

    for (j = 0; j < max; j++)
      putchr(y, x + j, (inp[j + offset] ? inp[j + offset] : ' '));
    gotoyx(y, x + index - offset);
    APPROPRIATE(update());
  }

  strcpy(string, inp);
  attrib(norm_color);
  j = strlen(string);
  for (int i = 0; i < max; i++) putchr(y, x + i, (i < j ? string[i] : ' '));

  cursor_off();
  APPROPRIATE(update());

  delete[] inp;
  return endkey - 1;     // 3 = KEY_DOWN/TAB, 2 = KEY_UP, 1 = ENTER, 0 = ESC
}
