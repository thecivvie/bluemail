/*
 * blueMail offline mail reader
 * demonstration driver (will be used for both packet and service)

 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef DEMO_H
#define DEMO_H


#include "../bmail.h"


#define DEMO_MAXFROMTO  35
#define DEMO_MAXSUBJECT 71


class demo : public basic_main
{
  private:
    PERSIDX *persIDX;
    int persmsgs;
    bodytype **body;

    int totmsgs, infomsglen;
    char *me, *you, *Demo, *info;

  public:
    demo(bmail *);
    ~demo();
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
};


class demo_reply : public basic_reply
{
  private:
    demo *mainDriver;
    bool service;
    struct REPLY
    {
      struct
      {
        char from[DEMO_MAXFROMTO + 1];
        char to[DEMO_MAXFROMTO + 1];
        char subject[DEMO_MAXSUBJECT + 1];
        char date[24];
      } header;
      const char *filename;
      long length;
      REPLY *next;
    } *firstReply, *currentReply;

    void replyInf();
    bool putReplies();
    bool put1Reply(FILE *, REPLY *);
    bool getReplies();
    bool get1Reply(FILE *, REPLY *);

  public:
    demo_reply(bmail *, main_driver *);
    ~demo_reply();
    const char *init();
    area_header *getNextArea();
    letter_header *getNextLetter();
    const char *getBody(int, int, letter_header *);
    void resetLetters();
    bool isLatin1();
    void enterLetter(letter_header *, const char *, long);
    void killLetter(int);
    const char *newLetterBody(int, long);
};


#endif
