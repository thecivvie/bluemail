/*
 * blueMail offline mail reader
 * OMEN driver

 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef OMEN_H
#define OMEN_H


#include "../bmail.h"


// OMEN area flags
#define BrdStatus_WRITE      1
#define BrdStatus_SYSOP      2
#define BrdStatus_PRIVATE    4
#define BrdStatus_PUBLIC     8
#define BrdStatus_NETMAIL   16
#define BrdStatus_ALIAS     32
#define BrdStatus_SELECTED  64
#define BrdStatus_reserved 128

// OMEN command flags
#define Command_SAVE     1
#define Command_PRIVATE 16
#define Command_ALIAS   32

// OMEN netmail attribute flags
#define NetAttrib_KILL  1
#define NetAttrib_CRASH 2


// OMEN reply header
struct OMEN_REPLY_HEADER
{
  unsigned char Command;
  unsigned char CurBoard;
  unsigned char MoveBoard;
  unsigned char MsgNumber[2];
  unsigned char to_length;
  char WhoTo[35];
  unsigned char subj_length;
  char Subject[72];
  unsigned char DestZone[2];
  unsigned char DestNet[2];
  unsigned char DestNode[2];
  unsigned char NetAttrib;
  unsigned char alias_length;
  char Alias[20];
  unsigned char CurHighBoard;
  unsigned char MoveHighBoard;
  unsigned char MsgHighNumber[2];
  unsigned char ExtraSpace[4];
};


class omen : public basic_main
{
  private:
    struct OMEN_AREA_INFO
    {
      int number;
      char shortname[17];
      char *title;
      int flags;
      int totmsgs;
      int numpers;
    } *areas;
    PERSIDX *persIDX;
    int persmsgs;
    bodytype **body;
    char packetID[3];
    bool c_set_iso, select_on;
    char line[MAXLINELEN];
    FILE *newmsg;

    bool getAreas(FILE *);
    void readInfo();
    int buildIndices();

  public:
    omen(bmail *);
    ~omen();
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
    int getAreaIndex(int) const;         // for access from omen_reply only
};


class omen_reply : public basic_reply
{
  private:
    omen *mainDriver;
    struct REPLY
    {
      OMEN_REPLY_HEADER header;
      const char *filename;
      long length;
      REPLY *next;
    } *firstReply, *currentReply;

    void replyInf();
    bool putReplies();
    bool put1Reply(FILE *, REPLY *, int);
    bool getReplies();
    bool get1Reply(FILE *, REPLY *, const char *);
    bool putConfig();

  public:
    omen_reply(bmail *, main_driver *);
    ~omen_reply();
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
