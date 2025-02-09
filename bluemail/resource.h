/*
 * blueMail offline mail reader
 * resource class

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef RESOURCE_H
#define RESOURCE_H


#include <stdio.h>


#if defined(__MSDOS__)
  #define DEFEDIT "edit"
#elif defined(__CYGWIN__) || defined(__MINGW32__)
  #define DEFEDIT "start /w notepad"
#elif defined(__EMX__)
  #define DEFEDIT "tedit"
#else
  #define DEFEDIT "vi"
#endif

#if defined(__MSDOS__)
  #define DEFARJ "arj a -e -hf"
  #define DEFUNARJ "arj e"
  #define DEFZIP "pkzip"
  #define DEFUNZIP "pkunzip"
  #define DEFLHA "lha a /m"
  #define DEFUNLHA "lha e"
#elif defined(__CYGWIN__) || defined(__MINGW32__)
  #define DEFARJ "arj a -e"
  #define DEFUNARJ "arj e"
  #define DEFZIP "zip -j"
  #define DEFUNZIP "unzip -j -o -L"
  #define DEFLHA "lha a /m"
  #define DEFUNLHA "lha e"
#else
  #define DEFUNARJ "unarj e"
  #define DEFZIP "zip -j"
  #define DEFUNZIP "unzip -j -o -L"
  #define DEFLHA "lha a"
  #define DEFUNLHA "lha efi"
#endif

#if defined(__MSDOS__) || defined(__EMX__)
  #define RCNAME "bmail.rc"
  #define ADDRBOOK "address.bk"
#else
  #define RCNAME ".bmailrc"
  #define ADDRBOOK "addressbook"
#endif


enum OptionIDs
{
  internalFIRST,                                 /* internally used options */
    PacketWorkDir = internalFIRST,
    ReplyWorkDir,
    InfWorkDir,
    PacketName,
    InfName,
    BBSName,
    SysOpName,
    LoginName,
    AliasName,
    hasLoginName,
    AddrPart,
  internalLAST = AddrPart,
  dirFIRST,
    HomeDir = dirFIRST,
    BmailDir,
    PacketDir,
    ReplyDir,
    SaveDir,
    InfDir,
    WorkDir,
    HudsonMsgBaseDir,
    BBBSMsgBaseDir,
    MboxesDir,
  dirLAST = MboxesDir,
  fileFIRST,
    addressbookFile = fileFIRST,
    taglineFile,
    signatureFile,
    statisticsFile,
    mboxFile,
    AreasHMBFile,
  fileLAST = AreasHMBFile,
  cmdFIRST,
    arjUncompressCommand = cmdFIRST,
    zipUncompressCommand,
    lhaUncompressCommand,
    unknownUncompressCommand,
    arjCompressCommand,
    zipCompressCommand,
    lhaCompressCommand,
    unknownCompressCommand,
    editorCommand,
    printCommand,
    userpgmCommand,
  cmdLAST = userpgmCommand,
  stringFIRST,
    UserName = stringFIRST,
    EmailAddress,
    QuoteHeaderFido,
    QuoteHeaderInternet,
    ToAll,
    UpperLower,
    Origin,
    Organization,
    IsPersonal,
    ReplyExtension,
  stringLAST = ReplyExtension,
  settFIRST,
    ConsoleCharset = settFIRST,
    StartupService,
    SortFilesBy,
    SortLettersBy,
    SortNetmailBy,
    SortSystemsBy,
    QuoteOMeter,
    ClockMode,
    MIMEBody,
    OverlongReplyLines,
  settLAST = OverlongReplyLines,
  miscFIRST,
    SuppressAreaListInfo = miscFIRST,
    LongAreaList,
    SaveLastreadPointers,
    LongLetterList,
    FullsizeLetterList,
    SmartScrollLetterList,
    SkipLetterList,
    SuppressLineCounter,
    EnableSigdashes,
    DisplayKludgelines,
    BeepOnPersonalMail,
    SkipTaglineBox,
    StripRe,
    OmitReplyRe,
    ArrowNoQuote,
    Pos1Input,
    StripSoftCR,
    DrawSortMark,
    PersonalArea,
    LetterMaxScroll,
    Transparency,
    OmitSystem,
    SaveReplies,
    OmitDemoService,
    DrawReplyMark,
    CallReplyMgr,
    OmitBulletins,
    SortAddressbook,
    ClearFilter,
    BBBSUser_1,
    IgnoreNDX,
    OmitEmptyQuotes,
    SaveAreaMarks,
    OmitAreaMarkInfo,
  miscLAST = OmitAreaMarkInfo,
  colorFIRST,        /* colors must be in same order as default color array */
    colorMainBorder = colorFIRST,
    colorMainBack,
    colorMainBottSep,
    colorWelcBorder,
    colorWelcHeader,
    colorWelcText,
    colorHelpBorder,
    colorHelpText,
    colorHelpKeys,
    colorHelpDescr,
    colorServiceLBorder,
    colorServiceLTText,
    colorServiceList,
    colorFileLBorder,
    colorFileLHeader,
    colorFileList,
    colorReplyMgrLBorder,
    colorReplyMgrLHeader,
    colorReplyMgrList,
    colorReplyMgrLPacket,
    colorAreaLBorder,
    colorAreaLTText,
    colorAreaLHeader,
    colorAreaListUnread,
    colorAreaListRead,
    colorAreaLReply,
    colorAreaLInfoDescr,
    colorAreaLInfoText,
    colorBulletinLBorder,
    colorBulletinLTText,
    colorBulletinList,
    colorOfflConfLBorder,
    colorOfflConfLTText,
    colorOfflConfList,
    colorOfflConfLAdded,
    colorOfflConfLDropped,
    colorLiAreaBorder,
    colorLiAreaTText,
    colorLiAreaList,
    colorLiAreaListRo,
    colorLetterLBorder,
    colorLetterLTText,
    colorLetterLArea,
    colorLetterLHeader,
    colorLetterListUnread,
    colorLetterListRead,
    colorLetterLFromUser,
    colorLetterLToUser,
    colorLetterHBorder,
    colorLetterHClock,
    colorLetterHMsgNum,
    colorLetterHText,
    colorLetterHFrom,
    colorLetterHTo,
    colorLetterHSubject,
    colorLetterHDate,
    colorLetterHFlags,
    colorLetterHFlagsHi,
    colorLetterText,
    colorLetterQText,
    colorLetterTagline,
    colorLetterSignature,
    colorLetterTearline,
    colorLetterOrigin,
    colorLetterKludgeline,
    colorLetterBottLine,
    colorAnsiviewHeader,
    colorAnsiview,
    colorRepBoxBorder,
    colorRepBoxDescr,
    colorRepBoxText,
    colorRepBoxInp,
    colorRepBoxHelpText,
    colorFileBoxBorder,
    colorFileBoxHeader,
    colorFileBoxInp,
    colorQueryBoxBorder,
    colorQueryBoxHeader,
    colorQueryBoxInp,
    colorAddrBorder,
    colorAddrTText,
    colorAddrHeader,
    colorAddrList,
    colorAddrInp,
    colorAddrKeys,
    colorAddrDescr,
    colorTagBoxBorder,
    colorTagBoxTText,
    colorTagBoxList,
    colorTagBoxInp,
    colorTagBoxKeys,
    colorTagBoxDescr,
    colorWarnText,
    colorWarnKeys,
    colorInfoText,
    colorSysCallHeader,
    colorSearchResult,
    colorShadow,
  colorLAST = colorShadow,
  maxOptions = colorLAST + 1     /* No. of last ID + 1 */
};


