/*
 * blueMail offline mail reader
 * error-reporting class

 Copyright (c) 1996 Kolossvary Tamas <thomas@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2002 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <new>
// older compilers may not define namespace std
#if __GNUC__ >= 3 && __GNUC_MINOR__ >= 1
  // use of namespace is mandatory
  using std::set_new_handler;
#endif
#include <stdlib.h>
#include <unistd.h>
#include "error.h"
#include "../interfac/interfac.h"


void fatalError (const char *description)
{
  delete interface;
  fprintf(stderr, "\nError: %s\n\n", description);
  exit(EXIT_FAILURE);
}


void memError ()
{
  fatalError("Out of memory.");
}


Error::Error ()
{
  set_new_handler(memError);
  if (!mygetcwd(origdir, sizeof(origdir))) fatalError("Path too long.");
  // the obvious place to call myinit() is main(),
  // but this may be too late, so we call it here
  myinit();
}


Error::~Error ()
{
  mychdir(origdir);
}


const char *Error::getOrigDir () const
{
  return origdir;
}
