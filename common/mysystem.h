/*
 * blueMail offline mail reader
 * version number and name #define'd, as well as
 * some operating system specific routines

 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2005 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef MYSYSTEM_H
#define MYSYSTEM_H


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


#define BM_NAME "blueMail"
#define BM_MAJOR 1
#define BM_MINOR 4
#define BM_TOP_N "%s"
#define BM_IS_WHAT "Offline Mail Reader"
#define BM_TOP_V "%d.%d"
#define BM_TOPHEADER BM_TOP_N " " BM_IS_WHAT " " BM_TOP_V


#define MYMAXPATH 256
#define MYMAXLINE 128

#define TMPFILEPATTERN "bmXXXXXX"


#if defined(__MSDOS__) || defined(__EMX__) || defined(__CYGWIN__) || defined(__MINGW32__)
  #define DOSPATHTYPE
  #define ALL_FILES_PATTERN "*.*"
  #define SH_SCRIPT "@echo off"
  #define SCRIPT_BAT
#else
  #define ALL_FILES_PATTERN "*"
  #define SH_SCRIPT "#!/bin/sh"
#endif


void myinit();

/* Some of the functions normally used by blueMail don't exist in all
   environments, some are available under other names: */

#if defined(__EMX__)
  #define strcasecmp stricmp
  #define strncasecmp strnicmp
  extern "C"
  {
    int _chdir2(__const__ char *);
    char *_getcwd2(char *, int);
  }
  #define mychdir _chdir2
  #define mygetcwd _getcwd2
  #define mygetenv getenv
  #define mymkdir mkdir
  #define myputenv putenv
  #define mysleep sleep
#elif defined(__CYGWIN__)
  #define mychdir chdir
  char *mygetcwd(char *, size_t);
  char *mygetenv(const char *);
  #define mymkdir mkdir
  #define myputenv putenv
  #define mysleep sleep
#elif defined(__MINGW32__)
  #define mychdir chdir
  #define mygetcwd getcwd
  #define mygetenv getenv
  #define mymkdir(p, m) mkdir(p)
  #define myputenv putenv
  #define mysleep(x) _sleep((x) * 1000)
#else
  #define mychdir chdir
  #define mygetcwd getcwd
  #define mygetenv getenv
  #define mymkdir mkdir
  #define myputenv putenv
  #define mysleep sleep
#endif


#ifdef __CYGWIN__
  int cygwinsystem(const char *);
#endif
int mysystem(const char *);
void myshell();
char *mytmpname(const char *);
char *mymktmpfile(const char *);
char *mymktmpdir(const char *);
const char *sysname();
FILE *fxopen(const char *, const char *);
bool isIso(const char *);
const char *myenvf();
#ifdef __MSDOS__
  int myrename(const char *, const char *);
#else
  #define myrename rename
#endif


#endif
