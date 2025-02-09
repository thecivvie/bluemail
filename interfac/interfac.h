/*
 * blueMail offline mail reader
 * class definitions for the interface (high level part)

 Copyright (c) 1996 Kolossvary Tamas <thomas@tvnet.hu>
 Copyright (c) 1997 John Zero <john@graphisoft.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef INTERFACE_H
#define INTERFACE_H


#include <signal.h>
#include "wincurs.h"
#include "../bluemail/bmail.h"
#include "color.h"


#ifdef FIXSTANDOUT
  #define PUTLINE(i, c) putline(i, interface->getUI()->colormode(c, (topline + i == active ? A_STANDOUT : A_NORMAL)))
#else
  #define PUTLINE(i, c) putline(i, (topline + i == active ? A_STANDOUT : c))
#endif


#define TAGLINE_LENGTH 76
#define QUERYINPUT 56

// WarningWindow results
#define WW_ESC -1
#define WW_NO   0
#define WW_YES  1

// save/print types (same order as in WarningWindow!)
#define SAVE_MAX      4     // no. of save/print types
#define SAVE_THIS     4
#define SAVE_PERSONAL 3
#define SAVE_MARKED   2
#define SAVE_LISTED   1

// delete types (same order as in WarningWindow!)
#define DEL_INF   3
#define DEL_REPLY 2
#define DEL_BOTH  1

// little area list types
#define AREAS_NETMAIL 0
#define AREAS_ALL     1

// letter parameter (reply) types
#define IS_NEW        0
#define IS_REPLY      1
#define IS_FORWARD    2
#define IS_DIFFAREA   4
#define IS_EDIT       8

// checkReplies() return codes
#define REP_OK      0
#define REP_DISCARD 1
#define REP_RETRY   2

// KeyHandle loops
#define KH_MAIN   0
#define KH_LIST   1
#define KH_ANSI   2

// FileListWindow types
#define FLT_UNKNOWN 0
#define FLT_PACKETS 1
#define FLT_MBOXES  2

// symbol for active filter
#define FILTER_SIGN ACS_DIAMOND


extern const char *llopts[];


enum direction {UP, DOWN, PGUP, PGDN, HOME, END, UPP, DWNP};
enum statetype
{
  nostate, servicelist, packetlist, filedblist, mboxlist, /* connectlist, */
  replymgrlist, arealist, bulletinlist, offlineconflist, letterlist,
  /* threadlist, */ letter, littlearealist, addressbook, taglinebook,
  ansiview
};
enum drawtype {againActPos, againPos1, firstTime};
enum convtype {CC_437toISO, CC_ISOto437, CC_WINtoISO};
enum fconvtype {F_IN, F_OUT, F_NONE};
enum searchtype {FND_YES, FND_NO, FND_STOP};


struct SERVICE
{
  char key;
  const char *name;
};


struct FILEDB
{
  OptionIDs id;
  const char *defined;
  const char *name;
  bool isFile;
};


class Window : public ShadowedWin
{
  private:
    Win *window;

  public:
    char *lineBuf;

    Window(int, int, int, int, chtype, const char * = NULL, chtype = 0,
           int = 3, int = 2, int = 3);
    ~Window();
    void delay_update(bool = true);
    void touch(bool = true);
    void putline(int, chtype);
    void Scroll(int);
};


class ListWindow
{
  private:
    int oldTop, savedTop;           // topline at last Draw() / last save
    int oldActive, savedActive;     // active at last Draw() / last save

    void checkPos(int);

  protected:
    Window *list;
    int list_max_y, list_max_x;     // max. lines and columns in list
    int topline;                    // the first element in the window
    int active;                     // this is the highlighted element
    bool smartScroll;

    void relist();
    void checkDel();
    void Draw();                    // redraws window with newly selected line
    virtual int noOfItems() = 0;
    virtual void oneLine(int) = 0;
    virtual searchtype oneSearch(int, const char *) = 0;
    virtual void extrakeys(int) = 0;

  public:
    ListWindow();
    virtual ~ListWindow();
    virtual void MakeActive() = 0;
    virtual void Delete() = 0;
    virtual void Quit() = 0;
    virtual void Touch();
    void Move(direction);
    void savePos();
    void restorePos();
    void setPos(int);
    searchtype search(const char *);
    void KeyHandle(int);
};


