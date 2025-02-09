/*
 * blueMail offline mail reader
 * demonstration driver (will be used for both packet and service)

 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "demo.h"
#include "../../common/auxil.h"
#include "../../common/mysystem.h"


#define TMSGLEN 240     // must be enough space for test message


/*
   demo main driver
*/

demo::demo (bmail *bm)
{
  this->bm = bm;
  tool = new basic_tool(bm, this);

  persmsgs = 1;
  persIDX = new PERSIDX[persmsgs];
  persIDX[0].area = 1;   // if persArea, the test area will become number 1
  persIDX[0].msgnum = 0;

  persArea = bm->resourceObject->isYes(PersonalArea);
  if (bm->getServiceType() == ST_REPLY) persArea = false;

  maxAreas = 1 + persOffset();
  totmsgs = 51;

  me = (char *) BM_NAME;
  you = (char *) "You";
  Demo = (char *) "Demo";

  bm->resourceObject->set(LoginName, you);
  bm->resourceObject->set(AliasName, you);
  bm->resourceObject->set(SysOpName, me);
  bm->resourceObject->set(BBSName, Demo);
  bm->resourceObject->set(hasLoginName, "N");

  info = (char *) "just a short note";
  infomsglen = 17;

  body = new bodytype *[maxAreas];
  for (int i = 0; i < maxAreas; i++) body[i] = NULL;
}


demo::~demo ()
{
  while (maxAreas) delete[] body[--maxAreas];

  delete[] letterBody;
  delete[] body;
  delete[] persIDX;
  delete tool;
}


inline const char *demo::init ()
{
  return NULL;
}


area_header *demo::getNextArea ()
{
  area_header *ah;

  bool isPersArea = (persArea && (NAID == 0));
  int msgs = (isPersArea ? persmsgs : totmsgs);

  body[NAID] = new bodytype[msgs];

  ah = new area_header(bm, NAID + bm->driverList->getOffset(this),
                       (isPersArea ? "PERS" : "1"),
                       (isPersArea ? "PERSONAL" : "TA"),
                       (isPersArea ? "Letters addressed to you" : "Test Area"),
                       (isPersArea ? "Demo Personal" : Demo),
                       (isPersArea ? A_COLLECTION | A_READONLY | A_LIST
                                   : A_SUBSCRIBED | A_FORCED),
                       msgs, 1, DEMO_MAXFROMTO, DEMO_MAXSUBJECT);
  NAID++;

  return ah;
}


inline int demo::getNoOfLetters ()
{
  return (persArea && (currentArea == 0) ? persmsgs : totmsgs);
}


letter_header *demo::getNextLetter ()
{
  int area, letter;
  long length;
  char subj[8];

  bool isPersArea = (persArea && (currentArea == 0));

  if (isPersArea)
  {
    area = persIDX[NLID].area;
    letter = persIDX[NLID].msgnum;
    length = TMSGLEN;
  }
  else
  {
    area = currentArea;
    letter = NLID;
    length = (NLID ? infomsglen : TMSGLEN);
  }

  body[area][letter].msglen = length;

  sprintf(subj, "Info %2d", letter);

  NLID++;
  return new letter_header(bm, letter, me,
                           (letter ? "All" : you),
                           (letter ? subj : "Test Message"),
                           "April 1, 2001", letter + 1, 0, area, 0,
                           isLatin1(), NULL, NULL, NULL, NULL, this);
}


const char *demo::getBody (int area, int letter, letter_header *lh)
{
  (void) lh;

  char *tmsg1 = (char *) "\1top (hidden)\n";
  char *tmsg2 = (char *) "line %2d\n";
  char *tmsg3 = (char *) "-- \nbye\n\1bottom (hidden)";
  char line[9];

  delete[] letterBody;
  letterBody = new char[body[area][letter].msglen + 1];

  // there is only one area (the test area) this function is called for,
  // and all letters go there
  if (letter) strcpy(letterBody, info);
  else   // must be less size than TMSGLEN bytes
  {
    strcpy(letterBody, tmsg1);
    for (int i = 1; i <= 25; i++)
    {
      sprintf(line, tmsg2, i);
      strcat(letterBody, line);
    }
    strcat(letterBody, tmsg3);
  }

  return letterBody;
}


