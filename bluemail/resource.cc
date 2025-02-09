/*
 * blueMail offline mail reader
 * resource class

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <dirent.h>
#include <string.h>
#include "resource.h"
#include "driverl.h"
#include "../common/error.h"
#include "../common/auxil.h"


extern Error error;


resource::resource ()
{
  int c;
  bool ok, fok;
  FILE *configFile;
  char configFileName[MYMAXPATH];
  const char *envhome;
  const char *greeting = "\nWelcome new user!\n\n"
                         "A new (empty) " RCNAME " has been created. "
                         "If you continue by pressing <ENTER>\n"
                         "now, " BM_NAME " will use the default values. "
                         "If you wish to edit your " RCNAME "\n"
                         "first, say 'E' at the prompt. Say 'Q' to quit "
                         "(don't start " BM_NAME ").\n\n"
                         "Your choice? (<ENTER>/e/q) ";

  for (c = 0; c < maxOptions; c++) resourceData[c] = NULL;
  initinit();

  envhome = mygetenv("BMAIL");
  if (!envhome) envhome = mygetenv("HOME");
  if (!envhome) envhome = error.getOrigDir();

  set(HomeDir, fixPath(envhome));
  mkfname(configFileName, get(HomeDir), RCNAME);

  if (!(configFile = fopen(configFileName, "rt")))
  {
    if ((fok = createNewConfig(configFileName)))
    {
      printf(greeting);
      c = getchar();
      switch (toupper(c))
      {
        case 'Q':
          freeAll();
          exit(1);
          break;

        case 'E':
          mysystem(makecmd(get(editorCommand), canonize(configFileName)));
          // no break here!
        default:
          fok = ((configFile = fopen(configFileName, "rt")) != NULL);
      }
    }
    if (!fok) fatalError("Could not open " RCNAME);
  }

  parseConfig(configFile);
  fclose(configFile);

#ifndef DRV_DEMO
  // no driver, no service
  set(OmitDemoService , "Y");
#endif

  bmHomeInit();
  ok = verifyPaths();
  if (ok) ok = taglineFileCheck();
  if (!ok) fatalError("Unable to access data directories.");

  workinit();
}


resource::~resource ()
{
  clearDirectory(resourceData[PacketWorkDir]);
  clearDirectory(resourceData[ReplyWorkDir]);
  clearDirectory(resourceData[InfWorkDir]);
  mychdir("..");
  rmdir(resourceData[PacketWorkDir]);
  rmdir(resourceData[ReplyWorkDir]);
  rmdir(resourceData[InfWorkDir]);
  mychdir("..");
  rmdir(resourceData[WorkDir]);
  freeAll();
}


void resource::freeAll ()
{
  for (int i = 0; i < maxOptions; i++) delete[] resourceData[i];
}


bool resource::checkPath (const char *path, bool show)
{
  DIR *dir;

  if ((dir = opendir(path)) == NULL)
  {
    if (show) printf("Creating %s...\n", canonize(path));
    if (mymkdir(path, S_IRWXU) != 0) return false;
  }
  else closedir(dir);

  return true;
}


// create tagline file if it doesn't exist
bool resource::taglineFileCheck ()
{
  FILE *tagf;
  const char **p;
  bool success = false;
  static const char *defaultTaglines[] =
  {
    BM_NAME ", the new multi-platform, multi-format offline mail reader!",
    NULL
  };

  if ((tagf = fopen(resourceData[taglineFile], "rt"))) success = true;
  else
  {
    printf("Creating %s...\n", canonize(resourceData[taglineFile]));
    if ((tagf = fopen(resourceData[taglineFile], "wt")))
    {
      for (p = defaultTaglines; *p; p++) fprintf(tagf, "%s\n", *p);
      success = true;
    }
  }

  if (success) success = (fclose(tagf) == 0);

  return success;
}


bool resource::verifyPaths ()
{
  if (checkPath(resourceData[BmailDir], true))
    if (checkPath(resourceData[PacketDir], true))
      if (checkPath(resourceData[ReplyDir], true))
        if (checkPath(resourceData[SaveDir], true))
          if (checkPath(resourceData[InfDir], true))
            return true;
  return false;
}


void resource::createConfigBlock (FILE *F, int from, int to, const char *section)
{
  fprintf(F, "\n[%s]\n", section);
  for (int i = from; i <= to; i++) fprintf(F, "#%s:\n", OptionNames[i]);
}


bool resource::createNewConfig (const char *cfgname)
{
  FILE *bmrc;
  const char **p;
  static const char *bmailrcHeader[] =
  {
    "# This is the " BM_NAME " configuration file.",
    "#",
    "# The format of an option is name: value.",
    "#",
    "# Spaces, tabs and blank lines will be ignored.",
    "# Lines beginning with # or [ are comments.",
    NULL
  };

  printf("Creating %s...\n", canonize(cfgname));

  if ((bmrc = fopen(cfgname, "wt")))
  {
    for (p = bmailrcHeader; *p; p++) fprintf(bmrc, "%s\n", *p);
    createConfigBlock(bmrc, dirFIRST, dirLAST, "Directories");
    createConfigBlock(bmrc, fileFIRST, fileLAST, "Files");
    createConfigBlock(bmrc, cmdFIRST, cmdLAST, "Commands");
    createConfigBlock(bmrc, stringFIRST, stringLAST, "Strings");
    createConfigBlock(bmrc, settFIRST, settLAST, "Settings");
    createConfigBlock(bmrc, miscFIRST, miscLAST, "Miscellaneous");
    createConfigBlock(bmrc, colorFIRST, colorLAST, "Colors");
    return (fclose(bmrc) == 0);
  }
  else return false;
}


void resource::parseConfig (FILE *configFile)
{
  char buffer[MYMAXLINE], err[58], *optName, *optValue;

  while (fgetsnl(buffer, sizeof(buffer), configFile))
  {
    if (*buffer != '#' && *buffer != '[' && *buffer != '\n')
    {
      parseOption(mkstr(buffer), ':', optName, optValue);

      for (int i = 0; i <= maxOptions; i++)
      {
        if (i == maxOptions)
        {
          sprintf(err, "Unknow option '%.32s'.", optName);
          fatalError(err);
        }

        if (strcasecmp(OptionNames[i], optName) == 0)
        {
          if (!*optValue)
          {
            sprintf(err, "Empty option '%.32s'.", optName);
            fatalError(err);
          }
          if ((i == ConsoleCharset || i == StartupService ||
               i == SortFilesBy || i == SortLettersBy ||
               i == SortNetmailBy || i == SortSystemsBy ||
               i == UpperLower || i == Origin || i == Organization ||
               i == QuoteOMeter || i == ClockMode || i == EmailAddress ||
               i == MIMEBody || i == OverlongReplyLines ||
               i == ReplyExtension)
              && !checkValue(i, optValue))
          {
            sprintf(err, "Invalid value, option '%.32s'.", optName);
            fatalError(err);
          }
          set(i, ((i >= dirFIRST && i <= dirLAST)   ||
                  (i >= fileFIRST && i <= fileLAST) ||
                  (i >= cmdFIRST && i <= cmdLAST) ? tilde_expand(fixPath(optValue))
                                                  : optValue));
          break;
        }
      }
    }
  }
}


bool resource::checkValue (int ID, const char *optValue)
{
  int value, name, n_len, addr, a_len;
  bool checked;

  switch (ID)
  {
    case ConsoleCharset:
      return (strcasecmp(optValue, "IBMPC") == 0 ||
              strcasecmp(optValue, "LATIN-1") == 0);

    case StartupService:
      return (strcasecmp(optValue, "PACKET") == 0 ||
              strcasecmp(optValue, "FILE") == 0 ||
              strcasecmp(optValue, "ARCHIVE") == 0 ||
              strcasecmp(optValue, "REPLY") == 0);

    case SortFilesBy:
    case SortSystemsBy:
      return (strcasecmp(optValue, "NAME") == 0 ||
              strcasecmp(optValue, "DATE") == 0);

    case SortLettersBy:
    case SortNetmailBy:
      return (strcasecmp(optValue, "SUBJECT") == 0 ||
              strcasecmp(optValue, "NUMBER") == 0 ||
              strcasecmp(optValue, "LAST NAME") == 0);

    case UpperLower:
      return ((strlen(optValue) & 1) == 0);

    case Origin:
      return (strlen(optValue) <= 40);

    case Organization:
      return (strlen(optValue) <= 64);

    case QuoteOMeter:
      for (unsigned int i = 0; i < strlen(optValue); i++)
        if (!is_digit(optValue[i]) && (optValue[i] != '%')) return false;

      value = atoi(optValue);
      return (value >= 0 && value <= 100);

    case ClockMode:
      return (
#ifdef WITH_CLOCK
              strcasecmp(optValue, "TIME") == 0 ||
              strcasecmp(optValue, "ELAPSED") == 0 ||
#endif
              strcasecmp(optValue, "OFF") == 0);

    case EmailAddress:
      parseAddress(optValue, name, n_len, addr, a_len);
      checked = isEmailAddress(optValue + addr, a_len);

      if (checked)
      {
        char *Addr = new char[a_len + 1];
        strncpy(Addr, optValue + addr, a_len);
        Addr[a_len] = '\0';
        set(AddrPart, Addr);
        delete[] Addr;
      }
      return checked;

    case MIMEBody:
      return (strcasecmp(optValue, "QUOTED-PRINTABLE") == 0 ||
              strcasecmp(optValue, "8BIT") == 0);

    case OverlongReplyLines:
      return (strcasecmp(optValue, "QUOTE") == 0 ||
              strcasecmp(optValue, "FOLD") == 0 ||
              strcasecmp(optValue, "PERMIT") == 0);

    case ReplyExtension:
      for (unsigned int i = 0; i < strlen(optValue); i++)
        if (!isalnum((unsigned char) optValue[i])) return false;

      return (strlen(optValue) <= 3);

    default:
      return false;
  }
}


const char *resource::get (int ID) const
{
  return (ID < 0 || ID >= maxOptions ? NULL : resourceData[ID]);
}


void resource::set (int ID, const char *newValue)
{
  if (ID >= 0 && ID < maxOptions)
  {
    delete[] resourceData[ID];
    resourceData[ID] = strdupplus(newValue);
  }
}


bool resource::isYes (int ID) const
{
  if ((ID < 0 || ID >= maxOptions) || !resourceData[ID]) return false;
  else return (toupper(*resourceData[ID]) == 'Y');
}


void resource::bmHomeSub (int index, const char *dirname)
{
  char path[MYMAXPATH];

  if (!resourceData[index])
  {
    mkfname(path, resourceData[BmailDir], dirname);
    set(index, path);
  }
}


void resource::subWorkPath (int index, const char *dirname)
{
  char path[MYMAXPATH];

  mkfname(path, resourceData[WorkDir], dirname);
  set(index, path);
  if (!checkPath(path, false)) fatalError("Could not create work subdir.");
}


void resource::initinit ()
{
  const char *env;

  set(UserName, "");
  set(QuoteHeaderFido, "-=> @O wrote to @R <=-@N");
  set(QuoteHeaderInternet, "On @D, @O wrote:@N");
  set(ToAll, "All");
  set(UpperLower, "");
  set(Origin, "");
  set(IsPersonal, "");
  set(QuoteOMeter, "50");
  set(ClockMode,
#ifdef WITH_CLOCK
                 "time"
#else
                 "off"
#endif
                       );
  set(MIMEBody, "quoted-printable");
  set(OverlongReplyLines, "quote");

  set(arjUncompressCommand, DEFUNARJ);
#if defined( __MSDOS__) || defined(__CYGWIN__) || defined(__MINGW32__)
  set(arjCompressCommand, DEFARJ);
#endif
  set(zipUncompressCommand, DEFUNZIP);
  set(zipCompressCommand, DEFZIP);
  set(lhaUncompressCommand, DEFUNLHA);
  set(lhaCompressCommand, DEFLHA);

  env = mygetenv("EDITOR");
  set(editorCommand, (env ? env : DEFEDIT));

  if ((env = mygetenv("MAIL"))) set(mboxFile, env);
}


void resource::workinit ()
{
  set(WorkDir, mymktmpdir(resourceData[WorkDir]));
  subWorkPath(PacketWorkDir, "downwork");
  subWorkPath(ReplyWorkDir, "upwork");
  subWorkPath(InfWorkDir, "infwork");
}


void resource::bmHomeInit ()
{
  char path[MYMAXPATH];

  if (!resourceData[BmailDir])
  {
    mkfname(path, resourceData[HomeDir], "bmail");
    set(BmailDir, path);
  }

  bmHomeSub(PacketDir, "down");
  bmHomeSub(ReplyDir, "up");
  bmHomeSub(SaveDir, "save");
  bmHomeSub(InfDir, "inf");
  bmHomeSub(addressbookFile, ADDRBOOK);
  bmHomeSub(taglineFile, "taglines");
  bmHomeSub(statisticsFile, "stats");
}
