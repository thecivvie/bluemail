/*
 * blueMail offline mail reader
 * file_header and file_list

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "bmail.h"
#include "../common/auxil.h"
#include "../common/error.h"


/*
   file header
*/

file_header::file_header (char *name, time_t date, off_t size)
{
  this->name = strdupplus(name);
  id = crc32(name);
  this->date = date;
  this->size = size;
  next = NULL;
}


file_header::~file_header ()
{
  delete[] name;
}


inline const char *file_header::getName () const
{
  return name;
}


inline uint32_t file_header::getID () const
{
  return id;
}


inline time_t file_header::getDate () const
{
  return date;
}


inline off_t file_header::getSize () const
{
  return size;
}


/*
   file list
*/

file_list::file_list (const char *dirname)
{
  DIR *dir;
  struct dirent *entry;
  struct stat fstat;

  if (!(dir = opendir(dirname))) fatalError("Could not open directory.");

  this->dirname = strdupplus(dirname);
  mychdir(dirname);
  noOfFiles = 0;

  file_header anchor((char *) "", 0, 0);
  file_header *fh = &anchor;

  while ((entry = readdir(dir)))
    if (*entry->d_name != '.')
      if (access(entry->d_name, R_OK | W_OK) == 0)
        if (stat(entry->d_name, &fstat) == 0)
          if (!S_ISDIR(fstat.st_mode))
          {
            fh->next = new file_header(entry->d_name, fstat.st_mtime,
                                                      fstat.st_size);
            fh = fh->next;
            noOfFiles++;
          }

  files = new file_header *[noOfFiles];

  int i = 0;
  fh = anchor.next;

  while (fh)
  {
    files[i++] = fh;
    fh = fh->next;
  }

  closedir(dir);
}


file_list::~file_list ()
{
  while (noOfFiles) delete files[--noOfFiles];
  delete[] files;
  delete[] dirname;
}


const char *file_list::getDir () const
{
  return dirname;
}


void file_list::sort (int sorttype)
{
  if (noOfFiles)
    qsort(files, noOfFiles, sizeof(file_header *),
          (sorttype ? (sorttype == FL_SORT_BY_DATE ? fsortbydate
                                                   : fsortbynewest)
                    : fsortbyname));

}


int file_list::getNoOfFiles () const
{
  return noOfFiles;
}


void file_list::gotoFile (int fileNo)
{
  // with -1 it can be used with getNext():
  if (fileNo >= -1 && fileNo < noOfFiles) activeFile = fileNo;
}


const char *file_list::getName () const
{
  return files[activeFile]->getName();
}


uint32_t file_list::getID () const
{
  return files[activeFile]->getID();
}


time_t file_list::getDate () const
{
  return files[activeFile]->getDate();
}


off_t file_list::getSize () const
{
  return files[activeFile]->getSize();
}


const char *file_list::exists (const char *pfname)
{
  gotoFile(-1);
  return getNext(pfname);
}


const char *file_list::getNext (const char *pfname)
{
  int i, flen, plen;
  const char *p, *f;
  bool ext;

  if (pfname)
  {
    plen = strlen(pfname);
    ext = (*pfname == '.');

    for (i = activeFile + 1; i < noOfFiles; i++)
    {
      f = files[i]->getName();

      if (ext)
      {
        flen = strlen(f);
        if (flen >= plen)
        {
          p = f + flen - plen;
          if (strcasecmp(p, pfname) == 0)
          {
            activeFile = i;
            return f;
          }
        }
      }

      else
      {
        plen = wildcard(pfname);
        bool found = (plen == -1 ? strcasecmp(f, pfname) == 0
                                 : strncasecmp(f, pfname, plen) == 0);
        if (found)
        {
          activeFile = i;
          return f;
        }
      }
    }
  }
  return NULL;
}


FILE *file_list::ftryopen (const char *pfname, const char *mode)
{
  const char *file;
  char fname[MYMAXPATH];

  if ((file = exists(pfname)))
  {
    mkfname(fname, dirname, file);
    return fxopen(fname, mode);
  }
  else return NULL;
}


void file_list::add (const char **list, const char *pfname, int *count)
{
  const char *file;
  int i;

  gotoFile(-1);
  while ((file = getNext(pfname)))
  {
    for (i = 0; i < *count; i++)
      if (list[i] == file) break;

    if (i == *count) list[(*count)++] = file;
  }
}


int file_list::nextNumExt (const char *fname)
{
  int result = -1;
  const char *file, *fwild = ext(fname, "*");

  gotoFile(-1);
  while ((file = getNext(fwild)))
  {
    int num = getNumExt(file);
    if (num > result) result = num;
  }

  if (result == 999) return -1;
  else return ++result;
}


bool file_list::changeName (const char *fname)
{
  mychdir(dirname);
  return (myrename(getName(), fname) == 0);
}


/*
   file qsort compare functions
*/

int fsortbyname (const void *a, const void *b)
{
  const char *fa = (*(file_header **) a)->getName();
  const char *fb = (*(file_header **) b)->getName();
  return strcoll(fa, fb);
}


int fsortbydate (const void *a, const void *b)
{
  time_t ta = (*(file_header **) a)->getDate();
  time_t tb = (*(file_header **) b)->getDate();
  if (ta < tb) return -1;
  if (ta > tb) return 1;
  return 0;
}


int fsortbynewest (const void *a, const void *b)
{
  return fsortbydate(b, a);
}