class ServiceListWindow : public ListWindow
{
  private:
    static SERVICE service[];
    int noOfServices, noOfListed;
    int *listed;
    char *filter;
    char format[9];

    void MakeChain();
    void DestroyChain();
    int noOfItems();
    void oneLine(int);
    searchtype oneSearch(int, const char *);
    void askFilter();
    void extrakeys(int);

  public:
    ServiceListWindow();
    ~ServiceListWindow();
    void MakeActive();
    void Delete();
    void Quit();
    srvcerror OpenService();
};


class FileListWindow : public ListWindow
{
  private:
    int noOfListed;
    file_list *fileList;
    int *listed;
    char *filter;
    int fileType;
    bool namesort;

    void DestroyChain();
    void Select();
    char *NameDate(char *);
    void oneLine(int);
    searchtype oneSearch(int, const char *);
    void renameFile();
    void askFilter();
    void extrakeys(int);

  public:
    file_stat *fileStat;

    FileListWindow();
    ~FileListWindow();
    void MakeActive();
    void Delete();
    void Quit();
    void MakeChain();
    void setFileType(int);
    bool OpenFile();
    int noOfItems();
};


class FileDBListWindow : public ListWindow
{
  private:
    static FILEDB filedb[];
    int noOfFileDBs, noOfListed;
    int *listed;
    char *filter;
    char format[8];

    void MakeChain();
    void DestroyChain();
    int getFileDBItem(int);
    void oneLine(int);
    searchtype oneSearch(int, const char *);
    void askFilter();
    void extrakeys(int);

  public:
    FileDBListWindow();
    ~FileDBListWindow();
    void MakeActive();
    void Delete();
    void Quit();
    srvcerror OpenFileDB();
    int noOfItems();
};


class ReplyMgrListWindow : public ListWindow
{
  private:
    int noOfListed;
    file_list *fileList;
    int *listedInf;
    char *filter;
    bool infsort;

    void DestroyChain();
    void Select();
    void oneLine(int);
    searchtype oneSearch(int, const char *);
    const char *replyPacket();
    void kill();
    void askFilter();
    void extrakeys(int);

  public:
    ReplyMgrListWindow();
    ~ReplyMgrListWindow();
    void MakeActive();
    void Delete();
    void Quit();
    void MakeChain();
    bool OpenReply();
    int noOfItems();
};


class HelpWindow
{
  private:
    Win *menu;
    ShadowedWin *win;
    const char *up_txt, *down_txt, *pgup_txt, *pgdn_txt, *home_txt, *end_txt,
               *search_txt, *fndnxt_txt, *save_txt;
    int midpos, endpos, hoffset;

    void newHelpMenu(const char **, const char **);
    void h_servicelist();
    void h_filelist();
    void h_filedblist();
    void h_replymgrlist();
    void h_arealist();
    void h_offlineconf();
    void h_letterlist();
    void h_general();
    void h_letter();
    void h_ansiview();

  public:
    HelpWindow();
    void help(statetype);
    void MakeActive();
    void Delete();
    void exchangeActive(statetype);
    void Reset();
    void More();
};


class LittleAreaListWindow : public ListWindow
{
  private:
    int noOfListed;
    int *listed;
    char *filter;
    int type;
    char format[13];

    void DestroyChain();
    int noOfItems();
    void oneLine(int);
    searchtype oneSearch(int, const char *);
    void askFilter();
    void extrakeys(int);

  public:
    LittleAreaListWindow();
    ~LittleAreaListWindow();
    void MakeActive();
    void Delete();
    void Quit();
    void MakeChain(int);
    int tellArea() const;
};


class AreaListWindow : public ListWindow
{
  private:
    bool oneSearchActive, *listed;
    char *filter;
    ShadowedWin *info;
    bool areainfo, hasPersonal;
    char format[20];
    bool isOneListed;

    void Touch();
    void MakeChain();
    void DestroyChain();
    int noOfItems();
    void oneLine(int);
    searchtype oneSearch(int, const char *);
    void NextAvail();
    void PrevAvail();
    void editMSF();
    void askFilter();
    void extrakeys(int);

