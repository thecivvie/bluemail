/*
 * blueMail offline mail reader
 * basic driver definitions

 Copyright (c) 2004 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef BASIC_H
#define BASIC_H


#include <time.h>
#include <stdio.h>


// area types
#define A_COLLECTION 0x0001   // collection of messages from other areas
#define A_REPLYAREA  0x0002   // the reply area
#define A_SUBSCRIBED 0x0004   // area is active, i.e. has been subscribed
#define A_FORCED     0x0008   // area may not be unsubscribed
#define A_LIST       0x0010   // area must appear in area list
#define A_NETMAIL    0x0020   // netmail area (not an echo area)
#define A_INTERNET   0x0040   // area is internet style (news, email)
#define A_READONLY   0x0080   // no posting allowed
#define A_ALIAS      0x0100   // use alias name in the this area
#define A_ALLOWPRIV  0x0200   // allow use of private flag in this echo area
#define A_CHRSLATIN1 0x0400   // preferred character set is Latin1 (not IBMPC)
#define A_MARKED     0x0800   // area is marked (for short area list)
// for offline configuration:
#define A_ADDED      0x4000   // area has been added (subscribe)
#define A_DROPPED    0x8000   // area has been dropped (unsubscribe)

// pseudo-area to allow manual subscriptions
#define AREA_EXTRA "-> Toggle Here to Manually Enter a New Subscription <-"

// message types (Blue Wave compatible)
#define PRIVATE       0x0001   // for addressee only
#define CRASH         0x0002   // high priority mail
#define RECEIVED      0x0004   // message received
#define SENT          0x0008   // message sent
#define FILEATTACHED  0x0010   // message with attached file
#define INTRANSIT     0x0020   // message to be forwarded (to others)
#define ORPHAN        0x0040   // message destination unknown
#define KILL          0x0080   // kill (delete) after sending
#define LOCAL         0x0100   // message originated here
#define HOLD          0x0200   // hold for pickup, don't send
#define IMMEDIATE     0x0400   // send message NOW        (*)
#define FILEREQUEST   0x0800   // file request
#define DIRECT        0x1000   // send direct, no routing (*)
#define UNUSED_M1     0x2000   // unused                  (*)
#define UNUSED_M2     0x4000   // unused                  (*)
#define FILEUPDATEREQ 0x8000   // updated file request
// (*) these are no FidoNet Technical Standard FTS-0001 flags

// message type strings
#define CRASH_TXT         "Crash"
#define RECEIVED_TXT      "Rcvd"
#define SENT_TXT          "Sent"
#define FILEATTACHED_TXT  "w/File"
#define INTRANSIT_TXT     "Trans"
#define ORPHAN_TXT        "Orphan"
#define KILL_TXT          "K/Sent"
#define LOCAL_TXT         "Local"
#define HOLD_TXT          "Hold"
#define IMMEDIATE_TXT     "Imm"
#define FILEREQUEST_TXT   "F/Req"
#define DIRECT_TXT        "Direct"
#define FILEUPDATEREQ_TXT "U/Req"

// message status flags (Blue Wave compatible)
#define MSF_READ     0x0001
#define MSF_REPLIED  0x0002
#define MSF_PERSONAL 0x0004
#define MSF_TAGGED   0x0008   // unused
#define MSF_SAVED    0x0010   // unused
#define MSF_PRINTED  0x0020   // unused
#define MSF_MARKED   0x0F00   // bmail switches between 0 (false) and 0x0F00
                              // (true) but accepts any bit set (true) to be
                              // interoperable with the four Blue Wave marks
                              // (which is no problem as bmail has only one)
#define SOFT_CR 141

// address lengths
#define FIDOADDRLEN  23
#define OTHERADDRLEN 99

// header lengths for drivers without limits
#define NEWSGROUPLIST  511
#define NEWSGROUPLEN   127
#define OTHERFROMTOLEN  79
#define OTHERSUBJLEN    99

// additional files to be included in bulletin list
#define BL_NOADD    0
#define BL_NEWFILES 1
#define BL_QWK      2

// for mail data stored into text files
#define MAXLINELEN 256


// message body access structure
struct bodytype
{
  long pointer;
  long msglen;
};

// temporary message index chain to set body[area][letter].pointer
struct MSG_NDX
{
  int area;
  long pointer;
  MSG_NDX *next;
};

// index of personal message msgnum in areas[area]
struct PERSIDX
{
  int area;
  int msgnum;
};

// temporary chain of personal messages to set persIDX[personal_letters]
struct pIDX
{
  PERSIDX persidx;
  pIDX *next;
};


// necessary forward references
class bmail;
class area_header;
class letter_header;
class basic_tool;
class file_list;
class net_address;


class main_driver
{
  public:
    virtual ~main_driver();
    virtual const char *init() = 0;
    virtual int getNoOfAreas() = 0;
    virtual area_header *getNextArea() = 0;
    virtual void selectArea(int) = 0;
    virtual int getNoOfLetters() = 0;
    virtual letter_header *getNextLetter() = 0;
    virtual const char *getBody(int, int, letter_header *) = 0;
    virtual void resetLetters() = 0;
    virtual void initMSF(int **) = 0;
    virtual int readMSF(int **) = 0;
    virtual const char *saveMSF(int **) = 0;
    virtual const char *getPacketID() const = 0;
    virtual bool isLatin1() = 0;
    virtual const char **getBulletins() = 0;
    virtual bool offlineConfig() = 0;
};


class reply_driver : public main_driver
{
  private:
    virtual void replyInf() = 0;

  protected:
    virtual bool putReplies() = 0;

  public:
    virtual ~reply_driver();
    virtual void enterLetter(letter_header *, const char *, long) = 0;
    virtual void killLetter(int) = 0;
    virtual const char *newLetterBody(int, long) = 0;
    virtual bool makeReply() = 0;
    virtual void getConfig() = 0;
    virtual const char *getExtraArea(int &) = 0;
    virtual void setExtraArea(int, bool, const char *) = 0;
};


class basic_main : public main_driver
{
  protected:
    bmail *bm;
    int NAID, NLID, currentArea, maxAreas;
    bool persArea;
    char *letterBody;
    const char **bulletins;

  public:
    basic_tool *tool;                // for access from reply driver only

    basic_main();
    int getNoOfAreas();
    void selectArea(int);
    void resetLetters();
    const char **getBulletins();
    int persOffset() const;          // for access from reply driver only
};


class basic_reply : public reply_driver
{
  protected:
    bmail *bm;
    int NLID, noOfLetters;
    char *replyBody;
    char replyPacketName[13];
    char error[80];

    area_header *getReplyArea(const char *);
    file_list *unpackReplies();

  public:
    basic_reply();
    int getNoOfAreas();
    void selectArea(int);
    int getNoOfLetters();
    void initMSF(int **);
    int readMSF(int **);
    const char *saveMSF(int **);
    const char *getPacketID() const;
    const char **getBulletins();
    bool offlineConfig();
    bool makeReply();
    void getConfig();
    const char *getExtraArea(int &);
    void setExtraArea(int, bool, const char *);
};


// routines common to more than one driver
class basic_tool
{
  private:
    bmail *bm;
    main_driver *driver;
    char line[MAXLINELEN];
    char *msgid, *from, *faddr, *raddr, *to, *taddr, *subject, *date,
         *newsgrps, *fupto, *refs, *inreplyto;
    bool qpEnc;

    void mimeCharset(const char *, bool *, bool *);
    void mimeEncodeHeader(FILE *, const char *);

  public:
    basic_tool(bmail *, main_driver *);
    ~basic_tool();
    void fsc0054(const char *, char, bool *);
    void MSFinit(int **, int, PERSIDX *, int);
    void XTIread(int **, int, const char * = ".xti");
    const char *XTIsave(int **, int, const char *, bool = true);
    char *getFidoAddr(const char *);
    void fidodate(char *, unsigned int, time_t);
    const char *rfc822_date();
    const char **bulletin_list(const char [][13], int, int);
    bool isQP(const char *);
    bool isUTF8(const char *);
    void mimeDecodeBody(char *);
    bool mimeEncodeBody(FILE *, const char *);
    void utf8DecodeBody(char *);
    long parseHeader(FILE *, long, bool *);
    char *getMessageID() const;
    char *getFrom() const;
    char *getFaddr() const;
    char *getReplyToAddr() const;
    char *getTo() const;
    char *getTaddr() const;
    char *getSubject() const;
    char *getDate() const;
    char *getNewsgroups() const;
    char *getFollowupTo() const;
    char *getReferences() const;
    char *getInReplyTo() const;
    bool isQP() const;
    char *fgetsline(FILE *);
    const char *uniqueID(int);
    void writeHeader(FILE *, bool, int, const char *, const char *,
                     const char *, const char *, const char *, const char *,
                     bool);
    void getBBBSAddr(const char *, unsigned int, net_address &, int &);
};


#endif