const char *const OptionNames[maxOptions] =
{
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "",
  "home",
  "bmail",
  "packet",
  "reply",
  "save",
  "inf",
  "work",
  "HudsonMsgBase",
  "BBBSMsgBase",
  "mboxes",
  "addressbook",
  "taglines",
  "signature",
  "statistics",
  "mbox",
  "AreasHMB",
  "arjUncompress",
  "zipUncompress",
  "lhaUncompress",
  "unknownUncompress",
  "arjCompress",
  "zipCompress",
  "lhaCompress",
  "unknownCompress",
  "editor",
  "print",
  "userpgm",
  "UserName",
  "EmailAddress",
  "QuoteHeaderFido",
  "QuoteHeaderInternet",
  "ToAll",
  "UpperLower",
  "Origin",
  "Organization",
  "IsPersonal",
  "ReplyExtension",
  "ConsoleCharset",
  "StartupService",
  "SortFilesBy",
  "SortLettersBy",
  "SortNetmailBy",
  "SortSystemsBy",
  "Quote-O-Meter",
  "ClockMode",
  "MIMEBody",
  "OverlongReplyLines",
  "SuppressAreaListInfo",
  "LongAreaList",
  "SaveLastreadPointers",
  "LongLetterList",
  "FullsizeLetterList",
  "SmartScrollLetterList",
  "SkipLetterList",
  "SuppressLineCounter",
  "EnableSigdashes",
  "DisplayKludgelines",
  "BeepOnPersonalMail",
  "SkipTaglineBox",
  "StripRe",
  "OmitReplyRe",
  "ArrowNoQuote",
  "Pos1Input",
  "StripSoftCR",
  "DrawSortMark",
  "PersonalArea",
  "LetterMaxScroll",
  "Transparency",
  "OmitSystem",
  "SaveReplies",
  "OmitDemoService",
  "DrawReplyMark",
  "CallReplyMgr",
  "OmitBulletins",
  "SortAddressbook",
  "ClearFilter",
  "BBBSUser#1",
  "IgnoreNDX",
  "OmitEmptyQuotes",
  "SaveAreaMarks",
  "OmitAreaMarkInfo",
  "MainBorder",
  "MainBackground",
  "MainBottomSeparator",
  "WelcomeBorder",
  "WelcomeHeader",
  "WelcomeText",
  "HelpBorder",
  "HelpText",
  "HelpKeys",
  "HelpDescription",
  "ServiceListBorder",
  "ServiceListTopText",
  "ServiceList",
  "FileListBorder",
  "FileListHeader",
  "FileList",
  "ReplyMgrListBorder",
  "ReplyMgrListHeader",
  "ReplyMgrList",
  "ReplyMgrListPacket",
  "AreaListBorder",
  "AreaListTopText",
  "AreaListHeader",
  "AreaListUnread",
  "AreaListRead",
  "AreaListReply",
  "AreaListInfoDescription",
  "AreaListInfoText",
  "BulletinListBorder",
  "BulletinListTopText",
  "BulletinList",
  "OfflineConfListBorder",
  "OfflineConfListTopText",
  "OfflineConfList",
  "OfflineConfListAdded",
  "OfflineConfListDropped",
  "LittleAreaListBorder",
  "LittleAreaListTopText",
  "LittleAreaList",
  "LittleAreaListReadonly",
  "LetterListBorder",
  "LetterListTopText",
  "LetterListArea",
  "LetterListHeader",
  "LetterListUnread",
  "LetterListRead",
  "LetterListFromUser",
  "LetterListToUser",
  "LetterHeaderBorder",
  "LetterHeaderClock",
  "LetterHeaderMsgnum",
  "LetterHeaderText",
  "LetterHeaderFrom",
  "LetterHeaderTo",
  "LetterHeaderSubject",
  "LetterHeaderDate",
  "LetterHeaderFlags",
  "LetterHeaderFlagsHigh",
  "LetterText",
  "LetterQuotedText",
  "LetterTagline",
  "LetterSignature",
  "LetterTearline",
  "LetterOrigin",
  "LetterKludgeline",
  "LetterBottomline",
  "AnsiviewHeader",
  "Ansiview",
  "ReplyBoxBorder",
  "ReplyBoxDescription",
  "ReplyBoxText",
  "ReplyBoxInput",
  "ReplyBoxHelpText",
  "FileBoxBorder",
  "FileBoxHeader",
  "FileBoxInput",
  "QueryBoxBorder",
  "QueryBoxHeader",
  "QueryBoxInput",
  "AddressbookBorder",
  "AddressbookTopText",
  "AddressbookHeader",
  "AddressbookList",
  "AddressbookInput",
  "AddressbookKeys",
  "AddressbookDescription",
  "TaglineBoxBorder",
  "TaglineBoxTopText",
  "TaglineBoxList",
  "TaglineBoxInput",
  "TaglineBoxKeys",
  "TaglineBoxDescription",
  "WarningText",
  "WarningKeys",
  "InfoText",
  "SystemCallHeader",
  "SearchResult",
  "Shadow",
};


class resource
{
  private:
    char *resourceData[maxOptions];

    void freeAll();
    bool checkPath(const char *, bool);
    bool taglineFileCheck();
    bool verifyPaths();
    void createConfigBlock(FILE *, int, int, const char *);
    bool createNewConfig(const char *);
    void parseConfig(FILE *);
    bool checkValue(int, const char *);
    void bmHomeSub(int, const char *);
    void subWorkPath(int, const char *);
    void initinit();
    void workinit();
    void bmHomeInit();

  public:
    resource();
    ~resource();
    const char *get(int) const;
    void set(int, const char *);
    bool isYes(int) const;
};


#endif