  public:
    AreaListWindow();
    ~AreaListWindow();
    void MakeActive();
    void Delete();
    void Quit();
    void ResetActive();
    void Select();
    void FirstUnread();
    bool makeReply();
};


class BulletinListWindow : public ListWindow
{
  private:
    int *listed;
    char *filter;
    const char **bulletin;
    char format[8];
    int noOfBulletins, noOfListed;

    void MakeChain();
    void DestroyChain();
    void oneLine(int);
    searchtype oneSearch(int, const char *);
    void askFilter();
    void extrakeys(int);

  public:
    BulletinListWindow();
    ~BulletinListWindow();
    void set(const char **);
    void MakeActive();
    void Delete();
    void Quit();
    void OpenBulletin(int, bool);
    void OpenBulletin();
    int noOfItems();
};


class OfflineConfigListWindow : public ListWindow
{
  private:
    int noOfAreas, noOfListed;
    int *listed;
    char *filter;
    char format[20];
    int extraline;

    void MakeChain();
    void DestroyChain();
    void Select();
    int noOfItems();
    char SelectionMark();
    void oneLine(int);
    searchtype oneSearch(int, const char *);
    bool toggleSelection();
    void askSubscribe();
    bool isDupe(const char *);
    void gotoArea(const char *);
    void askFilter();
    void extrakeys(int);

  public:
    OfflineConfigListWindow();
    ~OfflineConfigListWindow();
    void MakeActive();
    void Delete();
    void Quit();
};


class LetterListWindow : public ListWindow
{
  private:
    bool oneSearchActive, *listed;
    char *filter;
    int letter_sort, netmail_sort;
    char format[44];
    bool showArea, filterLast, filterHeader;
    int subjOffset;

    void DestroyChain();
    int noOfItems();
    void oneLine(int);
    searchtype oneSearch(int, const char *);
    void NextUnread();
    void PrevUnread();
    void askFilter();
    void extrakeys(int);

  public:
    LetterListWindow();
    ~LetterListWindow();
    void initSort();
    void initFilterFlag();
    void getSortFilterOptions(int &, int &, const char *&, bool &, bool &);
    void setSortFilterOptions(int, int, const char *, bool, bool);
    void resetSubjOffset();
    void MakeActive();
    void Delete();
    void Quit();
    void MakeChain();
    const char *From();
    void ResetActive();
    void Select();
    void FirstUnread();
};


class LetterWindow
{
  private:
    class Line
    {
      public:
        char *text;
        chtype attrib;
        unsigned int length;
        bool reply;            // whether to use line in quoted part of reply
        char qpos;             // position of quote character in line
        Line *next;

        Line(int);
        ~Line();
    };

    char *message;
    Line **linelist;
    int noOfLines;
    struct
    {
      int area;
      int letter;                         // -1 = no letter in chain
    } in_chain;
    char tagline[TAGLINE_LENGTH + 1];
    struct
    {
      int reply_area;
      int type;
      char *to;
      bool to_latin1;
      net_address na;
      char *subject;
      bool subj_latin1;
      char *tagline;
    } LetterParam;
    bool showkludge, rot13, maxscroll;
    int maxlines;                         // height of message window
    Win *header, *text, *statbar;
    char *workbuf;
    int position;                         // line of message currently on top
    int savedPos, searchPos;              // line of last saved / searched
    int matchPos;                         // line of last match
    bool *matchCol;                       // columns of last match
    bool hideReadFlag;
#ifdef WITH_CLOCK
    time_t lasttime;
#endif
    struct
    {
      bool valid;
      int fromArea;                       // area information
      int netmail_sort;                   // letter list informatiom
      int letter_sort;                    // letter list informatiom
      const char *filter;                 // letter list informatiom
      bool filterLast;                    // letter list informatiom
      bool filterHeader;                  // letter list informatiom
      int fromLetter;                     // letter information
      int position;                       // letter information
      const char *SkipLetterList;
    } ReturnInfo;                         // for jump/returnReplies()

