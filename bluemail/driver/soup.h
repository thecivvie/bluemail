/*
 * blueMail offline mail reader
 * SOUP driver

 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef SOUP_H
#define SOUP_H


#include "../bmail.h"


class soup : public basic_main
{
  private:
    struct SOUP_AREA_INFO
    {
      char number[12];
      const char *prefix;
      const char *title;
      char format;
      char index;
      bool isEmail;
      int totmsgs;
      int numpers;
      long offset;
      SOUP_AREA_INFO *next;
    } **areas;
    long **headerlines;
    PERSIDX *persIDX;
    int persmsgs;
    bodytype **body;
    char error[80];
    char line[MAXLINELEN];
    bool offcfg;
    FILE *msgFile;

    bool getAreas();
    int buildIndices();
    bool openFile(SOUP_AREA_INFO *);
    bool readCommands();

  public:
    soup(bmail *);
    ~soup();
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
    int getAreaIndex(const char *);     // for access from soup_reply only
    int getAreaIndex(bool) const;       // for access from soup_reply only
};


class soup_reply : public basic_reply
{
  private:
    soup *mainDriver;
    struct REPLY
    {
      struct
      {
        const char *from;
        const char *to;
        const char *subject;
        const char *date;
        const char *references;
        const char *newsgroups;
        const char *netAddr;
      } header;
      bool latin1;
      const char *filename;
      long length;
      REPLY *next;
    } *firstReply, *currentReply;
    char line[MAXLINELEN];
    struct EXTRA_AREA
    {
      const char *title;
      bool subscribed;
      EXTRA_AREA *next;
    } *extraAreas;
    long *extraAreasLIST;
    int extraAreasCount, extraAreasLISTCount;
    FILE *list;

    void replyInf();
    bool putReplies();
    bool put1Reply(FILE *, REPLY *, int);
    bool getReplies();
    bool get1Reply(FILE *, REPLY *&, bool, long);
    bool putConfig();
    void initExtraAreaLIST();

  public:
    soup_reply(bmail *, main_driver *);
    ~soup_reply();
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
    const char *getExtraArea(int &);
    void setExtraArea(int, bool, const char *);
};


#endif
