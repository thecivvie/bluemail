/*
 * blueMail offline mail reader
 * net address

 Copyright (c) 2002 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include "bmail.h"
#include "../common/auxil.h"


net_address::net_address ()
{
  zone = net = node = point = 0;
  otherAddr = NULL;
}


net_address::net_address (net_address &na)
{
  copy(na);
}


net_address::~net_address ()
{
  delete[] otherAddr;
}


void net_address::copy (net_address &na)
{
  zone = na.zone;
  net = na.net;
  node = na.node;
  point = na.point;
  otherAddr = strdupplus(na.otherAddr);
}


net_address &net_address::operator = (net_address &na)
{
  if (&na != this)
  {
    delete[] otherAddr;
    copy(na);
  }

  return *this;
}


void net_address::set (unsigned int zone, unsigned int net,
                       unsigned int node, unsigned int point)
{
  this->zone = zone;
  this->net = net;
  this->node = node;
  this->point = point;
}


unsigned int net_address::getZone () const
{
  return zone;
}


unsigned int net_address::getNet () const
{
  return net;
}


unsigned int net_address::getNode () const
{
  return node;
}


unsigned int net_address::getPoint () const
{
  return point;
}


// NULL resets both addresses
void net_address::set (const char *address)
{
  unsigned int zo, ne, no, po = 0;

  if (address && (sscanf(address, "%u:%u/%u.%u", &zo, &ne, &no, &po) >= 3))
    set(zo, ne, no, po);
  else
  {
    delete[] otherAddr;
    otherAddr = strdupplus(address);

    if (!address) set(0, 0, 0, 0);
  }
}


// return valid address string or NULL
char *net_address::get (bool other) const
{
  static char fidoAddr[FIDOADDRLEN + 1];

  if (other) return (otherAddr && *otherAddr ? otherAddr : NULL);
  else
  {
    if (zone == 0 && net == 0 && node == 0 && point == 0) return NULL;

    if (point) sprintf(fidoAddr, "%u:%u/%u.%u", zone, net, node, point);
    else sprintf(fidoAddr, "%u:%u/%u", zone, net, node);

    return fidoAddr;
  }
}
