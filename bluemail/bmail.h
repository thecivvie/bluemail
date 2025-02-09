/*
 * blueMail offline mail reader
 * class definitions for bmail

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef BMAIL_H
#define BMAIL_H


#include <time.h>
#include <stdio.h>
#include <sys/types.h>
#include "resource.h"
#include "service.h"
#include "driver/basic.h"

#if defined(__MSDOS__)
  #include <limits.h>
  #define SIZE_MAX SSIZE_MAX
  #define uint32_t unsigned int
#else
  #define __STDC_LIMIT_MACROS
  #include <inttypes.h>
#endif


// for file_list::sort()
#define FL_SORT_BY_NAME   0
#define FL_SORT_BY_DATE   1   // unused
#define FL_SORT_BY_NEWEST 2

// for letter_list::sort() - used in same order there!
#define LL_SORT_BY_SUBJ     0
#define LL_SORT_BY_MSGNUM   1
#define LL_SORT_BY_LASTNAME 2
#define LL_SORT_FIRSTTYPE LL_SORT_BY_SUBJ
#define LL_SORT_LASTTYPE  LL_SORT_BY_LASTNAME


// return types for saveLastread()
enum lrtype {LR_OK, LR_ERROR, LR_NORETRY};


// necessary forward references
class driver_list;
class reader;


class net_address
{
  private:
    unsigned int zone, net, node, point;
    char *otherAddr;

    void copy(net_address &);

  public:
    net_address();
    net_address(net_address &);
    ~net_address();
    net_address &operator =(net_address &);
    void set(unsigned int, unsigned int, unsigned int, unsigned int);
    unsigned int getZone() const;
    unsigned int getNet() const;
    unsigned int getNode() const;
    unsigned int getPoint() const;
    void set(const char *);
    char *get(bool = false) const;
};


class file_header
{
  private:
    char *name;
    uint32_t id;
    time_t date;
    off_t size;

  public:
    file_header *next;

    file_header(char *, time_t, off_t);
    ~file_header();
    const char *getName() const;
    uint32_t getID() const;
    time_t getDate() const;
    off_t getSize() const;
};


class file_list
{
  private:
    const char *dirname;
    file_header **files;
    int noOfFiles, activeFile;

  public:
    file_list(const char *);
    ~file_list();
    const char *getDir() const;
    void sort(int);
    int getNoOfFiles() const;
    void gotoFile(int);
    const char *getName() const;
    uint32_t getID() const;
    time_t getDate() const;
    off_t getSize() const;
    const char *exists(const char *);
    const char *getNext(const char *);
    FILE *ftryopen(const char *, const char *);
    void add(const char **, const char *, int *);
    int nextNumExt(const char *);
    bool changeName(const char *);
};

int fsortbyname(const void *, const void *);
int fsortbydate(const void *, const void *);
int fsortbynewest(const void *, const void *);


class file_stat
{
  private:
    resource *ro;

    struct filestat
    {
      uint32_t fileID;
      time_t fileDate;
      int msgsTotal;
      int msgsUnread;
      bool used;
      filestat *next;
    } *anchor, *last;

    void newStat(filestat *);

  public:
    file_stat(bmail *);
    ~file_stat();
    void addStat(uint32_t, time_t, int, int);
    bool getStat(uint32_t, time_t, int *, int *) const;
};


class area_header
{
  private:
    bmail *bm;
    int area, flags, noOfLetters, noOfPersonal, maxFromToLen, maxSubjLen;
    const char *number, *shortname, *title, *type;
    main_driver *driver;

  public:
    area_header(bmail *, int, const char *, const char *, const char *,
                const char *, int, int, int, int, int);
    ~area_header();
    const char *getNumber() const;
    const char *getShortName() const;
    const char *getTitle() const;
    const char *getAreaType() const;
    bool isCollection() const;
    bool isReplyArea() const;
    bool isSubscribed() const;
    bool isForced() const;
    bool isToList() const;
    bool useAlias() const;
    bool isNetmail() const;
    bool isInternet() const;
    bool isReadonly() const;
    bool hasTo() const;
    bool allowPrivate() const;
    bool isLatin1() const;
    bool isMarked() const;
    void setMarked(bool);
    void setAdded(bool = false);
    bool isAdded() const;
    void setDropped(bool = false);
    bool isDropped() const;
    int getNoOfLetters() const;
    int getNoOfUnread();
    int getNoOfPersonal() const;
    int getMaxFromToLen() const;
    int getMaxSubjLen() const;
    int supportedMSF() const;
};


class area_list
{
  private:
    bmail *bm;
    int noOfAreas, currentArea, *activeHeader, noOfActive;
    bool shortlist, *filter;
    area_header **areaHeader;

    int checkArea(int) const;
    void readAreaMarks();

  public:
    area_list(bmail *);
    ~area_list();
    void relist(bool);
    const char *getNumber() const;
    const char *getShortName(int = -1) const;
    const char *getTitle(int = -1) const;
    const char *getAreaType() const;
    bool isCollection() const;
    bool isReplyArea() const;
    bool isSubscribed() const;
    bool isForced() const;
    bool useAlias() const;
    bool isNetmail(int = -1) const;
    bool isInternet(int = -1) const;
    bool isReadonly() const;
    bool hasTo() const;
    bool allowPrivate() const;
    bool isLatin1() const;
    bool isLatin1(int) const;
    bool isMarked() const;
    bool isAnyMarked() const;
    void setMarked(bool);
    void setAdded(bool = false);
    bool isAdded() const;
    void setDropped(bool = false);
    bool isDropped() const;
    int getNoOfLetters() const;
    int getNoOfUnread() const;
    int getNoOfPersonal() const;
    int getMaxFromToLen() const;
    int getMaxSubjLen() const;
    int supportedMSF() const;
    int getNoOfAreas() const;
    int getNoOfActive() const;
    int getAreaNo() const;
    int getActive();
    void gotoArea(int);
    void gotoActive(int);
    bool findNetmail(int *) const;
    void getLetterList();
    void enterLetter(int, const char *, const char *, const char *, int, int,
                     net_address *, const char *, const char *, const char *,
                     long);
    void killLetter(int);
    const char *newLetterBody(int, long);
    bool repliesOK();
    bool makeReply();
    void refreshReplyArea();
    void updateCollectionStatus();
    void setFilter(bool *);
    bool areaConfig() const;
};


class letter_header
{
  private:
    driver_list *dl;
    resource *ro;
    main_driver *driver;
    reader *read;
    char *from, *to, *subject, *date;
    int LetterID, msgNum, replyTo, AreaID, flags;
    long length;
    bool latin1;
    net_address netAddr;
    char *replyAddr, *replyIn, *replyID;

  public:
    letter_header(bmail *, int, const char *, const char *, const char *,
                  const char *, int, int, int, int, bool, net_address *,
                  const char *, const char *, const char *, main_driver *);
    ~letter_header();
    int getLetterID() const;
    const char *getFrom() const;
    void setFrom(const char *);
    const char *getTo() const;
    void setTo(const char *);
    const char *getSubject() const;
    void setSubject(const char *);
    const char *getDate() const;
    int getMsgNum() const;
    int getReplyTo() const;
    int getAreaID() const;
    int getFlags() const;
    bool isPrivate() const;
    char *Flags() const;
    bool isLatin1() const;
    void setLatin1(bool);
    net_address &getNetAddr();
    const char *getReplyAddr() const;
    const char *getReplyIn() const;
    const char *getReplyID() const;
    main_driver *getDriver() const;
    bool isPersonal() const;
    bool isRead() const;
    void setRead();
    void setMarked();
    int getStatus() const;
    void setStatus(int);
    const char *getBody();
};


class letter_list
{
  private:
    resource *ro;
    main_driver *driver;
    reader *read;
    int area, areaType, noOfLetters, currentLetter, *activeHeader, noOfActive;
    bool shortlist, *filter;
    letter_header **letterHeader;

    void init();
    void cleanup();

  public:
    letter_list(bmail *, int, int);
    ~letter_list();
    void sort(int);
    void relist(bool);
    const char *getFrom() const;
    const char *getTo() const;
    const char *getSubject() const;
    const char *getDate() const;
    int getMsgNum() const;
    int getReplyTo() const;
    int getAreaID() const;
    int getFlags() const;
    bool isPrivate() const;
    char *Flags() const;
    bool isLatin1() const;
    net_address &getNetAddr();
    const char *getReplyAddr() const;
    const char *getReplyIn() const;
    const char *getReplyID() const;
    bool isPersonal() const;
    bool isRead() const;
    void setRead();
    int getStatus() const;
    void setStatus(int);
    const char *getBody() const;
    int getNoOfLetters() const;
    int getNoOfActive() const;
    void gotoLetter(int);
    void gotoActive(int);
    int getCurrent() const;
    int getActive() const;
    void refreshLetters();
    void setFilter(bool *);
};

int lsortbysubj(const void *, const void *);
int lsortbymsgnum(const void *, const void *);
int lsortbylastname(const void *, const void *);


class reader
{
  public:
    virtual ~reader();
    virtual int supportedMSF() = 0;
    virtual void setRead(int, int, bool) = 0;
    virtual bool isRead(int, int) const = 0;
    virtual void setStatus(int, int, int) = 0;
    virtual int getStatus(int, int) const = 0;
    virtual int getNoOfUnread(int) const = 0;
    virtual int getNoOfPersonal(int) const = 0;
    virtual const char *saveMSF() = 0;
};


class main_reader : public reader
{
  private:
    bmail *bm;
    main_driver *driver;
    int noOfAreas, *noOfLetters, **msgstat, msfs;

  public:
    main_reader(bmail *, main_driver *);
    ~main_reader();
    int supportedMSF();
    void setRead(int, int, bool);
    bool isRead(int, int) const;
    void setStatus(int, int, int);
    int getStatus(int, int) const;
    int getNoOfUnread(int) const;
    int getNoOfPersonal(int) const;
    const char *saveMSF();
};


class reply_reader: public reader
{
  public:
    reply_reader(bmail *, main_driver *);
    ~reply_reader();
    int supportedMSF();
    void setRead(int, int, bool);
    bool isRead(int, int) const ;
    void setStatus(int, int, int);
    int getStatus(int, int) const;
    int getNoOfUnread(int) const;
    int getNoOfPersonal(int) const;
    const char *saveMSF();
};


class driver_list
{
  private:
    struct
    {
      main_driver *driver;
      reader *read;
      int offset;
    } driverList[2];     // 0: main_driver/main_reader   (must exist)
                         // 1: reply_driver/reply_reader (may exist)
    int noOfDrivers, attributes;
    char *envVar;
    const char *driverID;
    char driverEnv[21];

  public:
    driver_list(bmail *);
    ~driver_list();
    const char *init(bmail *);
    int getNoOfDrivers() const;
    bool canReply() const;
    bool offlineConfig() const;
    main_driver *getDriver(int) const;
    main_driver *getMainDriver() const;
    reply_driver *getReplyDriver() const;
    reader *getReadObject(main_driver *) const;
    int getOffset(main_driver *) const;
    bool hasPersonal() const;
    bool useTearline() const;
    bool allowQuotedPrintable() const;
    void toggleQuotedPrintable();
    bool allowCrossPost() const;
    bool useEmail() const;
    bool hasExtraAreas() const;
};


class bmail
{
  private:
    char bm_version[41];
    char select_error[80];
    uint32_t lastFileID;
    time_t lastFileMtime;
    const char *lastFileName;
    servicetype serviceType;

    bool loadDriver(const char *);

  public:
    resource *resourceObject;
    services *service;
    file_list *fileList;
    driver_list *driverList;
    area_list *areaList;
    letter_list *letterList;

    bmail();
    ~bmail();
    void Delete();
    bool selectPacket(const char *);
    bool selectFileDB(const char *, bool);
    bool selectMbox(const char *);
    bool selectReply(const char *);
    bool selectDemo();
    servicetype getServiceType() const;
    lrtype saveLastread();
    uint32_t getLastFileID() const;
    time_t getLastFileMtime() const;
    const char *getLastFileName(bool = true) const;
    const char *version() const;
    bool isLatin1() const;
    const char **getBulletins() const;
    const char *getSelectError() const;
    const char *getExtraArea(int &);
    void setExtraArea(int, bool, const char * = NULL);
    const char *getAreaMarksFile(const char * = NULL);
    bool saveAreaMarks();
};


#endif
