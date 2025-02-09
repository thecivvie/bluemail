/*
 * blueMail offline mail reader
 * Hippo v2 driver

 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef HIPPO_H
#define HIPPO_H


#include "../bmail.h"


class hippo : public basic_main
{
  private:
    struct HIPPO_AREA_INFO
    {
      char *title;
      int flags;
      int totmsgs;
      int numpers;
      HIPPO_AREA_INFO *next;
    } **areas;
    PERSIDX *persIDX;
    int persmsgs;
    bodytype **body;
    char packetID[9];
    bool hasInfo, gotAddr;
    net_address mainAddr;
    char line[MAXLINELEN];
    FILE *hd;

    int buildIndices();
    HIPPO_AREA_INFO *newArea(HIPPO_AREA_INFO *);
    int getAreaIndex(HIPPO_AREA_INFO *, const char *) const;
    HIPPO_AREA_INFO *getArea(HIPPO_AREA_INFO *, int) const;

  public:
    hippo(bmail *);
    ~hippo();
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
    int getAreaIndex(const char *) const;  // for access from hippo_reply only
    const char *getAreaInfo(int *) const;  // for access from hippo_reply only
};


class hippo_reply : public basic_reply
{
  private:
    hippo *mainDriver;
    struct REPLY
    {
      struct
      {
        char *to;
        char *subject;
        int replyto;
        const char *areatag;
        bool isPrivate;
        const char *netAddr;
      } header;
      const char *filename;
      long length;
      REPLY *next;
    } *firstReply, *currentReply;
    char line[MAXLINELEN];
    int confAreas, *confArea;

    void replyInf();
    bool putReplies();
    bool put1Reply(FILE *, REPLY *);
    bool getReplies();
    bool get1Reply(FILE *, REPLY *&);
    void get1Config(FILE *, const char *);
    bool putConfig(FILE *);

  public:
    hippo_reply(bmail *, main_driver *);
    ~hippo_reply();
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
