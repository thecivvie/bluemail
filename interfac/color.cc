/*
 * blueMail offline mail reader
 * color handling

 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "interfac.h"
#include "../common/auxil.h"


const chtype *color = NULL;


// blueMail default colors
chtype Color::bmcolors[] =
{
  /*  0 */ COL(COLOR_BLUE, COLOR_BLACK) | A_BOLD,
  /*  1 */ COL(COLOR_WHITE, COLOR_BLACK),
  /*  2 */ COL(COLOR_MAGENTA, COLOR_BLACK),
  /*  3 */ COL(COLOR_BLUE, COLOR_BLACK) | A_BOLD,
  /*  4 */ COL(COLOR_YELLOW, COLOR_BLACK) | A_BOLD,
  /*  5 */ COL(COLOR_CYAN, COLOR_BLACK) | A_BOLD,
  /*  6 */ COL(COLOR_WHITE, COLOR_BLUE) | A_BOLD,
  /*  7 */ COL(COLOR_YELLOW, COLOR_BLUE) | A_BOLD,
  /*  8 */ COL(COLOR_YELLOW, COLOR_BLACK) | A_BOLD,
  /*  9 */ COL(COLOR_WHITE, COLOR_BLACK) | A_BOLD,
  /* 10 */ COL(COLOR_CYAN, COLOR_BLUE) | A_BOLD,
  /* 11 */ COL(COLOR_GREEN, COLOR_BLUE) | A_BOLD,
  /* 12 */ COL(COLOR_WHITE, COLOR_BLUE) | A_BOLD,
  /* 13 */ COL(COLOR_YELLOW, COLOR_BLUE) | A_BOLD,
  /* 14 */ COL(COLOR_GREEN, COLOR_BLUE) | A_BOLD,
  /* 15 */ COL(COLOR_CYAN, COLOR_BLUE) | A_BOLD,
  /* 16 */ COL(COLOR_YELLOW, COLOR_BLUE) | A_BOLD,
  /* 17 */ COL(COLOR_GREEN, COLOR_BLUE) | A_BOLD,
  /* 18 */ COL(COLOR_CYAN, COLOR_BLUE) | A_BOLD,
  /* 19 */ COL(COLOR_MAGENTA, COLOR_BLUE) | A_BOLD,
  /* 20 */ COL(COLOR_BLUE, COLOR_BLACK) | A_BOLD,
  /* 21 */ COL(COLOR_WHITE, COLOR_BLACK) | A_BOLD,
  /* 22 */ COL(COLOR_GREEN, COLOR_BLACK) | A_BOLD,
  /* 23 */ COL(COLOR_CYAN, COLOR_BLACK) | A_BOLD,
  /* 24 */ COL(COLOR_BLUE, COLOR_BLACK),
  /* 25 */ COL(COLOR_MAGENTA, COLOR_BLACK),
  /* 26 */ COL(COLOR_YELLOW, COLOR_BLACK) | A_BOLD,
  /* 27 */ COL(COLOR_WHITE, COLOR_BLACK) | A_BOLD,
  /* 28 */ COL(COLOR_CYAN, COLOR_BLUE) | A_BOLD,
  /* 29 */ COL(COLOR_GREEN, COLOR_BLUE) | A_BOLD,
  /* 30 */ COL(COLOR_WHITE, COLOR_BLUE) | A_BOLD,
  /* 31 */ COL(COLOR_CYAN, COLOR_BLUE) | A_BOLD,
  /* 32 */ COL(COLOR_YELLOW, COLOR_BLUE) | A_BOLD,
  /* 33 */ COL(COLOR_WHITE, COLOR_BLUE) | A_BOLD,
  /* 34 */ COL(COLOR_GREEN, COLOR_BLUE) | A_BOLD,
  /* 35 */ COL(COLOR_RED, COLOR_BLUE) | A_BOLD,
  /* 36 */ COL(COLOR_CYAN, COLOR_BLUE) | A_BOLD,
  /* 37 */ COL(COLOR_YELLOW, COLOR_BLUE) | A_BOLD,
  /* 38 */ COL(COLOR_WHITE, COLOR_BLUE) | A_BOLD,
  /* 39 */ COL(COLOR_WHITE, COLOR_BLUE),
  /* 40 */ COL(COLOR_YELLOW, COLOR_BLUE) | A_BOLD,
  /* 41 */ COL(COLOR_WHITE, COLOR_BLUE) | A_BOLD,
  /* 42 */ COL(COLOR_YELLOW, COLOR_BLUE) | A_BOLD,
  /* 43 */ COL(COLOR_GREEN, COLOR_BLUE) | A_BOLD,
  /* 44 */ COL(COLOR_CYAN, COLOR_BLUE) | A_BOLD,
  /* 45 */ COL(COLOR_WHITE, COLOR_BLUE),
  /* 46 */ COL(COLOR_YELLOW, COLOR_BLUE) | A_BOLD,
  /* 47 */ COL(COLOR_MAGENTA, COLOR_BLUE) | A_BOLD,
  /* 48 */ COL(COLOR_BLUE, COLOR_CYAN),
  /* 49 */ COL(COLOR_WHITE, COLOR_CYAN) | A_BOLD,
  /* 50 */ COL(COLOR_GREEN, COLOR_CYAN) | A_BOLD,
  /* 51 */ COL(COLOR_WHITE, COLOR_CYAN) | A_BOLD,
  /* 52 */ COL(COLOR_BLUE, COLOR_CYAN),
  /* 53 */ COL(COLOR_BLUE, COLOR_CYAN),
  /* 54 */ COL(COLOR_BLACK, COLOR_CYAN),
  /* 55 */ COL(COLOR_GREEN, COLOR_CYAN) | A_BOLD,
  /* 56 */ COL(COLOR_WHITE, COLOR_CYAN),
  /* 57 */ COL(COLOR_YELLOW, COLOR_CYAN) | A_BOLD,
  /* 58 */ COL(COLOR_WHITE, COLOR_BLACK),
  /* 59 */ COL(COLOR_YELLOW, COLOR_BLACK),
  /* 60 */ COL(COLOR_CYAN, COLOR_BLACK),
  /* 61 */ COL(COLOR_CYAN, COLOR_BLACK),
  /* 62 */ COL(COLOR_BLUE, COLOR_BLACK) | A_BOLD,
  /* 63 */ COL(COLOR_GREEN, COLOR_BLACK) | A_BOLD,
  /* 64 */ COL(COLOR_BLACK, COLOR_BLACK) | A_BOLD,
  /* 65 */ COL(COLOR_YELLOW, COLOR_MAGENTA) | A_BOLD,
  /* 66 */ COL(COLOR_BLACK, COLOR_CYAN),
  /* 67 */ COL(COLOR_WHITE, COLOR_BLACK),
  /* 68 */ COL(COLOR_YELLOW, COLOR_BLUE) | A_BOLD,
  /* 69 */ COL(COLOR_WHITE, COLOR_BLUE) | A_BOLD,
  /* 70 */ COL(COLOR_CYAN, COLOR_BLUE) | A_BOLD,
  /* 71 */ COL(COLOR_BLACK, COLOR_CYAN),
  /* 72 */ COL(COLOR_BLACK, COLOR_WHITE),
  /* 73 */ COL(COLOR_YELLOW, COLOR_BLUE) | A_BOLD,
  /* 74 */ COL(COLOR_WHITE, COLOR_BLUE) | A_BOLD,
  /* 75 */ COL(COLOR_BLACK, COLOR_CYAN),
  /* 76 */ COL(COLOR_YELLOW, COLOR_YELLOW) | A_BOLD,
  /* 77 */ COL(COLOR_WHITE, COLOR_YELLOW) | A_BOLD,
  /* 78 */ COL(COLOR_BLACK, COLOR_WHITE),
  /* 79 */ COL(COLOR_RED, COLOR_BLACK),
  /* 80 */ COL(COLOR_WHITE, COLOR_BLACK) | A_BOLD,
  /* 81 */ COL(COLOR_GREEN, COLOR_BLACK) | A_BOLD,
  /* 82 */ COL(COLOR_BLUE, COLOR_BLACK) | A_BOLD,
  /* 83 */ COL(COLOR_BLACK, COLOR_CYAN),
  /* 84 */ COL(COLOR_YELLOW, COLOR_BLACK) | A_BOLD,
  /* 85 */ COL(COLOR_WHITE, COLOR_BLACK) | A_BOLD,
  /* 86 */ COL(COLOR_RED, COLOR_BLACK),
  /* 87 */ COL(COLOR_WHITE, COLOR_BLACK) | A_BOLD,
  /* 88 */ COL(COLOR_BLUE, COLOR_BLACK) | A_BOLD,
  /* 89 */ COL(COLOR_BLACK, COLOR_CYAN),
  /* 90 */ COL(COLOR_YELLOW, COLOR_BLACK) | A_BOLD,
  /* 91 */ COL(COLOR_WHITE, COLOR_BLACK) | A_BOLD,
  /* 92 */ COL(COLOR_WHITE, COLOR_RED) | A_BOLD,
  /* 93 */ COL(COLOR_YELLOW, COLOR_RED) | A_BOLD,
  /* 94 */ COL(COLOR_CYAN, COLOR_BLUE) | A_BOLD,
  /* 95 */ COL(COLOR_CYAN, COLOR_BLUE) | A_BOLD,
  /* 96 */ COL(COLOR_WHITE, COLOR_RED) | A_BOLD,
  /* 97 */ COL(COLOR_BLACK, COLOR_BLACK) | A_BOLD
};