    int areaOfMsg();
    void Draw(drawtype = againActPos);
    void Linecounter();
#ifdef WITH_CLOCK
    void Clock();
#endif
    const char *From();
    void addNetAddr(char *);
    bool charset();
    void DrawHeader();
    void DrawBody();
    void DrawStatbar();
    int getLineQuotePos(const char *);
    chtype getLineColour(const char *, bool &, bool &, char &);
    void MakeChain(int);
    void DestroyChain();
    void oneLine(int);
    void Previous();
    void Move(int);
    void checkLetterFilter();
    void jumpReplies(int);
    bool EnterHeader(char *, char *, char *, bool, bool, bool, net_address *,
                     int *, bool);
    bool okArea();
    int QuotedTextPercentage(const char *);
    void mkrepfname(char *);
    void QuoteText(const char *);
    void ForwardText(const char *);
    void WriteText(FILE *, bool, bool);
    void Editor(const char *);
    long fileconv(const char *, fconvtype);
    void EditLetter();
    bool EditBody();
    void SaveText(FILE *, bool);

  public:
    LetterWindow();
    ~LetterWindow();
    void MakeActive(bool);
    void Delete();
    void Touch();
    void Reset();
    void savePos();
    void restorePos();
    void setPos(int, bool);
    searchtype search(const char *);
    void Next();
    void returnReplies(bool);
    void KeyHandle(int);
    void setLetterParam(int, int);
    void setLetterParam(const char *, bool, net_address &,
                        const char * = NULL, bool = false);
    void setLetterParam(const char *);
    void EnterLetter();
    bool okPrint();
    bool Save(int, bool);
};


class AddressBook : public ListWindow
{
  private:
    class Person
    {
      public:
        char name[OTHERFROMTOLEN + 1];
        net_address address;
        char *subject;
        Person *next;

        Person();
        ~Person();
        void setName(const char *);
        const char *getAddress() const;
    };

    bool addrsort, addrdisp, fake, highlight;
    int noOfPersons, noOfListed;
    Person **person, *first;
    int *listed;
    char *filter;
    bool inLetter;
    int len1, len2;
    char format[25];

    void MakeChain();
    void DestroyChain();
    int noOfItems();
    void oneLine(int);
    searchtype oneSearch(int, const char *);
    bool isDupe(Person *);
    void writeAddress(Person *, FILE *);
    void writeAddresses(int, bool, Person * = NULL);
    void newAddress(const char *, const char *);
    void gotoAddress(const char *);
    void PickAddress();
    void EditSubject(Person *);
    void askFilter();
    void extrakeys(int);

    friend int asortbyname(const void *, const void *);

  public:
    AddressBook();
    ~AddressBook();
    void MakeActive();
    void Delete();
    void Quit();
    void EnterAddress(Person * = NULL);
};

int asortbyname(const void *, const void *);


class TaglineWindow : public ListWindow
{
  private:
    class Tagline
    {
      public:
        char text[TAGLINE_LENGTH + 1];
        Tagline *next;

        Tagline();
    };

    bool isActive, tagsort, fake;
    int noOfTaglines, noOfListed;
    Tagline **tagline, *first;
    int *listed;
    char *filter;
    char format[11];

    void MakeChain();
    void DestroyChain();
    int noOfItems();
    void oneLine(int);
    searchtype oneSearch(int, const char *);
    void gotoTagline(const char *);
    void writeTaglines(int, bool, const char * = NULL);
    void RandomTagline();
    void askFilter();
    void extrakeys(int);

    friend int tsortbytext(const void *, const void *);

  public:
    TaglineWindow();
    ~TaglineWindow();
    void MakeActive();
    void Delete();
    void Quit();
    void EnterTagline(const char * = NULL);
};

int tsortbytext(const void *, const void *);


class AnsiWindow
{
  private:
    class AnsiLine
    {
      private:
        AnsiLine *prev, *next;

      public:
        int length;
        chtype *text;

        AnsiLine(int = 0, AnsiLine * = NULL);
        ~AnsiLine();
        AnsiLine *getprev() const;
        AnsiLine *getnext(int = 0);
    };

    class stringstream
    {
      private:
        FILE *file;
        const unsigned char *string, *pos;

      public:
        void init(FILE *);
        void init(const unsigned char *);
        void rewind();
        unsigned char fetc();
        void retc(unsigned char);
        bool isEnd() const;
    };

