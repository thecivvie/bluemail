/*
 * blueMail offline mail reader
 * main

 Copyright (c) 1996 Kolossvary Tamas <thomas@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <ctype.h>
#include <stdlib.h>
#include <locale.h>
#include "interfac.h"
#include "../bluemail/driverl.h"
#include "../common/error.h"


Interface *interface = NULL;
Error error;
bmail bm;

#ifdef WITH_CLOCK
time_t starttime;
#endif


int main (void)
{
#ifdef WITH_CLOCK
  starttime = time(NULL);
#endif

  setlocale(LC_COLLATE, "");
  setlocale(LC_CTYPE, "");
  srand((unsigned int) time(NULL));

  const char *ss = bm.resourceObject->get(StartupService);

  interface = new Interface();
  interface->mainwin();
  interface->changestate(servicelist);

  if (ss)
  {
    if (toupper(*ss) == 'A')
    {
#ifdef DRV_MBOX
      interface->services.KeyHandle('F');
      interface->filedbs.KeyHandle(META_S1);
#endif
    }
    else interface->services.KeyHandle(toupper(*ss));
  }

  interface->KeyHandle();

  delete interface;
  return EXIT_SUCCESS;
}