Color::Color ()
{
  for (int i = colorFIRST; i <= colorLAST; i++)
    if (bm.resourceObject->get(i))
      bmcolors[i - colorFIRST] = colors(bm.resourceObject->get(i),
                                        bmcolors[i - colorFIRST]);
}


chtype Color::getBackground () const
{
  return bmcolors[colorMainBack - colorFIRST];
}


void Color::convert (UserInterface *ui)
{
  for (int i = colorFIRST; i <= colorLAST; i++)
    bmcolors[i - colorFIRST] = ui->color(bmcolors[i - colorFIRST]);

  color = bmcolors;
}


// analyze a <foreground color>,<background color>,<attribute> color string
chtype Color::colors (const char *colorstring, const chtype std)
{
  static struct
  {
    char *color;
    chtype value;
  } colormap[] = {{(char *) "BLACK", COLOR_BLACK},
                  {(char *) "BLUE", COLOR_BLUE},
                  {(char *) "GREEN", COLOR_GREEN},
                  {(char *) "CYAN", COLOR_CYAN},
                  {(char *) "RED", COLOR_RED},
                  {(char *) "MAGENTA", COLOR_MAGENTA},
                  {(char *) "YELLOW", COLOR_YELLOW},
                  {(char *) "WHITE", COLOR_WHITE}};
  char *pos, *part;
  static char result[8];
  int len;
  chtype c[3] = {(std >> 3) & 7, std & 7, std & (chtype) ~COL(7, 7)};

  pos = (char *) colorstring;

  for (int i = 0; i < 3; i++)
    if (*pos)
    {
      pos = lcrop(pos);

      part = pos;
      len = 0;

      while (*pos && (*pos != ','))
      {
        pos++;
        len++;
      }

      if (len > 7) len = 0;
      else strncpy(result, part, len);
      result[len] = '\0';

      switch (i)
      {
        case 0:
        case 1:
          for (int j = 0; j < 8; j++)
            if (strcasecmp(result, colormap[j].color) == 0)
            {
              c[i] = colormap[j].value;
              break;
            }
          break;

        case 2:
          if (strcasecmp(result, "NORMAL") == 0)
          {
            c[i] = A_NORMAL;
            break;
          }
          if (strcasecmp(result, "BOLD") == 0)
          {
            c[i] = A_BOLD;
            break;
          }
          if (strcasecmp(result, "REVERSE") == 0)
          {
#ifdef FIXSTANDOUT
            c[2] = c[0];
            c[0] = c[1];
            c[1] = c[2];
            c[i] = A_NORMAL;
#else
            c[i] = A_REVERSE;
#endif
            break;
          }
      }

      if (*pos == ',') pos++;
    }

  return COL(c[0], c[1]) | c[2];
}
