/*
 * blueMail offline mail reader
 * some useful auxiliary routines

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef AUXIL_H
#define AUXIL_H


#include <sys/stat.h>
#include <sys/types.h>
#include "mysystem.h"

#if defined(__MSDOS__)
  #include <limits.h>
  #define SIZE_MAX SSIZE_MAX
  #define uint32_t unsigned int
#else
  #define __STDC_LIMIT_MACROS
  #include <inttypes.h>
#endif


#define BLOCKLEN 65534
#define strtokl(s, d) strtoken(s, d, true)
#define strtokr(s, d) strtoken(s, d, false)
#define fname(p) fsplit(p, true)
#define fdir(p) fsplit(p, false)
#define is_digit(c) isdigit((unsigned char) c)
#define to_upper(c) ((_c = uplow(c, 1, 0)) ? _c : toupper(c))
#define to_lower(c) ((_c = uplow(c, 0, 1)) ? _c : tolower(c))
#define strcasecoll(s1, s2) strncasecoll(s1, s2, SIZE_MAX)
#define CRC32POLYNOM 0x04C11DB7


struct CHARSETMAP
{
  const char *charset;
  bool isLatin1;
  bool is8bit;
};


extern CHARSETMAP charsetmap[];


char *cropesp(char *);
char *lcrop(char *);
char *strdupplus(const char *);
char *stralloc(const char *);
char *stem(const char *);
const char *stripRE(const char *);
void clearDirectory(const char *);
size_t slen(const char *);
int mkfname(char *, const char*, const char*);
int mkfnamext(char *, const char*, const char*);
#ifdef DOSPATHTYPE
  char *convert(const char *, char, char);
  #define canonize(x) convert(x, '/', '\\')
#else
  #define canonize(x) x
#endif
char *charcpy(const char *, char *, unsigned int, char);
char *strtoken(char *, char, bool);
char *strtokn(char *, char);
char *makecmd(const char *, const char *);
const char *tilde_expand(const char *);
const char *fixPath(const char *);
const char *extent(const char *);
unsigned int getLSBshort(const unsigned char *);
unsigned long getLSBlong(const unsigned char *);
unsigned long getMSBlong(const unsigned char *);
void putLSBshort(unsigned char *, unsigned int);
void putLSBlong(unsigned char *, unsigned long);
void putMSBlong(unsigned char *, unsigned long);
unsigned long getMKSlong(const unsigned char *);
time_t fmtime(const char *);
void fmtime(const char *, time_t);
off_t fsize(const char *);
const char *fsplit(const char *, bool);
void fcreate(const char *, const char *);
int fcopy(const char *, const char *);
char *mkstr(char *);
char *strupper(char *);
char *strlower(char *);
char *ext(const char *, const char *);
int wildcard(const char *);
void mimeDecodeHeader(const char *, char *, bool *);
const char *strrword(const char *);
char uplow(char, int, int);
int strncasecoll(const char *, const char *, size_t);
#ifndef _GNU_SOURCE
  char *strcasestr(const char *, const char *);
#endif
bool ishex(char *);
size_t fgetsnl(char *, int, FILE *);
void parseOption(char *, char, char *&, char *&);
void parseAddress(const char *, int &, int &, int &, int &);
bool isEmailAddress(const char *, int);
const char *nextArg(const char *, char *);
int getNumExt(const char *);
char *strex(int &, const char *);
char *strexcmp(const char *, const char *);
size_t utf8_decode(char *, char);
uint32_t crc32(const char *);


#endif
