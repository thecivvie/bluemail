/*
 * blueMail offline mail reader
 * driver_list ("loads" the driver for the specific mail format)

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>,
                    Robert Vukovic <vrobert@uns.ns.ac.yu>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include "bmail.h"
#include "driverl.h"
#include "../common/auxil.h"
#include "../common/mysystem.h"

#ifdef DRV_DEMO
  #include "driver/demo.h"
#endif
#ifdef DRV_BWAVE
  #include "driver/bwave.h"
#endif
#ifdef DRV_QWK
  #include "driver/qwk.h"
#endif
#ifdef DRV_SOUP
  #include "driver/soup.h"
#endif
#ifdef DRV_OMEN
  #include "driver/omen.h"
#endif
#ifdef DRV_HIPPO
  #include "driver/hippo.h"
#endif
#ifdef DRV_HMB
  #include "driver/hmb.h"
#endif
#ifdef DRV_MBOX
  #include "driver/mbox.h"
#endif
#ifdef DRV_BBBS
  #include "driver/bbbs.h"
#endif


// driver attribute flags
#define NO_ATTRB   0x00
#define PERSONAL   0x01   // driver can tell which messages are personal ones
#define TEARLINE   0x02   // needs a tearline in FidoNet style echomail
#define QPRTABLE   0x04   // can have quoted-printable messages
#define CROSSPOST  0x08   // area a message is entered in may be edited
#define EMAIL      0x10   // needs a configured e-mail address
#define EXTRAAREAS 0x20   // there are more areas than told by getNoOfAreas()


enum drivertype {UNDEF,
                 DEMO, BWAVE, QWK, SOUP, OMEN, HIPPO, HMB, MBOX, BBBS};


driver_list::driver_list (bmail *bm)
{
  drivertype dt;
  file_list *fl = bm->fileList;
  envVar = (char *) "BMDRIVER";

  // find out the mail format
#ifdef DRV_DEMO
  if (fl->exists("demo.pkt") && (fl->getSize() == 0)) dt = DEMO;
  else
#endif
#ifdef DRV_BWAVE
  if (fl->exists(".inf")) dt = BWAVE;
  else
#endif
#ifdef DRV_QWK
  if (fl->exists("control.dat") && fl->exists("messages.dat")) dt = QWK;
  else
#endif
#ifdef DRV_SOUP
  if (fl->exists("areas")) dt = SOUP;
  else
#endif
#ifdef DRV_OMEN
  if (fl->exists("newmsg*") && fl->exists("system*")) dt = OMEN;
  else
#endif
#ifdef DRV_HIPPO
  if (fl->exists(".hd")) dt = HIPPO;
  else
#endif
#ifdef DRV_HMB
  if (fl->exists("msginfo.bbs") && fl->exists("msgidx.bbs") &&
      fl->exists("msghdr.bbs") && fl->exists("msgtxt.bbs")) dt = HMB;
  else
#endif
#ifdef DRV_MBOX
  if (bm->getLastFileName() && fl->exists(fname(bm->getLastFileName())))
    dt = MBOX;
  else
#endif
#ifdef DRV_BBBS
  if (fl->exists("areacfg4.dat")) dt = BBBS;
  else
#endif
  dt = UNDEF;

  // now "load" the driver
  switch (dt)
  {
#ifdef DRV_DEMO
    case DEMO:
      driverList[0].driver = new demo(bm);
      driverList[1].driver = new demo_reply(bm, driverList[0].driver);
      noOfDrivers = 2;
      driverList[0].offset = 1;   // normal areas start here
      driverList[1].offset = 0;   // start for REPLY_AREA
      attributes = PERSONAL | TEARLINE;
      driverID = "Demo";
      break;
#endif
#ifdef DRV_BWAVE
    case BWAVE:
      driverList[0].driver = new bwave(bm);
      driverList[1].driver = new bwave_reply(bm, driverList[0].driver);
      noOfDrivers = 2;
      driverList[0].offset = 1;   // normal areas start here
      driverList[1].offset = 0;   // start for REPLY_AREA
      attributes = PERSONAL | QPRTABLE;
      driverID = "Blue Wave";
      break;
#endif
#ifdef DRV_QWK
    case QWK:
      driverList[0].driver = new qwk(bm);
      driverList[1].driver = new qwk_reply(bm, driverList[0].driver);
      noOfDrivers = 2;
      driverList[0].offset = 1;   // normal areas start here
      driverList[1].offset = 0;   // start for REPLY_AREA
      attributes = TEARLINE;
      if (((qwk *) driverList[0].driver)->isQWKE())
      {
        attributes |= QPRTABLE;
        driverID = "QWKE";
      }
      else driverID = "QWK";
      break;
#endif
#ifdef DRV_SOUP
    case SOUP:
      driverList[0].driver = new soup(bm);
      driverList[1].driver = new soup_reply(bm, driverList[0].driver);
      noOfDrivers = 2;
      driverList[0].offset = 1;   // normal areas start here
      driverList[1].offset = 0;   // start for REPLY_AREA
      attributes = QPRTABLE | CROSSPOST | EMAIL | EXTRAAREAS;
      if (*bm->resourceObject->get(IsPersonal)) attributes |= PERSONAL;
      driverID = "SOUP";
      break;
#endif
#ifdef DRV_OMEN
    case OMEN:
      driverList[0].driver = new omen(bm);
      driverList[1].driver = new omen_reply(bm, driverList[0].driver);
      noOfDrivers = 2;
      driverList[0].offset = 1;   // normal areas start here
      driverList[1].offset = 0;   // start for REPLY_AREA
      attributes = PERSONAL | TEARLINE | QPRTABLE;
      driverID = "OMEN";
      break;
#endif
#ifdef DRV_HIPPO
    case HIPPO:
      driverList[0].driver = new hippo(bm);
      driverList[1].driver = new hippo_reply(bm, driverList[0].driver);
      noOfDrivers = 2;
      driverList[0].offset = 1;   // normal areas start here
      driverList[1].offset = 0;   // start for REPLY_AREA
      attributes = PERSONAL | TEARLINE;
      driverID = "Hippo";
      break;
#endif
#ifdef DRV_HMB
    case HMB:
      driverList[0].driver = new hmb(bm);
      driverList[1].driver = new hmb_reply(bm, driverList[0].driver);
      noOfDrivers = 2;
      driverList[0].offset = 1;   // normal areas start here
      driverList[1].offset = 0;   // start for REPLY_AREA
      attributes = PERSONAL | TEARLINE;
      driverID = "Hudson";
      break;
#endif
#ifdef DRV_MBOX
    case MBOX:
      driverList[0].driver = new mbox(bm);
      driverList[1].driver = new mbox_reply(bm, driverList[0].driver);
      noOfDrivers = 2;
      driverList[0].offset = 1;   // normal areas start here
      driverList[1].offset = 0;   // start for REPLY_AREA
      attributes = QPRTABLE | EMAIL;
      driverID = "mbox";
      break;
#endif
#ifdef DRV_BBBS
    case BBBS:
      driverList[0].driver = new bbbs(bm);
      driverList[1].driver = new bbbs_reply(bm, driverList[0].driver);
      noOfDrivers = 2;
      driverList[0].offset = 1;   // normal areas start here
      driverList[1].offset = 0;   // start for REPLY_AREA
      attributes = PERSONAL | TEARLINE;
      driverID = "BBBS";
      break;
#endif
    default:
      driverList[0].driver = NULL;
      driverList[1].driver = NULL;
      noOfDrivers = 0;
      driverList[0].offset = 0;   // normal areas start here
      driverList[1].offset = -1;  // no REPLY_AREA
      attributes = NO_ATTRB;
      driverID = NULL;
  }

  driverList[0].read = NULL;
  driverList[1].read = NULL;

  if (driverID)
  {
    sprintf(driverEnv, "%.8s=%.11s", envVar, driverID);
    myputenv(driverEnv);
  }
}


driver_list::~driver_list ()
{
  myputenv(envVar);

  while (noOfDrivers--)
  {
    delete driverList[noOfDrivers].read;
    delete driverList[noOfDrivers].driver;
  }
}


const char *driver_list::init (bmail *bm)
{
  const char *init_error = NULL;

  for (int i = 0; i < noOfDrivers && init_error == NULL; i++)
    init_error = driverList[i].driver->init();

  if (!init_error)
  {
    if (noOfDrivers >= 1)
      driverList[0].read = new main_reader(bm, driverList[0].driver);

    if (noOfDrivers == 2)
      driverList[1].read = new reply_reader(bm, driverList[1].driver);
  }

  return init_error;
}


int driver_list::getNoOfDrivers () const
{
  return noOfDrivers;
}


bool driver_list::canReply () const
{
  return (driverList[1].driver != NULL);
}


bool driver_list::offlineConfig () const
{
  return canReply() && driverList[0].driver->offlineConfig();
}


main_driver *driver_list::getDriver (int areaNo) const
{
  int i = (areaNo <= driverList[1].offset);
  return driverList[i].driver;
}


main_driver *driver_list::getMainDriver () const
{
  return driverList[0].driver;
}


reply_driver *driver_list::getReplyDriver () const
{
  return (reply_driver *) driverList[1].driver;
}


reader *driver_list::getReadObject (main_driver *driver) const
{
  int i = (driver == driverList[1].driver);
  return driverList[i].read;
}


int driver_list::getOffset(main_driver *driver) const
{
  int i = (driver == driverList[1].driver);
  return driverList[i].offset;
}


bool driver_list::hasPersonal () const
{
  return (attributes & PERSONAL) != 0;
}


bool driver_list::useTearline () const
{
  return (attributes & TEARLINE) != 0;
}


bool driver_list::allowQuotedPrintable () const
{
  return (attributes & QPRTABLE) != 0;
}


void driver_list::toggleQuotedPrintable ()
{
  attributes ^= QPRTABLE;
}


bool driver_list::allowCrossPost () const
{
  return (attributes & CROSSPOST) != 0;
}


bool driver_list::useEmail () const
{
  return (attributes & EMAIL) != 0;
}


bool driver_list::hasExtraAreas () const
{
  return (attributes & EXTRAAREAS) != 0;
}
