/*
 * blueMail offline mail reader
 * program to extract files from a collection

 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <stdio.h>
#include <string.h>
#include "bmuncoll.h"
#include "../bluemail/service.h"
#include "auxil.h"


#define PGM "bmuncoll"


void clearDirectory (const char *dirname)
{
  (void) dirname;
}


void usage ()
{
  fprintf(stdout, "Usage: " PGM " OPTION FILE\n\n");
  fprintf(stdout, "Options:\n");
  fprintf(stdout, "  -t        test collection\n");
  fprintf(stdout, "  -x        extract files from collection\n");
}


void error (const char *msg)
{
  fprintf(stderr, PGM ": %s\n", msg);
}


bool
// commonly used routine
#include "uncoll.cc"


int main (int argc, char *argv[])
{
  FILE *collection;
  int files = 0;

  fprintf(stdout, BM_NAME " uncollect " BM_TOP_V "\n\n", BM_MAJOR, BM_MINOR);

  if (argc != 3 ||
      (strcasecmp(argv[1], "-t") != 0 &&
       strcasecmp(argv[1], "-x") != 0))
  {
    usage();
    return 1;
  }

  if (!(collection = fopen(argv[2], "rb")))
  {
    error("cannot open file");
    return 2;
  }

  if (fscanf(collection, COLL_MAGIC "%d:", &files) != 1)
  {
    error("file is not a collection");
    return 3;
  }
  else fprintf(stdout, "%s contains %d file%s\n",
                       argv[2], files, (files == 1 ? "" : "s"));

  fclose(collection);

  if (strcasecmp(argv[1], "-x") == 0)
  {
    if (uncollect(argv[2], 0, ".", true))
      fprintf(stdout, "%d file%s successfully extracted\n",
                      files, (files == 1 ? "" : "s"));
    else
    {
      error("extraction failed");
      return 4;
    }
  }

  return 0;
}
