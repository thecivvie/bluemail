/*
 * blueMail offline mail reader
 * Blue Wave packet driver

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef BWAVE_H
#define BWAVE_H


#include "../bmail.h"

#ifndef BIG_ENDIAN
  #define BIG_ENDIAN
#endif
#include "bluewave.h"


class bwave : public basic_main
{
  private:
    INF_AREA_INFO *areas;
    PERSIDX *persIDX;
    int persmsgs;
    bodytype **body;
    char packetID[9];
    char error[80];
    INF_HEADER infHeader;
    unsigned int infHeaderLen, infAreainfoLen, mixStructLen, ftiStructLen;
    unsigned int net, node;
    //bool usesUPL;
    unsigned int ctrl_flags;
    int maxfromto, maxsubject;
    int *mixID, noOfMixRecs;
    MIX_REC *mixRecord;
    FILE *ftiFile, *datFile;

    void setPacketID();
    FILE *openFile(const char *);
    bool readINF();
    bool readMIX();

  public:
    bwave(bmail *);
    ~bwave();
    const char *init();
    area_header *getNextArea();
    int getNoOfLetters();
    letter_header *getNextLetter();
    const char *getBody(int, int, letter_header *);
    void initMSF(int **);
    int readMSF(int **);
    const char *saveMSF(int **);
    const char *getPacketID() const;
    bool isLatin1();
    bool offlineConfig();
    unsigned int getNet() const;         // for access from bwave_reply only
    unsigned int getNode() const;        // for access from bwave_reply only
    INF_HEADER &getInfHeader();          // for access from bwave_reply only
};


class bwave_reply : public basic_reply
{
  private:
    bwave *mainDriver;
    struct REPLY
    {
      UPL_REC upl;
      const char *filename;
      long length;
      REPLY *next;
    } *firstReply, *currentReply;

    int getAreaIndex(const char *) const;
    void replyInf();
    bool putReplies();
    bool put1Reply(FILE *, FILE *, FILE *, REPLY *);
    bool getReplies();
    bool get1Reply(FILE *, FILE *, FILE *, REPLY *, bool, unsigned int);
    bool putConfig();

  public:
    bwave_reply(bmail *, main_driver *);
    ~bwave_reply();
    const char *init();
    area_header *getNextArea();
    letter_header *getNextLetter();
    const char *getBody(int, int, letter_header *);
    void resetLetters();
    bool isLatin1();
    void enterLetter(letter_header *, const char *, long);
    void killLetter(int);
    const char *newLetterBody(int, long);
    void getConfig();
};


#endif
