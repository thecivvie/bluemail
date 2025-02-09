/*
 * blueMail offline mail reader
 * reader, main_reader, reply_reader (high level parts of reader)

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2001 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include "bmail.h"


/*
   reader (virtual destructor)
*/

reader::~reader()
{
}


/*
   main_reader (for regular areas)
*/

main_reader::main_reader (bmail *bm, main_driver *driver)
{
  this->bm = bm;
  this->driver = driver;

  noOfAreas = driver->getNoOfAreas();
  noOfLetters = new int[noOfAreas];
  msgstat = new int *[noOfAreas];

  for (int a = 0; a < noOfAreas; a++)
  {
    driver->selectArea(a);
    noOfLetters[a] = driver->getNoOfLetters();
    msgstat[a] = new int[noOfLetters[a]];
  }

  msfs = driver->readMSF(msgstat);
}


main_reader::~main_reader ()
{
  while(noOfAreas) delete[] msgstat[--noOfAreas];
  delete[] msgstat;
  delete[] noOfLetters;
}


inline int main_reader::supportedMSF ()
{
  return msfs;
}


void main_reader::setRead (int area, int letter, bool on)
{
  if (on) msgstat[area][letter] |= MSF_READ;
  else msgstat[area][letter] &= ~MSF_READ;
}


bool main_reader::isRead (int area, int letter) const
{
  return (msgstat[area][letter] & MSF_READ) != 0;
}


void main_reader::setStatus (int area, int letter, int stat)
{
  msgstat[area][letter] = stat;
}


int main_reader::getStatus (int area, int letter) const
{
  int status = msgstat[area][letter];
  // see MSF_MARKED in bmail.h for an explanation of this
  return (status & MSF_MARKED ? status | MSF_MARKED : status);
}


int main_reader::getNoOfUnread (int area) const
{
  int count = 0;

  for (int l = 0; l < noOfLetters[area]; l++)
    if ((msgstat[area][l] & MSF_READ) == 0) count++;

  return count;
}


int main_reader::getNoOfPersonal (int area) const
{
  int count = 0;

  for (int l = 0; l < noOfLetters[area]; l++)
    if (msgstat[area][l] & MSF_PERSONAL) count++;

  return count;
}


const char *main_reader::saveMSF ()
{
  return driver->saveMSF(msgstat);
}


/*
   reply_reader (does almost nothing)
*/

reply_reader::reply_reader (bmail *bm, main_driver *driver)
{
  (void) bm;
  (void) driver;
}


reply_reader::~reply_reader ()
{
}


inline int reply_reader::supportedMSF ()
{
  return 0;
}


inline void reply_reader::setRead (int area, int letter, bool on)
{
  (void) area;
  (void) letter;
  (void) on;
}


inline bool reply_reader::isRead (int area, int letter) const
{
  (void) area;
  (void) letter;
  return true;
}


inline void reply_reader::setStatus (int area, int letter, int stat)
{
  (void) area;
  (void) letter;
  (void) stat;
}


inline int reply_reader::getStatus (int area, int letter) const
{
  (void) area;
  (void) letter;
  return MSF_READ;
}


inline int reply_reader::getNoOfUnread (int area) const
{
  (void) area;
  return 0;
}


inline int reply_reader::getNoOfPersonal (int area) const
{
  (void) area;
  return 0;
}


inline const char *reply_reader::saveMSF ()
{
  return NULL;
}