void demo::initMSF (int **msgstat)
{
  for (int a = persOffset(); a < maxAreas; a++)
    for (int l = 0; l < totmsgs; l++)
      msgstat[a][l] = (l == 0 ? MSF_PERSONAL : 0);
}


int demo::readMSF (int **msgstat)
{
  tool->XTIread(msgstat, persOffset());
  return (MSF_READ | MSF_REPLIED | MSF_PERSONAL | MSF_MARKED);
}


inline const char *demo::saveMSF (int **msgstat)
{
  return tool->XTIsave(msgstat, persOffset(), getPacketID());
}


inline const char *demo::getPacketID () const
{
  return "demo";
}


inline bool demo::isLatin1 ()
{
  return false;
}


inline bool demo::offlineConfig ()
{
  return true;
}


/*
   demo reply driver
*/

demo_reply::demo_reply (bmail *bm, main_driver *mainDriver)
{
  this->bm = bm;
  this->mainDriver = (demo *) mainDriver;
  service = (bm->getServiceType() == ST_DEMO);
  firstReply = currentReply = NULL;
}


demo_reply::~demo_reply ()
{
  REPLY *next, *r = firstReply;

  while (r)
  {
    next = r->next;
    if (r->filename) remove(r->filename);
    delete[] r->filename;
    delete r;
    r = next;
  }

  delete[] replyBody;
}


const char *demo_reply::init ()
{
  sprintf(replyPacketName, "%.8s.dr%c", mainDriver->getPacketID(),
                                        (service ? 's' : 'p'));

  if (bm->getServiceType() != ST_REPLY) replyInf();

  return (getReplies() ? NULL : error);
}


inline area_header *demo_reply::getNextArea ()
{
  return getReplyArea("Demo Replies");
}


letter_header *demo_reply::getNextLetter ()
{
  letter_header *newLetter = new letter_header(bm, NLID,
                                               currentReply->header.from,
                                               currentReply->header.to,
                                               currentReply->header.subject,
                                               currentReply->header.date,
                                               NLID, 0,
                                               1 + mainDriver->persOffset(),
                                               0, isLatin1(), NULL, NULL,
                                               NULL, NULL, this);

  NLID++;
  currentReply = currentReply->next;

  return newLetter;
}


const char *demo_reply::getBody (int area, int letter, letter_header *lh)
{
  FILE *replyFile;

  (void) area;
  (void) lh;
  delete[] replyBody;

  REPLY *r = firstReply;
  for (int l = 1; l < letter; l++) r = r->next;

  if ((replyFile = fopen(r->filename, "rt")))
  {
    long msglen = r->length;
    replyBody = new char[msglen + 1];
    msglen = fread(replyBody, sizeof(char), msglen, replyFile);
    if (msglen && (replyBody[msglen - 1] == '\n')) msglen--;
    replyBody[msglen] = '\0';
    fclose(replyFile);
  }
  else replyBody = strdupplus("");

  return replyBody;
}


void demo_reply::resetLetters ()
{
  NLID = 1;
  currentReply = firstReply;
}


inline bool demo_reply::isLatin1 ()
{
  return mainDriver->isLatin1();
}


void demo_reply::enterLetter (letter_header *newLetter,
                              const char *newLetterFileName, long msglen)
{
  REPLY *newReply = new REPLY;
  memset(newReply, 0, sizeof(REPLY));

  strcpy(newReply->header.from, newLetter->getFrom());
  strcpy(newReply->header.to, newLetter->getTo());
  strcpy(newReply->header.subject, newLetter->getSubject());

  time_t now = time(NULL);
  strftime(newReply->header.date, sizeof(newReply->header.date),
           "%b %d, %Y   %H:%M:%S", localtime(&now));

  newReply->filename = strdupplus(newLetterFileName);
  newReply->length = msglen;
  newReply->next = NULL;

  if (!firstReply) firstReply = newReply;

  if (noOfLetters)
  {
    REPLY *r = firstReply;
    // find last reply
    for (int l = 1; l < noOfLetters; l++) r = r->next;
    r->next = newReply;
  }

  noOfLetters++;
}


