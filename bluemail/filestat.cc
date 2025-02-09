/*
 * blueMail offline mail reader
 * file statistics

 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include "bmail.h"


file_stat::file_stat (bmail *bm)
{
  FILE *statfile;
  filestat stat;

  ro = bm->resourceObject;
  anchor = last = NULL;

  if ((statfile = fopen(ro->get(statisticsFile), "rb")))
  {
    while (fread(&stat, sizeof(filestat), 1, statfile)) newStat(&stat);
    fclose(statfile);
  }
}


file_stat::~file_stat ()
{
  FILE *statfile;
  filestat *next, *i = anchor;

  if (anchor && (statfile = fopen(ro->get(statisticsFile), "wb")))
  {
    while (i)
    {
      next = i->next;

      if (i->used)
      {
        i->used = false;
        fwrite(i, sizeof(filestat), 1, statfile);
      }

      delete i;
      i = next;
    }
    fclose(statfile);
  }
}


void file_stat::newStat (filestat *stat)
{
  filestat *nstat = new filestat;

  if (!anchor) anchor = nstat;
  stat->next = NULL;
  *nstat = *stat;
  if (last) last->next = nstat;
  last = nstat;
}


void file_stat::addStat (uint32_t id, time_t date, int total, int unread)
{
  filestat *i = anchor;

  while (i)
  {
    if (i->fileID == id && i->fileDate == date)
    {
      i->msgsTotal = total;
      i->msgsUnread = unread;
      i->used = true;
      return;
    }
    i = i->next;
  }

  filestat stat;
  stat.fileID = id;
  stat.fileDate = date;
  stat.msgsTotal = total;
  stat.msgsUnread = unread;
  stat.used = true;
  newStat(&stat);
}


bool file_stat::getStat (uint32_t id, time_t date, int *total, int *unread) const
{
  filestat *i = anchor;

  while (i)
  {
    if (i->fileID == id && i->fileDate == date)
    {
      *total = i->msgsTotal;
      *unread = i->msgsUnread;
      i->used = true;
      return true;
    }
    i = i->next;
  }

  return false;
}
