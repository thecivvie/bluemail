/*
 * blueMail offline mail reader
 * UserInterface, Win and ShadowedWin (low level curses window definitions)

 Copyright (c) 1996 Kolossvary Tamas <thomas@tvnet.hu>
 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef WINCURS_H
#define WINCURS_H


#include <signal.h>

#ifdef CURS_HEADER
#include CURS_HEADER
#endif


#if defined(WITH_CLOCK) && defined(__PDCURSES__)
  #if (PDC_BUILD >= 2601) || defined(PDC_BMPATCH)
    // starting with PDCurses 2.6 the patch for the halfdelay()
    // input function needed for the clock is no longer required
  #else
    #error +-----------------------------------------+
    #error !
    #error ! Your version of PDCurses does not meet
    #error ! the requirements necessary for option
    #error ! WITH_CLOCK. To compile, you must either
    #error !
    #error ! omit option WITH_CLOCK in Makefile.inc
    #error !
    #error ! or
    #error !
    #error ! apply a patch for PDCurses
    #error !
    #error ! which should be available from where
    #error ! you got this source, too. See file
    #error ! INSTALL for more information.
    #error !
    #error +-----------------------------------------+
  #endif
#endif


// key definitions
#define KEY_CTRL_C   3
#define KEY_CTRL_D   4
#define KEY_CTRL_E   5
#define KEY_CTRL_F   6
#define KEY_CTRL_H   8
#define KEY_TAB      9
#define KEY_CTRL_K  11
#define KEY_CTRL_P  16
#define KEY_CTRL_R  18
#define KEY_CTRL_T  20
#define KEY_ESC     27
#define KEY_DEL    127
#define META_SPACE (KEY_MAX + 1)
#define META_D     (KEY_MAX + 2)
#define META_P     (KEY_MAX + 3)
#define META_Q     (KEY_MAX + 4)
#define META_X     (KEY_MAX + 5)
#define META_S1    (KEY_MAX + 6)

// results from ShadowedWin::getstring()
#define GOT_ESC     0
#define GOT_ENTER   1
#define GOT_UP      2
#define GOT_DOWN    3
#define GOT_NOTHING 4   // the maximum value possible plus 1

// some characters are unprintable on standard Unix terminals,
// but PDCurses will handle them
#ifdef __PDCURSES__
#define ANSI_ALL_CHRS
#endif


struct HOTKEY
{
  int keycode;
  const char *description;
  bool selected;
  int xorindex;     // hotkey to be deselected, if this one is selected
};

struct HOTKEYS
{
  int ypos, xpos;
  int maxkey;
  HOTKEY *hotkey;
};


#if defined(SIGWINCH) && !defined(XCURSES) && !defined(__CYGWIN__)
  void sigwinchHandler(int);
#endif


class UserInterface
{
  private:
    bool pair_used[64];
    int f_remap, b_remap;

    void init_colors(bool, const chtype);

  public:
    UserInterface(bool, const chtype);
    ~UserInterface();
    void Clear();
    void doUpdate();
    void Beep();
    void gotoyx(int, int);
    void breakPgm();
    void resumePgm();
#ifdef SIGWINCH
    bool sigwinched();
    void resize();
#endif
    chtype color(chtype);
#ifdef FIXSTANDOUT
    chtype colormode(const chtype, const chtype);
#endif
    void ansi_init(const chtype, const chtype, const chtype);
    chtype ansi_color(const chtype, bool);
    bool ansi_remap();
    void ansi_remap(chtype *, int);
    void ansi_done();
};


class Win
{
  protected:
    WINDOW *win;
    bool hasBorder, keywait;

  public:
    Win(int, int, int, int, chtype);
    ~Win();
    void Clear(chtype);
    void putch(int, int, chtype);
    void putchr(int, int, char);
    void putchnstr(int, int, const chtype *, int);
    void putstring(int, int, const char *, int = -1);
    void clreol(int, int);
    void attrib(chtype);
    void gotoyx(int, int);
    void horizline(int, int);
    void delay_update();
    void update();
    void touch();
    void wscroll(int);
    void cursor_on(int = 1);
    void cursor_off();
    bool keypressed();
#ifdef WITH_CLOCK
    void inkey_wait(bool);
#endif
    int inkey();
    void boxtitle(chtype, const char *, chtype);
    void Border();
};


class ShadowedWin : public Win
{
#ifdef USE_SHADOWS
  private:
    WINDOW *shadow;
#endif

  public:
    ShadowedWin(int, int, int, int, chtype, const char * = NULL, chtype = 0,
                int = 3, bool = true);
    ~ShadowedWin();
    void delay_update();
    void update();
    void touch();
    int getstring(int, int, char *, int, chtype, chtype, bool,
                  HOTKEYS * = NULL, ShadowedWin * = NULL, bool = true);
};


#endif
