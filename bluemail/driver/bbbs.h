/*
 * blueMail offline mail reader
 * BBBS driver

 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef BBBS_H
#define BBBS_H


#include "../bmail.h"

#define __BBBS_NO_EXTERNS__
#include "bbbsdef.h"
#undef true
#undef false
struct userconfstat
{
  unlong lastread;
  unlong status;
};

#ifndef BIG_ENDIAN
  #define BIG_ENDIAN
#endif
#include "bluewave.h"


class bbbs : public basic_main
{
  private:
    struct BBBS_AREA_INFO
    {
      char *title;
      unlong status;
      int totmsgs;
      int numpers;
      unlong first;
    } *areas;
    PERSIDX *persIDX;
    int persmsgs;
    struct BODYTYPE
    {
      bool extraline;
      unlong lines;
      long pointer;
    } **body;
    char error[80];
    static struct TREE
    {
      short c0;
      short c1;
    } tree[];
    char *unpacked;
    enum filetype {NONE, DAT, TXT};
    bool user0;
    FILE *msgdat;

    bool getAreas();
    bool openFile(int, filetype);
    int buildIndices();
    int unpack(byte *);
    void addLetterBody(int);

  public:
    bbbs(bmail *);
    ~bbbs();
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
    char *getConfFile(int);                 // for access from bbbs_reply only
    int getAreaIndex(const char *) const;   // for access from bbbs_reply only
    bool isFido(int) const;                 // for access from bbbs_reply only
    bool isNetmail(int) const;              // for access from bbbs_reply only
};


class bbbs_reply : public basic_reply
{
  private:
    bbbs *mainDriver;
    struct REPLY
    {
      struct
      {
        const char *from;
        const char *to;
        const char *subject;
        const char *areatag;
        int flags;
        const char *netAddr;
      } header;
      const char *filename;
      long length;
      REPLY *next;
    } *firstReply, *currentReply;
    char scriptname[11];

    void replyInf();
    void quotation_out(FILE *, const char *);
    bool putReplies();
    bool put1Reply(FILE *, REPLY *);
    bool getReplies();
    bool get1Reply(FILE *, REPLY *);

  public:
    bbbs_reply(bmail *, main_driver *);
    ~bbbs_reply();
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
