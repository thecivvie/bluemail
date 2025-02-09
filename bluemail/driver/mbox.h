/*
 * blueMail offline mail reader
 * Unix style mail file (Berkeley format - mbox) driver

 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef MBOX_H
#define MBOX_H


#include "../bmail.h"


class mbox : public basic_main
{
  private:
    int totmsgs;
    struct MSG_INFO
    {
      long top;
      long header;
      long status;
      long x_status;
      long body;
      long end;
      int stat;
      long headerlines;
      MSG_INFO *next;
    } *anchor, **msg;
    char line[MAXLINELEN];
    char lockfile[MYMAXPATH + 6];
    FILE *mboxf;

    void buildIndices();
    const char *Status(int) const;
    void catastrophe(bool, const char *) const;

  public:
    const char *mfile;     // for access from mbox_reply only

    mbox(bmail *);
    ~mbox();
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


class mbox_reply : public basic_reply
{
  private:
    mbox *mainDriver;
    struct REPLY
    {
      struct
      {
        const char *from;
        const char *to;
        const char *subject;
        const char *date;
        const char *inreplyto;
        const char *netAddr;
      } header;
      bool latin1;
      const char *filename;
      long length;
      REPLY *next;
    } *firstReply, *currentReply;

    void replyInf();
    bool putReplies();
    bool put1Reply(REPLY *, int);
    bool getReplies();
    bool get1Reply(REPLY *, const char *);

  public:
    mbox_reply(bmail *, main_driver *);
    ~mbox_reply();
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
