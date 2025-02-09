/*
 * blueMail offline mail reader
 * QWK / QWKE packet driver

 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef QWK_H
#define QWK_H


#include "../bmail.h"


#define NDX_RECLEN      5
#define MSGDAT_RECLEN 128

// these are for QWKE (QWKE_MAXSUBJECT >= QWKE_MAXFROMTO >= 25)
#define QWKE_MAXFROMTO  60     // the value for QWK is 25
#define QWKE_MAXSUBJECT 80     // the value for QWK is 25


// class to handle the QWK header in the MESSAGES.DAT file
class qwk_header
{
  private:
    struct QWK_MSGHEADER
    {
      char status;               // message status flag
      char msgnum[7];            // message number
      char date[8];              // date (MM-DD-YY)
      char time[5];              // time (24 hour HH:MM)
      char to[25];               // to
      char from[25];             // from
      char subject[25];          // subject
      char password[12];         // message passwort
      char refnum[8];            // reference message number
      char chunks[6];            // number of 128 byte chunks (header + data)
      char alive;                // message active (225) or to be killed (226)
      unsigned char confnum[2];  // conference number (in little endian)
      char rest[3];              // unimportant information
    };

    void qstrncpy(char *, const char *, size_t);

  public:
    char from[QWKE_MAXFROMTO + 1], to[QWKE_MAXFROMTO + 1],
         subject[QWKE_MAXSUBJECT + 1], date[15];
    int msgnum, refnum, confnum;
    long msglen;
    bool privat, skipit;

    bool read(FILE *);
    bool write(FILE *);
};


class qwk : public basic_main
{
  private:
    struct QWK_AREA_INFO
    {
      char areanum[6];
      int number;          // for performance reasons only
      char *title;
      int totmsgs;
      int flags;           // for QWKE only
    } *areas;
    bodytype **body;
    char bulletinfiles[3][13];
    char BBSid[9];
    char error[80];
    bool qwke, offcfg;
    char ctrlname[26];
    FILE *ctrlFile, *msgFile;

    bool readControl();
    char *nextLine();
    FILE *openFile(const char *);
    bool readExtInformation();
    bool readDoorId();
    bool hasNdx();
    void readIndices();
    int getLetterIndex(int, long) const;

  public:
    qwk(bmail *);
    ~qwk();
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
    int getAreaIndex(int) const;             // for access from qwk_reply only
    bool isQWKE() const;                     // for access from qwk_reply only
    char *isQWKEKludge(const char *, char *, // for access from qwk_reply only
                       char *, int) const;
    const char *getCtrlname() const;         // for access from qwk_reply only
};


class qwk_reply : public basic_reply
{
  private:
    qwk *mainDriver;
    struct REPLY
    {
      qwk_header qh;
      const char *filename;
      const char *address;      // for QWKE (Netmail) only
      REPLY *next;
    } *firstReply, *currentReply;

    void replyInf();
    bool putReplies();
    bool put1Reply(FILE *, REPLY *);
    bool getReplies();
    bool get1Reply(FILE *, REPLY *);
    bool putConfig();

  public:
    qwk_reply(bmail *, main_driver *);
    ~qwk_reply();
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
