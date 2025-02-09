/*
 * blueMail offline mail reader
 * error-reporting class

 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2000 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef ERROR_H
#define ERROR_H


#include "mysystem.h"


void fatalError(const char *);
void memError();


class Error
{
  private:
    char origdir[MYMAXPATH];

  public:
    Error();
    ~Error();
    const char *getOrigDir() const;
};


#endif
