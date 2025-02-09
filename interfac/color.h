/*
 * blueMail offline mail reader
 * color handling

 Copyright (c) 1996 John Zero <john@graphisoft.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef COLOR_H
#define COLOR_H


#include "wincurs.h"


// this is how we internally represent a color (pair)
// (blueMail internal color type as used in bmcolors)
#define COL(f, b) (((f) << 3) + (b))


#define C_MBORDER        color[0]      // main screen border
#define C_MBACK          color[1]      // main screen background
#define C_MSEPBOTT       color[2]      // main screen separator (bottom)
#define C_WELCBORDER     color[3]      // welcome border
#define C_WELCHEADER     color[4]      // welcome header
#define C_WELCTEXT       color[5]      // welcome text
#define C_HELPBORDER     color[6]      // help border
#define C_HELPTEXT       color[7]      // help text
#define C_HELPKEYS       color[8]      // help keys
#define C_HELPDESCR      color[9]      // help descriptions
#define C_SLBORDER       color[10]     // service list border
#define C_SLTOPTEXT      color[11]     // service list top border text
#define C_SLIST          color[12]     // service list
#define C_FLBORDER       color[13]     // file list border
#define C_FLHEADER       color[14]     // file list header
#define C_FLIST          color[15]     // file list
#define C_REPMLBORDER    color[16]     // reply manager list border
#define C_REPMLHEADER    color[17]     // reply manager list header
#define C_REPMLIST       color[18]     // reply manager list
#define C_REPMLPACKET    color[19]     // reply manager list packet
#define C_ALBORDER       color[20]     // area list border
#define C_ALTOPTEXT      color[21]     // area list top border text
#define C_ALHEADER       color[22]     // area list header
#define C_ALISTUNRD      color[23]     // area list Unread
#define C_ALISTREAD      color[24]     // area list Read
#define C_ALREPLY        color[25]     // area list reply area
#define C_ALINFODESCR    color[26]     // area list info win description
#define C_ALINFOTEXT     color[27]     // area list info win text
#define C_BULLLBORDER    color[28]     // bulletin list border
#define C_BULLLTOPTEXT   color[29]     // bulletin list top border text
#define C_BULLLIST       color[30]     // bulletin list
#define C_OFFCFLBORDER   color[31]     // offline config list border
#define C_OFFCFLTOPTEXT  color[32]     // offline config list top border text
#define C_OFFCFLIST      color[33]     // offline config list
#define C_OFFCFLISTADD   color[34]     // offline config list (added)
#define C_OFFCFLISTDROP  color[35]     // offline config list (dropped)
#define C_LALBORDER      color[36]     // little area border
#define C_LALTOPTEXT     color[37]     // little area top border text
#define C_LALIST         color[38]     // little area list
#define C_LALISTRO       color[39]     // little area list read-only
#define C_LLBORDER       color[40]     // letter list border
#define C_LLTOPTEXT      color[41]     // letter list top border text
#define C_LLAREA         color[42]     // letter list areaname
#define C_LLHEADER       color[43]     // letter list header
#define C_LLISTUNRD      color[44]     // letter list Unread
#define C_LLISTREAD      color[45]     // letter list Read
#define C_LLFROMUSER     color[46]     // letter list from user
#define C_LLTOUSER       color[47]     // letter list to user
#define C_LHBORDER       color[48]     // letter header border
#define C_LHCLOCK        color[49]     // letter header clock
#define C_LHMSGNUM       color[50]     // letter header MSGNUM
#define C_LHTEXT         color[51]     // letter header (text)
#define C_LHFROM         color[52]     // letter header FROM
#define C_LHTO           color[53]     // letter header TO
#define C_LHSUBJ         color[54]     // letter header SUBJECT
#define C_LHDATE         color[55]     // letter header DATE
#define C_LHFLAGS        color[56]     // letter header flags
#define C_LHFLAGSHI      color[57]     // letter header flags high
#define C_LTEXT          color[58]     // letter text
#define C_LQTEXT         color[59]     // letter quoted text
#define C_LTAGLINE       color[60]     // letter tagline
#define C_LSIGNATURE     color[61]     // letter signature
#define C_LTEARLINE      color[62]     // letter tearline
#define C_LORIGIN        color[63]     // letter origin
#define C_LKLUDGE        color[64]     // letter kludge line
#define C_LBOTTLINE      color[65]     // letter bottom (status) line
#define C_ANSIVHEADER    color[66]     // ANSI viewer header
#define C_ANSIVIEW       color[67]     // ANSI viewer
#define C_REPBOXBORDER   color[68]     // reply box border
#define C_REPBOXDESCR    color[69]     // reply box description
#define C_REPBOXTEXT     color[70]     // reply box text
#define C_REPBOXINP      color[71]     // reply box input
#define C_REPBOXHELP     color[72]     // reply box help text
#define C_FILEBOXBORDER  color[73]     // file box border
#define C_FILEBOXHEADER  color[74]     // file box header
#define C_FILEBOXINP     color[75]     // file box input
#define C_QUERYBOXBORDER color[76]     // query box border
#define C_QUERYBOXHEADER color[77]     // query box header
#define C_QUERYBOXINP    color[78]     // query box input
#define C_ADDRBKBORDER   color[79]     // addressbook border
#define C_ADDRBKTOPTEXT  color[80]     // addressbook top border text
#define C_ADDRBKHEADER   color[81]     // addressbook header
#define C_ADDRBKLIST     color[82]     // addressbook list
#define C_ADDRBKINP      color[83]     // addressbook input
#define C_ADDRBKKEYS     color[84]     // addressbook (help) keys
#define C_ADDRBKDESCR    color[85]     // addressbook (help) description
#define C_TAGBOXBORDER   color[86]     // tagline box border
#define C_TAGBOXTTEXT    color[87]     // tagline box top border text
#define C_TAGBOXLIST     color[88]     // tagline box list
#define C_TAGBOXINP      color[89]     // tagline box input
#define C_TAGBOXKEYS     color[90]     // tagline box keys
#define C_TAGBOXDESCR    color[91]     // tagline box keys description
#define C_WARNTEXT       color[92]     // warning text
#define C_WARNKEYS       color[93]     // warning text highlighted keys
#define C_INFOTEXT       color[94]     // info text
#define C_SYSCALLHEAD    color[95]     // system call header
#define C_SEARCHRESULT   color[96]     // search results
#define C_SHADOW         color[97]     // shadow for all windows


extern const chtype *color;


class Color
{
  private:
    static chtype bmcolors[];

    chtype colors(const char *, const chtype);

  public:
    Color();
    chtype getBackground() const;
    void convert(UserInterface *);
};


#endif
