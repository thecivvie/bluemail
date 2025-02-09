/*
 * blueMail offline mail reader
 * character conversion part of interface (ISO 8859-1 <-> IBM codepage 437)

 Copyright (c) 1996 Peter Karlsson <dat95pkn@idt.mdh.se>,
                    Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include "interfac.h"
#include "../common/auxil.h"


/* Original tables by Peter Karlsson, modified by William McBrine after
   DOSEmu's video/terminal.h, by Mark D. Rejhon.
   Minor fixes by Ingo Brueckl. */

const char *Interface::dos2isotab =
  "\307\374\351\342\344\340\345\347\352\353\350\357\356\354\304\305"
  "\311\346\306\364\366\362\373\371\377\326\334\242\243\245Pf"
  "\341\355\363\372\361\321\252\272\277\255\254\275\274\241\253\273"
  ":%&|{{{..{I.'''."
  "``+}-+}}`.**}=**"
  "+*+``..**'.#_][~"
  "a\337\254\266{\363\265t\330\364\326\363o\370En"
  "=\261><()\367=\260\267\267%\140\262\376 ";

const char *Interface::iso2dostab =
  "\356\250'f\"_\250\250^\250S<\250\250Z\250"
  "\250\140\047\"\"\371--~Ts>\250\250zY"
  " \255\233\234\356\235|\025\"C\246\256\252-R-"
  "\370\361\3753\'\346\024\372,1\247\257\254\253/\250"
  "AAAA\216\217\222\200E\220EEIIII"
  "D\245OOOO\231x\350UUU\232Y \341"
  "\205\240\203a\204\206\221\207\212\202\210\211\215\241\214\213"
  " \244\225\242\223o\224\366\355\227\243\226\201y \230";

// some people say ISO 8859-1 but mean Windows-1252
const char *Interface::win2isotab =
  "\244\277'f\"_\277\277^\277S<\277\277Z\277"
  "\277\140\264\"\"\267--~Ts>\277\277zY";


bool Interface::isoToggleSet (bool value)
{
  bool old = isoToggle;
  isoToggle = value;
  return old;
}


char *Interface::charconv (char *buf, convtype cdir)
{
  char *p;
  unsigned char u;
  const char *table = (cdir == CC_437toISO ? dos2isotab
                                           : (cdir == CC_ISOto437 ? iso2dostab
                                                                  : win2isotab));

  for (p = buf; *p; p++)
    if (((u = *p) & 0x80) && (cdir != CC_WINtoISO || u < 0xA0))
      *p = table[u & 0x7F];

  return buf;
}


char *Interface::charconv_in (char *buf, bool isISO)
{
  if (isoDisplay ^ (isISO ^ isoToggle))
    return charconv(buf, isoDisplay ? CC_437toISO : CC_ISOto437);
  else if (isoDisplay && (isISO ^ isoToggle))
    return charconv(buf, CC_WINtoISO);
  else return buf;
}


char *Interface::charconv_out (char *buf, bool isISO)
{
  if (isoDisplay ^ isISO)
    return charconv(buf, isoDisplay ? CC_ISOto437 : CC_437toISO);
  else return buf;
}


char Interface::charconv_in (char c, bool isISO)
{
  static char buf[] = {'\0', '\0'};

  *buf = c;
  charconv_in(buf, isISO);

  return *buf;
}


char Interface::charconv_out (char c, bool isISO)
{
  static char buf[] = {'\0', '\0'};

  *buf = c;
  charconv_out(buf, isISO);

  return *buf;
}


// same as charconv_in (char *buf, bool) above, but buf isn't changed
char *Interface::charconv_in (bool isISO, const char *buf)
{
  delete[] charconv_buf;
  charconv_buf = strdupplus(buf);

  return charconv_in(charconv_buf, isISO);
}


// alternative charconv_in (used by the ANSI viewer)
chtype Interface::charconv_in (bool isISO, char c)
{
  unsigned char u = c;

  if (isoDisplay && !(isISO ^ isoToggle) && (u < ' ' || u >= 0x80))
    return (u | A_ALTCHARSET);
  else return (chtype) (unsigned char) charconv_in(c, isISO);
}
