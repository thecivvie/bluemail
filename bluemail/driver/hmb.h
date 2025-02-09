/*
 * blueMail offline mail reader
 * Hudson Message Base driver

 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef HMB_H
#define HMB_H


#include "../bmail.h"

#ifndef BIG_ENDIAN
  #define BIG_ENDIAN
#endif
#include "hudson.h"
#include "bluewave.h"
#define HAS_UINT
#include "fido.h"


class hmb : public basic_main
{
  private:
    struct
    {
      unsigned char number;
      char shortname[4];
      char *title;
      char *address;
      unsigned int lastread;
      int numpers;
    } board[201];              // 200 plus one for the personal area
    bool isNetmail[200];
    struct PERSHIDX
    {
      unsigned int msgnum;
      unsigned char area;
      int letter;
    } *persHIDX;
    int persmsgs;
    bodytype **body;
    HMB_INFO hmb_info;
    char error[80];
    FILE *idx, *hdr, *txt;

    FILE *openFile(const char *);
    int getPersIndices();
    int getAreas();
    void getNextHdr(HMB_HDR *, int, bool) const;

  public:
    hmb(bmail *);
    ~hmb();
    const char *init();
    area_header *getNextArea();
    int getNoOfLetters();
    letter_header *getNextLetter();
    const char *getBody(int, int, letter_header *);
    void resetLetters();
    void initMSF(int **);
    int readMSF(int **);
    const char *saveMSF(int **);
    const char *getPacketID() const;
    bool isLatin1();
    bool offlineConfig();
    bool isNetmailArea(int) const;           // for access from hmb_reply only
    int getAreaIndex(unsigned char) const;   // for access from hmb_reply only
    int getAreaIndex(const char *) const;    // for access from hmb_reply only
    unsigned char getBoard(const char *) const;  // access from hmb_reply only
    const char *getBoardAddr(const char *) const;// access from hmb_reply only
};


class hmb_reply : public basic_reply
{
  private:
    hmb *mainDriver;
    struct REPLY
    {
      FIDO_MSG_HEADER msgh;
      bool flag_direct;
      unsigned char board;
      const char *filename;
      long length;
      REPLY *next;
    } *firstReply, *currentReply;
    char scriptname[11];

    void replyInf();
    bool putReplies();
    bool put1Reply(FILE *, REPLY *);
    bool getReplies();
    bool get1Reply(FILE *, REPLY *);

  public:
    hmb_reply(bmail *, main_driver *);
    ~hmb_reply();
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
