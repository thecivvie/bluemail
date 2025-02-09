/*
 * blueMail offline mail reader
 * help windows

 Copyright (c) 1996 Kolossvary Tamas <thomas@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "interfac.h"
#include "../common/auxil.h"


HelpWindow::HelpWindow ()
{
  up_txt = "    Up: Up one line";
  down_txt = "  Down: Down one line";
  pgup_txt = "  PgUp: Up one screen";
  pgdn_txt = "  PgDn: Down one screen";
  home_txt = "  Home: Top of screen";
  end_txt = "   End: End of screen";
  search_txt = " /: Search";
  fndnxt_txt = " .: Find next";
  save_txt = ": save replies now";

  Reset();
}


void HelpWindow::newHelpMenu (const char **keys, const char **func)
{
  int y, z;
  int l, max_l = 0, max_m = 0, max_r = 0;

  menu = new Win(3, COLS - 2, LINES - 4, 1, C_HELPDESCR);

  for (z = hoffset - 1, y = 0; y < 3; y++)
  {
    if ((l = slen(keys[++z]) + slen(func[z])) > max_l) max_l = l;
    if ((l = slen(keys[++z]) + slen(func[z])) > max_m) max_m = l;
    if ((l = slen(keys[++z]) + slen(func[z])) > max_r) max_r = l;
  }

  midpos = interface->midpos(2, max_l, max_m, max_r);
  endpos = interface->endpos(2, max_r);

  menu->attrib(C_HELPKEYS);

  for (z = hoffset - 1, y = 0; y < 3; y++)
  {
    if (keys[++z]) menu->putstring(y, 1, keys[z]);
    if (keys[++z]) menu->putstring(y, midpos, keys[z]);
    if (keys[++z]) menu->putstring(y, endpos, keys[z]);
  }

  menu->attrib(C_HELPDESCR);

  for (z = hoffset - 1, y = 0; y < 3; y++)
  {
    if (func[++z]) menu->putstring(y, 1 + strlen(keys[z]), func[z]);
    if (func[++z]) menu->putstring(y, midpos + strlen(keys[z]), func[z]);
    if (func[++z]) menu->putstring(y, endpos + strlen(keys[z]), func[z]);
  }
}


void HelpWindow::h_servicelist ()
{
  static const char *keys[] =
  {
    // special treatment for index 6
    "A", "F", NULL,
    NULL, "P", NULL,
    NULL, "R", "Q"
  },
  *func[] =
  {
    // special treatment for index 6
    ": Addressbook", ": Files/databases", NULL,
    NULL, ": Packets", NULL,
    NULL, ": Replies", ": Quit " BM_NAME
  };

  // special treatment for index 6
  if (bm.resourceObject->isYes(OmitDemoService)) keys[6] = func[6] = NULL;
  else
  {
    keys[6] = "D";
    func[6] = ": Demo";
  }

  newHelpMenu(keys, func);
  menu->delay_update();
}


void HelpWindow::h_filelist ()
{
  static const char *keys[] =
  {
    "A", "R", "!",
    "K", NULL, NULL,
    "O", NULL, "Q"
  },
  *func[] =
  {
    ": Addressbook", ": Rename file", ": refresh",
    ": Kill file", NULL, NULL,
    ": change sort Order", NULL, ": Back to service(s)"
  };

  newHelpMenu(keys, func);
  menu->delay_update();
}


void HelpWindow::h_filedblist ()
{
  static const char *keys[] =
  {
    "A", NULL, NULL,
    NULL, NULL, NULL,
    NULL, NULL, "Q"
  },
  *func[] =
  {
    ": Addressbook", NULL, NULL,
    NULL, NULL, NULL,
    NULL, NULL, ": Back to service list"
  };

  newHelpMenu(keys, func);
  menu->delay_update();
}


void HelpWindow::h_replymgrlist ()
{
  static const char *keys[] =
  {
    "A", NULL, NULL,
    "K", NULL, NULL,
    "O", NULL, "Q"
  },
  *func[] =
  {
    ": Addressbook", NULL, NULL,
    ": Kill file", NULL, NULL,
    ": change sort Order", NULL, ": Back to service list"
  };

  newHelpMenu(keys, func);
  menu->delay_update();
}


void HelpWindow::h_arealist ()
{
  static const char *keys[] =
  {
    // special treatment for index 2
    "A", "I", NULL,
    "B", "L", ">",
    "E", "O", "Q",

    // special treatment for index 12
    "F", NULL, "=",
    NULL, NULL, ">",
    NULL, NULL, "Q"
  },
  *func[] =
  {
    // special treatment for index 2
    ": Addressbook", ": toggle area Info", NULL,
    ": Bulletins", ": Long/short area list", ": more...",
    ": Enter letter in area", ": Offline configuration", ": Back to service(s)",

    // special treatment for index 12
    ": message status Flags", NULL, ": mark/unmark area",
    NULL, NULL, ": back...",
    NULL, NULL, ": Back to service(s)"
  };

  // special treatment for index 2
  if (bm.driverList->canReply() && interface->isUnsaved())
  {
    keys[2] = "!";
    func[2] = save_txt;
  }
  else keys[2] = func[2] = NULL;

  // special treatment for index 12
  if (bm.resourceObject->isYes(SkipLetterList))
  {
    keys[12] = "S";
    func[12] = ": Show letter list";
  }
  else keys[12] = func[12] = NULL;

  newHelpMenu(keys, func);
  menu->delay_update();
}


void HelpWindow::h_offlineconf ()
{
  static const char *keys[] =
  {
    NULL, NULL, NULL,
    "!", "+", "",
    "*", "-", "Q"
  },
  *func[] =
  {
    NULL, NULL, NULL,
    " means: forced", " means: added", "[Space] to toggle",
    " means: subscribed", " means: dropped", ": Back to area list"
  };

  newHelpMenu(keys, func);
  menu->delay_update();
}


void HelpWindow::h_letterlist ()
{
  static const char *keys[] =
  {
    // special treatment for indices 2, 4 and 7
    "A", "L", NULL,
    "B", NULL, ">",
    "E", NULL, "Q",

    // special treatment for index 11
    "P", NULL, NULL,
    "S", "+", ">",
    NULL, "-", "Q"
  },
  *func[] =
  {
    // special treatment for indices 2, 4 and 7
    ": Addressbook", ": List all/unread", NULL,
    ": filter in Bodies", NULL, ": more...",
    ": Enter letter in area", NULL, ": Back to area list",

    // special treatment for index 11
    ": Print letters", NULL, NULL,
    ": Save letters", ": scroll subject right", ": back...",
    NULL, ": scroll subject left", ": Back to area list"
  },
  *repkeys[] =
  {
    // special treatment for index 5
    "A", "E", "S",
    "B", "K", NULL,
    "D", "P", "Q"
  },
  *repfunc[] =
  {
    // special treatment for index 5
    ": Addressbook", ": Edit letter", ": Save letters",
    ": filter in Bodies", ": Kill letter", NULL,
    ": move to Different area", ": Print letters", ": Back to area list"
  };

  // special treatment for index 2
  if (bm.areaList->supportedMSF() & MSF_READ)
  {
    keys[2] = "U";
    func[2] = ": Unread/read toggle";
  }
  else keys[2] = func[2] = NULL;

  // special treatment for index 4
  if (bm.areaList->supportedMSF() & MSF_MARKED)
  {
    keys[4] = "M";
    func[4] = ": Mark/unmark letter";
  }
  else keys[4] = func[4] = NULL;

  // special treatment for index 7
  if (bm.areaList->isCollection()) keys[7] = func[7] = NULL;
  else
  {
    keys[7] = "O";
    func[7] = ": change sort Order";
  }

  // special treatment for indices 11 and 5
  if (bm.driverList->canReply() && interface->isUnsaved())
  {
    keys[11] = repkeys[5] = "!";
    func[11] = repfunc[5] = save_txt;
  }
  else keys[11] = func[11] = repkeys[5] = repfunc[5] = NULL;

  if (bm.areaList->isReplyArea())
  {
    Reset();
    newHelpMenu(repkeys, repfunc);
  }
  else newHelpMenu(keys, func);

  menu->delay_update();
}


void HelpWindow::h_general ()
{
  int y = 1;

  static const char *item[] =
  {
    "",
    "",
    search_txt,
    fndnxt_txt,
    " |: Filter",
    "",
    " F1, ?: Help",
    "",
    " Alt-D: Invoke shell",
    " Alt-P: Call user program",
    " Alt-Q: Back to service list",
    " Alt-X: Exit immediately",
    NULL
  };

  win = new ShadowedWin(14, 62, (LINES - 14) >> 1, (COLS - 62) >> 1,
                        C_HELPBORDER);

  win->putstring(y, 2, "Keys working throughout the program:");
  win->attrib(C_HELPTEXT);

  for (int i = 0; item[i]; i++) win->putstring(y++, 2, item[i]);

  y = 3;
  win->putstring(y++, 32, up_txt);
  win->putstring(y++, 32, down_txt);
  win->putstring(y++, 32, pgup_txt);
  win->putstring(y++, 32, pgdn_txt);
  win->putstring(y++, 32, home_txt);
  win->putstring(y++, 32, end_txt);
  y++;
  win->putstring(y++, 32, " Enter: Select");
  win->putstring(y++, 32, "   Esc: Cancel");
  win->putstring(y++, 32, "     Q: Back to last window");
}


void HelpWindow::h_letter ()
{
  int i, y = 1;

  static const char *common1[] =
  {
    " #: rot13 decryption                         :: change clock mode",
    " =: quoted-printable decoding",           // !: save replies now
    " A: Addressbook",
    " C: toggle Character set translation",
    NULL
  },
  *common2[] =
  {
    " P: Print letter",
    " S: Save letter",
    " V: ANSI Viewer",
    " X: toggle kludge line display",
    " Q: Back to letter list",
    NULL
  },
  *regular[] =
  {
    " D: reply to Different area",
    " E: Enter letter to area",
    " F: Forward letter to area",
    " J: Jump to replies",
    " M: Mark/unmark letter",
    " N: Netmail reply",
    " O: reply to Original sender",
    " R: Reply to letter",
    " T: Tagline management",
    "^T: get Tagline from letter",
    " U: Unread/read toggle",
    NULL
  },
  *reply[] =
  {
    "",
    "",
    " D: move to Different area",
    " E: Edit letter header",
    "^E: Edit letter body",
    " J: return from Jump to replies",
    " K: Kill letter",
    "",
    "",
    NULL
  },
  *navigation[] =
  {
    up_txt,
    down_txt,
    pgup_txt,
    pgdn_txt,
    home_txt,
    end_txt,
    "",
    " Right: Next letter",
    "  Left: Previous letter",
    " Enter: Next letter",
    "",
    " Space: Page through area",
    "   Tab: Next subject",
    NULL
  };

  bool isReplyArea = bm.areaList->isReplyArea();

  // 9 = items in reply[], 11 = items in regular[]
  // + 9 = items in common1[] and common2[]
  int l = (isReplyArea ? 9 : 11) + 9;

  win = new ShadowedWin(l + 2, 70, (LINES - l - 2) >> 1, (COLS - 70) >> 1,
                        C_HELPBORDER);

  win->attrib(C_HELPTEXT);

  for (i = 0; common1[i]; i++) win->putstring(y++, 2, common1[i]);
  const char **specific = (isReplyArea ? reply : regular);
  for (i = 0; specific[i]; i++) win->putstring(y++, 2, specific[i]);
  for (i = 0; common2[i]; i++) win->putstring(y++, 2, common2[i]);

  // 13 = items in navigation[]
  y = l - 13;
  for (i = 0; navigation[i]; i++) win->putstring(++y, 42, navigation[i]);

  y = (isReplyArea ? 3 : 4);
  win->putstring(y++, 46, search_txt);
  win->putstring(y++, 46, fndnxt_txt);

  if (bm.driverList->canReply() && interface->isUnsaved())
  {
    win->putstring(2, 47, "!");
    win->putstring(2, 48, save_txt);
  }
}


void HelpWindow::h_ansiview ()
{
  int y = 1;

  static const char *item[] =
  {
    search_txt,
    fndnxt_txt,
    "",
    " C: toggle Character set translation",
    " V: animate",
    "",
    up_txt,
    down_txt,
    pgup_txt,
    pgdn_txt,
    home_txt,
    end_txt,
    "",
    "",
    " Right: Next bulletin",
    "   Left: Previous bulletin",
    " Enter: Next bulletin",
    "",
    " Q: Back to letter/bulletins",
    NULL
  };

  win = new ShadowedWin(21, 41, (LINES - 21) >> 1, (COLS - 41) >> 1,
                        C_HELPBORDER, NULL, 0, 3, false);

  win->putstring(14, 2, "Only when viewing bulletins:");
  win->attrib(C_HELPTEXT);

  for (int i = 0; item[i]; i++)
    win->putstring(y++, 2, (item[i][0] == ' ' &&
                            item[i][1] == ' ' ? item[i] + 1: item[i]));
}


void HelpWindow::help (statetype state)
{
  bool ll_help = ((state == letterlist) &&
                  bm.resourceObject->isYes(FullsizeLetterList));

  switch (state)
  {
    case letter:
      h_letter();
      break;

    case ansiview:
      h_ansiview();
      break;

    default:
      h_general();
  }

  if (ll_help) h_letterlist();

  win->update();
  (void) win->inkey();
  delete win;

  if (ll_help) delete menu;
}


void HelpWindow::MakeActive ()
{
  switch (interface->getstate())
  {
    case servicelist:
      h_servicelist();
      break;

    case packetlist:
    case mboxlist:
      h_filelist();
      break;

    case filedblist:
      h_filedblist();
      break;

    case replymgrlist:
      h_replymgrlist();
      break;

    case arealist:
      h_arealist();
      break;

    case letterlist:
      if (!bm.resourceObject->isYes(FullsizeLetterList)) h_letterlist();
      break;

    default:
      ;
  }
}


void HelpWindow::Delete ()
{
  switch (interface->getstate())
  {
    case servicelist:
    case packetlist:
    case filedblist:
    case mboxlist:
    case replymgrlist:
    case arealist:
      delete menu;
      break;

    case letterlist:
      if (!bm.resourceObject->isYes(FullsizeLetterList)) delete menu;
      break;

    default:
      ;
  }
}


void HelpWindow::exchangeActive (statetype state)
{
  switch (state)
  {
    case offlineconflist:
      delete menu;
      h_offlineconf();
      break;

    default:
      ;
  }
}


void HelpWindow::Reset ()
{
  hoffset = 0;
}


void HelpWindow::More ()
{
  hoffset = (hoffset ? 0 : 9);
  Delete();
  MakeActive();
}