    stringstream source;               // data to be displayed
    const char *description;
    bool latin1;
    int position;                      // line currently displayed at top
    int savedPos, searchPos;           // line of last saved / searched
    int matchPos;                      // line of last match
    bool *matchCol;                    // columns of last match
    int maxlines;                      // of the text window
    Win *header, *text, *animtext;
    bool anim, animBreak;              // animation mode?
    int cpx, cpy;                      // ANSI cursor positions
    int lpy;                           // last y position
    int spx, spy;                      // stored ANSI cursor positions
    bool a_bold, a_blink, a_reverse;   // attributes
    chtype a_fg, a_bg;                 // colors
    chtype attrib;                     // current attribute
    AnsiLine *head, *curr, **linelist;
    int noOfLines;
    int baseline;                      // base for y positions in non-anim mode
    char escparm[32];                  // temp copy of ESC sequence parameters

    void DrawHeader(const char * = NULL);
    void DrawBody();
    void pos_reset();
    void attrib_reset();
    void MakeChain();
    void DestroyChain();
    void oneLine(int);
    int getparm();
    void check_pos();
    void attrib_set();
    void esc();
    void output(unsigned char);
    void animate();

  public:
    AnsiWindow();
    ~AnsiWindow();
    void set(const char *, const char *, bool);
    void set(FILE *, const char *, bool);
    void MakeActive();
    void Delete();
    void savePos();
    void restorePos();
    void setPos(int, bool);
    searchtype search(const char *);
    void KeyHandle(int);
};


class Interface
{
  private:
    UserInterface *ui;
    Color *colorlist;
    Win *input, *screen, *welcome;
    statetype prevstate, state, fromstate;
    ListWindow *currlist;
    HelpWindow helpwin;
    LittleAreaListWindow littleareas;
    AddressBook addresses;
    int Key, lalreturn, keyHandler;
    bool unsaved_reply, any_read, any_marked, beeper, exitNow, quitNow;
    static const char *dos2isotab;
    static const char *iso2dostab;
    static const char *win2isotab;
    bool isoDisplay, isoToggle;
    char *charconv_buf;
    char search_for[QUERYINPUT + 1];
    bool shadow, search_first;

    void endstate(statetype);
    void newstate(statetype);
    void newKeyHandle(statetype, ListWindow *);
    void startReading();
    bool saveLastread();
    int checkReplies();
    bool saveReplies();
    void search();
    void user_program();
    char *charconv(char *, convtype);

  public:
    ServiceListWindow services;
    FileListWindow files;
    FileDBListWindow filedbs;
    ReplyMgrListWindow systems;
    AreaListWindow areas;
    BulletinListWindow bulletins;
    OfflineConfigListWindow offlineconfs;
    LetterListWindow letters;
    LetterWindow letterwin;
    TaglineWindow taglines;
    AnsiWindow ansiwin;

    Interface();
    ~Interface();
    void mainwin(bool = true);
    int WarningWindow(const char *, const char ** = NULL, int = 2);
    void ErrorWindow(const char *, bool = true);
    void InfoWindow(const char *, int = 2);
    int QueryBox(const char *, char *, int, bool = true);
    statetype getstate() const;
    statetype getprevstate() const;
    void changestate(statetype);
    void update(bool = true);
    bool Select();
    bool back();                // returns true if we have to quit the program
#ifdef SIGWINCH
    void sigwinch();
#endif
    void ansiView(const char *, const char *, bool);
    void ansiView(FILE *, const char *, bool);
    bool isLatin1() const;
    void charsetToggle();
    void setUnsaved();
    void setSaved();
    bool isUnsaved() const;
    void setAnyRead();
    void setAnyMarked();
    void delay_beep();
    void KeyHandle();
    int midpos(int, int, int, int) const;
    int endpos(int, int) const;
    void syscallwin();
    void endsyscall();
    UserInterface *getUI() const;
    int selectArea(int);
    void shadowedWin(bool);
    bool isoToggleSet(bool);
    char *charconv_in(char *, bool);
    char *charconv_out(char *, bool);
    char charconv_in(char, bool);
    char charconv_out(char, bool);
    char *charconv_in(bool, const char *);
    chtype charconv_in(bool, char);
};


// global variables defined in main.cc
extern Interface *interface;
extern bmail bm;

#ifdef WITH_CLOCK
extern time_t starttime;
#endif


#endif