void demo_reply::killLetter (int letter)
{
  REPLY *killReply, *r;

  if (letter == 1)
  {
    killReply = firstReply;
    firstReply = firstReply->next;
  }
  else
  {
    r = firstReply;
    for (int l = 1; l < letter - 1; l++) r = r->next;
    killReply = r->next;
    r->next = (letter == noOfLetters ? NULL : r->next->next);
  }

  remove(killReply->filename);
  delete[] killReply->filename;
  delete killReply;

  noOfLetters--;
  resetLetters();
}


const char *demo_reply::newLetterBody (int letter, long size)
{
  REPLY *r = firstReply;
  for (int l = 1; l < letter; l++) r = r->next;

  if (size >= 0) r->length = size;
  return r->filename;
}


void demo_reply::replyInf ()
{
  if (!service)
  {
    fcreate(bm->resourceObject->get(InfWorkDir), "demo.pkt");
    bm->service->createSystemInfFile(0, mainDriver->getPacketID(),
                                        strrchr(replyPacketName, '.') + 1);
  }
}


bool demo_reply::putReplies ()
{
  FILE *drp;
  bool success = true;

  // use packet name for this, too
  if (!(drp = fopen(replyPacketName, "wb"))) return false;

  if (fprintf(drp, "%05d", noOfLetters) != 5)
  {
    fclose(drp);
    return false;
  }

  REPLY *r = firstReply;

  for (int l = 0; l < noOfLetters; l++)
  {
    if (!put1Reply(drp, r))
    {
      success = false;
      break;
    }
    r = r->next;
  }

  fclose(drp);
  return success;
}


bool demo_reply::put1Reply (FILE *drp, REPLY *r)
{
  FILE *reply;
  int c;

  if (fwrite(&r->header, sizeof(r->header), 1, drp) != 1) return false;

  if (!(reply = fopen(r->filename, "rt"))) return false;

  // keep it simple
  while ((c = fgetc(reply)) != EOF) fputc(c, drp);

  fputc('\0', drp);   // put an end mark
  fclose(reply);

  return true;
}


bool demo_reply::getReplies ()
{
  file_list *replies;
  const char *drpFile = NULL;
  FILE *drp = NULL;
  bool success = true;

  if (!(replies = unpackReplies())) return true;

  if (!(drpFile = replies->exists((service ? ".drs" : ".drp"))) ||
      !(drp = replies->ftryopen(drpFile, "rb")) ||
      !(fscanf(drp, "%05d", &noOfLetters)))
  {
    strcpy(error, "Could not open reply file.");
    success = false;
  }
  else
  {
    REPLY seed, *r = &seed;
    seed.next = NULL;

    for (int l = 0; l < noOfLetters; l++)
    {
      r->next = new REPLY;
      r = r->next;
      r->filename = NULL;
      r->next = NULL;

      if (!get1Reply(drp, r))
      {
        strcpy(error, "Could not get reply.");
        success = false;
        break;
      }
    }

    firstReply = seed.next;
  }

  if (drp) fclose(drp);
  if (drpFile) remove(drpFile);
  delete replies;

  return success;
}


bool demo_reply::get1Reply (FILE *drp, REPLY *r)
{
  FILE *reply;
  char filename[13], filepath[MYMAXPATH];
  static int fnr = 1;
  int c;
  long length = 0;

  if (fread(&r->header, sizeof(r->header), 1, drp) != 1) return false;

  sprintf(filename, "1.%03d", fnr++);
  mkfname(filepath, bm->resourceObject->get(WorkDir), filename);

  if (!(reply = fopen(filepath, "wt"))) return false;

  // keep it simple
  while ((c = fgetc(drp)))
  {
    fputc(c, reply);
    length++;
  }

  r->filename = strdupplus(filepath);
  r->length = length;
  fclose(reply);

  return true;
}
