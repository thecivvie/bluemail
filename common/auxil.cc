/*
 * blueMail offline mail reader
 * some useful auxiliary routines

 Copyright (c) 1996 Toth Istvan <stoty@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <stdio.h>
#include <time.h>
#include <utime.h>
#include "auxil.h"
#include "../bluemail/bmail.h"


extern bmail bm;


// take off control characters from a string and spaces from its end
char *cropesp (char *st)
{
  char *p = st;

  while (*p)
  {
    if (*p && ((unsigned char) *p < ' ')) *p = ' ';
    p++;
  }
  for (p = strrchr(st, '\0') - 1; p > st && *p == ' '; p--) ;

  *++p = '\0';
  return st;
}


// find first character
char *lcrop (char *s)
{
  char *pos = s;

  if (pos)
    while (*pos && (*pos == ' ' || *pos == '\t')) pos++;

  return pos;
}


// duplicate a string
char *strdupplus (const char *original)
{
  char *dup = NULL;

  if (original)
  {
    dup = new char[strlen(original) + 1];
    strcpy(dup, original);
  }

  return dup;
}


// allocate memory for a string
char *stralloc (const char *original)
{
  return (original ? new char[strlen(original) + 1] : NULL);
}


// return a filename without extension
char *stem (const char *fileName)
{
  static char lcnam[13];
  unsigned int e, b, l = sizeof(lcnam) - 1;

  for (e = 0; fileName[e] && (fileName[e] != '.') && (e < l); e++) ;
  for (b = 0; b < e; b++) lcnam[b] = fileName[b];
  lcnam[b] = '\0';

  return lcnam;
}


// get rid of RE:
const char *stripRE (const char *subject)
{
  while (strncasecmp(subject, "RE: ", 4) == 0) subject += 4;
  return subject;
}


// delete any file in a directory
void clearDirectory (const char *dirname)
{
  DIR *dir;
  struct dirent *entry;

  if ((dir = opendir(dirname)))
  {
    if (mychdir(dirname) == 0)
      while ((entry = readdir(dir)))
        if (*entry->d_name != '.') remove(entry->d_name);
    closedir(dir);
  }
}


// to avoid segmentation faults
size_t slen (const char *s)
{
  return (s ? strlen(s) : 0);
}


// build a path to a filename in a directory
int mkfname (char *dest, const char *dir, const char *file)
{
  char format[12];

#ifdef __MSDOS__
  #define FORMAT  "%%.%lus/%%s"
#else
  #define FORMAT  "%%.%us/%%s"
#endif

  sprintf(format, FORMAT, MYMAXPATH - strlen(file) - 2);
  return sprintf(dest, format, dir, file);
}


// build a filename from stem and extension
int mkfnamext (char *dest, const char *fstem, const char *fext)
{
  int result = mkfname(dest, fstem, fext);
  char *delim = strrchr(dest, '/');
  *delim = '.';
  return result;
}


#ifdef DOSPATHTYPE
// convert pathnames to "canonical" form (exchange slashes and backslashes)
char *convert (const char *sinner, char from, char to)
{
  static char saint[MYMAXPATH];
  int i;

  for (i = 0; sinner[i]; i++) saint[i] = (sinner[i] == from ? to : sinner[i]);

  saint[i] = '\0';
  return saint;
}
#endif


/* Copy a character to position pos into buffer, but not more than maxchar
   to prevent buffer from overflowing. Buffer must have length maxchar+1.
   If ok, return next pos. */
char *charcpy (const char *buffer, char *pos, unsigned int maxchar, char c)
{
  *pos = '\0';
  if (strlen(buffer) < maxchar) *pos++ = c;
  return pos;
}


// smart strtok routine
char *strtoken (char *string, char delim, bool left)
{
  static char buffer[MYMAXLINE];
  char *str, *buf;

  // lame, but protects against buffer overflow
  if (strlen(string) >= sizeof(buffer))
    return (left ? string : &string[strlen(string)]);

  str = string;
  buf = buffer;

  while (*str && (*str != delim)) *buf++ = *str++;
  *buf = '\0';
  if (*str) str++;

  if (left) return buffer;
  else return str;
}


// basically the same as strtok, but returns empty tokens, too
char *strtokn (char *string, char delim)
{
  static char *last, *next;

  if (string) last = next = string;     // initialize
  else last = next;

  while (*next && (*next != delim)) next++;

  if (*next) *next++ = '\0';

  return last;
}


// build a command string with argument
char *makecmd (const char *cmd, const char *arg)
{
  static char command[MYMAXPATH + MYMAXPATH];
  char format[18];

  sprintf(format, "%%.%us %%.%us", MYMAXPATH - 1, MYMAXPATH - 1);
  sprintf(command, format, cmd, arg);
  return command;
}


// expand ~/ in pathnames
const char *tilde_expand (const char *path)
{
  const char *home = mygetenv ("HOME");
  static char result[MYMAXPATH];

  if (path && home && (*path == '~') && (path[1] == '/'
#ifdef DOSPATHTYPE
                                                    || path[1] == '\\'
#endif
                                                                      ) &&
      (strlen(home) + strlen(path) - 1 < MYMAXPATH))
  {
    sprintf(result, "%s%s", home, path + 1);
    return result;
  }
  else return path;
}


// for consistency, no path should end in a slash
const char *fixPath (const char *path)
{
  size_t l;
  static char result[MYMAXPATH];

  if ((l = strlen(path)) > 0)
  {
    char d = path[l - 1];

    if (d == '/'
#ifdef DOSPATHTYPE
                 || d == '\\'
#endif
                             )
    {
      strncpy(result, path, MYMAXPATH);
      result[(l < MYMAXPATH ? l : MYMAXPATH) - 1] = '\0';
      return result;
    }
  }

  return path;
}


// append '.' if file has no extension
const char *extent (const char *path)
{
  bool charfound = false;
  size_t len = strlen(path);
  static char result[MYMAXPATH];

  for (int i = len - 1; i >= 0; i--)
  {
    if (path[i] == '/'
#ifdef DOSPATHTYPE
                       || path[i] == '\\'
#endif
                                         ) break;
    if (path[i] == '.') return path;
    charfound = true;
  }

  strcpy(result, path);
  if (charfound && (len < MYMAXPATH - 1)) strcat(result, ".");
  return result;
}


// get a little endian short as integer
unsigned int getLSBshort (const unsigned char *x)
{
  return ((unsigned int) x[1] << 8) + (unsigned int) x[0];
}


// get a little endian long as long
unsigned long getLSBlong (const unsigned char *x)
{
  return ((unsigned long) x[3] << 24) + ((unsigned long) x[2] << 16) +
         ((unsigned long) x[1] << 8) + (unsigned long) x[0];
}


// get a big endian long as long
unsigned long getMSBlong (const unsigned char *x)
{
  return ((unsigned long) x[0] << 24) + ((unsigned long) x[1] << 16) +
         ((unsigned long) x[2] << 8) + (unsigned long) x[3];
}


// put an integer into a little endian short
void putLSBshort (unsigned char *dest, unsigned int source)
{
  dest[0] = source & 0xFF;
  dest[1] = (source & 0xFF00) >> 8;
}


// put a long into a little endian long
void putLSBlong (unsigned char *dest, unsigned long source)
{
  dest[0] = source & 0xFF;
  dest[1] = (source & 0xFF00) >> 8;
  dest[2] = (source & 0xFF0000) >> 16;
  dest[3] = (source & 0xFF000000) >> 24;
}


// put a long into a big endian long
void putMSBlong (unsigned char *dest, unsigned long source)
{
  dest[0] = (source & 0xFF000000) >> 24;
  dest[1] = (source & 0xFF0000) >> 16;
  dest[2] = (source & 0xFF00) >> 8;
  dest[3] = source & 0xFF;
}


// put a float in Microsoft MKS binary format into a long
unsigned long getMKSlong (const unsigned char *mks)
{
  unsigned long m = (unsigned long) mks[0] +
                    ((unsigned long) mks[1] << 8) +
                    ((unsigned long) mks[2] << 16);

  return (m | 0x800000L) >> (24 + 0x80 - mks[3]);
}


// return file modification time
time_t fmtime (const char *file)
{
  struct stat fstat;

  return (stat(file, &fstat) == 0 ? fstat.st_mtime : 0);
}


// set a file's modification time
void fmtime (const char *file, time_t mtime)
{
  struct utimbuf ut;

  time(&ut.actime);
  ut.modtime = mtime;
  utime(file, &ut);
}


// return file size
off_t fsize (const char *file)
{
  struct stat fstat;

  return (stat(file, &fstat) == 0 ? fstat.st_size : -1);
}


// return the filename or directory part of a path
const char *fsplit (const char *path, bool filename)
{
  static char dir[MYMAXPATH + 1];
  strncpy (dir, path, MYMAXPATH);
  dir[MYMAXPATH] = '\0';
  int slen = strlen(dir);

  while (--slen >= 0)
  {
    char p = dir[slen];
    if (p == '/' || p == '\\') break;
  }

  if (filename) return dir + slen + 1;
  else
  {
    if (slen < 0) dir[++slen] = '.';
    if (slen == 0) slen++;
    dir[slen] = '\0';
    return dir;
  }
}


// create an empty file in a directory
void fcreate (const char *dir, const char *file)
{
  char fname[MYMAXPATH];
  FILE *f;

  mkfname(fname, dir, file);
  if ((f = fopen(fname, "wb"))) fclose(f);
}


// copy a file, return error(level)
int fcopy (const char *from, const char *to)
{
  FILE *src = NULL, *dest = NULL;
  size_t len;

  src = fopen(from, "rb");
  dest = fopen(to, "wb");

  if (!src || !dest)
  {
    if (src) fclose(src);
    if (dest) fclose(dest);
    return 1;   // error(level)
  }

  char *buffer = new char[BLOCKLEN];

  while ((len = fread(buffer, sizeof(char), BLOCKLEN, src)))
    fwrite(buffer, sizeof(char), len, dest);

  delete[] buffer;
  fclose(src);
  fclose(dest);

  return 0;   // error(level)
}


// get rid of newline (and cr) character
char *mkstr (char *line)
{
  char *pos = strrchr(line, '\n');

  if (pos)
  {
    *pos-- = '\0';
    if (pos >= line && *pos == '\r') *pos = '\0';
  }

  return line;
}


// convert a string to upper case
char *strupper (char *s)
{
  char _c, *p = s;

  while (*p)
  {
    *p = to_upper(*p);
    p++;
  }
  return s;
}


// convert a string to lower case
char *strlower (char *s)
{
  char _c, *p = s;

  while (*p)
  {
    *p = to_lower(*p);
    p++;
  }
  return s;
}


// change fname's extension to new_ext
char *ext (const char *fname, const char *new_ext)
{
  static char result[13];

  sprintf(result, "%.8s.%.3s", stem(fname), new_ext);
  return result;
}


// check whether filename contains wildcard and tell fname's length
int wildcard (const char *fname)
{
  int pos = strlen(fname) - 1;

  return (pos >= 0 && fname[pos] == '*' ? pos : -1);
}


// MIME charsets we can handle
CHARSETMAP charsetmap[] = {{"UTF-8", true, false},
                           {"ISO-8859-1", true, true},
                           {"ISO-8859-15", true, true},
                           {"WINDOWS-1252", true, true},
                           {"CP1252", true, true},
                           {"US-ASCII", false, true},
                           {"IBM437", false, true},
                           {"CP437", false, true},
                           {"X-CP437", false, true},
                           {NULL, false, true}};

// decode a RFC2047 encoded header field
// and report whether encoding was ISO 8859-1
void mimeDecodeHeader (const char *source, char *dest, bool *latin1)
{
  int i, qp, b64idx = 0;
  char c, t = '\0', b[4];
  const char *cset, *b64char;
  size_t lenc, lens;
  char *endpos = NULL, *startpos = dest;
  bool insideQ = false, insideB = false, is8bit = true;

  // Base64 alphabet
  static const char *b64alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz"
                                   "0123456789+/";

  if (source)
  {
    while (*source)
    {
      c = *source++;

      // start of encoding?
      if ((c == '=' && *source == '?') && !insideB)
      {
        i = 0;
        lens = strlen(source);

        while ((cset = charsetmap[i++].charset))
        {
          lenc = strlen(cset);

          if (// source long enough
              lens >= lenc + 4 &&
              // known character set
              strncasecmp(source + 1, cset, lenc) == 0 &&
              // well-formed
              source[lenc + 1] == '?' && source[lenc + 3] == '?' &&
              // encoding type Q or B
              ((t = toupper(source[lenc + 2])) == 'Q' || t == 'B'))
          {
             if (t == 'Q') insideQ = true;
             else insideB = true;

             if (endpos) dest = endpos;
             endpos = NULL;

             b64idx = 0;
             source += lenc + 4;
             *latin1 = charsetmap[--i].isLatin1;
             is8bit = charsetmap[i].is8bit;
             break;
          }
        }

        // wasn't start of encoding
        if (!cset) *dest++ = c;
      }

      // end of encoding?
      else if (c == '?' && *source == '=')
      {
        if (insideQ || insideB)
        {
          source++;
          endpos = dest;
        }
        else
        {
          *dest++ = c;
          endpos = NULL;
        }

        insideQ = insideB = false;
      }

      // inside quoted-printable?
      else if (insideQ)
      {
        switch (c)
        {
          case '=':
            qp = 0;
            b[1] = b[2] = '\0';
            b[0] = *source;
            if (*source) b[1] = *++source;
            sscanf(b, "%x", (unsigned int *) &qp);
            if (qp != 0) *dest++ = qp;
            if (*source) source++;
            break;

          case '_':
            *dest++ = ' ';
            break;

          default:
            *dest++ = c;
        }
      }

      // inside base64?
      else if (insideB)
      {
        if ((b64char = strchr(b64alphabet, c)))
        {
          b[b64idx] = b64char - b64alphabet;

          switch (b64idx++)
          {
            case 0:
              break;

            case 1:
              *dest++ = (b[0] << 2 | b[1] >> 4);
              break;

            case 2:
              *dest++ = (b[1] << 4 | b[2] >> 2);
              break;

            case 3:
              *dest++ = (b[2] << 6 | b[3]);
              b64idx = 0;
              break;
          }
        }
      }

      // not an encoded part
      else
      {
        *dest++ = c;
        if (c != ' ' && c != '\t') endpos = NULL;
      }
    }
  }

  *dest = '\0';
  if (!is8bit) (void) utf8_decode(startpos, '\0');
}


// get rightmost word in string
const char *strrword (const char *str)
{
  const char *p = strrchr(str, ' ');
  return (p ? ++p : str);
}


// upper and lower case conversion for non-ASCII characters
char uplow (char c, int look, int take)
{
  const char *map = bm.resourceObject->get(UpperLower);
  // this map must have an even length

  if (*map)
    for (int i = 0; map[i]; i += 2)
      if (map[i + look] == c) return map[i + take];

  return '\0';
}


// a case-insensitive strncoll()
int strncasecoll (const char *s1, const char *s2, size_t n)
{
  char *p1, *p2;
  int result;

  size_t n1 = strlen(s1);
  size_t n2 = strlen(s2);

  if (n < n1) n1 = n;
  if (n < n2) n2 = n;

  p1 = new char[n1 + 1];
  p2 = new char[n2 + 1];

  strncpy(p1, s1, n1);
  strncpy(p2, s2, n2);

  p1[n1] = p2[n2] = '\0';

  strlower(p1);
  strlower(p2);

  result = strcoll(p1, p2);

  delete[] p1;
  delete[] p2;

  return result;
}


#ifndef _GNU_SOURCE
// case-insensitive version of strstr()
char *strcasestr (const char *s1, const char *s2)
{
  char _c;
  const char *p, *q = s2, *start = NULL;

  if (*q)
  {
    for (p = s1; *p; p++)
    {
      if (start)
      {
        if (!*q) break;

        if (to_lower(*p) == to_lower(*q)) q++;
        else start = NULL;
      }
      else if (to_lower(*p) == to_lower(*s2))
      {
        q = s2 + 1;
        start = p;
      }
    }
  }

  return (!*q ? (char *) start : NULL);
}
#endif


// check whether two hex digits follow
bool ishex (char *digits)
{
  if (digits[0] == '\0' || digits[1] == '\0') return false;
  else return (isxdigit((unsigned char) digits[0]) &&
               isxdigit((unsigned char) digits[1]));
}


// basically the same as fgets, but if the buffer is too small (i.e. '\n'
// was not read from stream, it reads on until the end of line is reached;
// returns count of characters read
size_t fgetsnl (char *s, int n, FILE *stream)
{
  int c;

  size_t len = slen(fgets(s, n, stream));

  if (len && !strchr(s, '\n'))
    do
    {
      if ((c = fgetc(stream)) == EOF) break;
      else len++;
    }
    while (c != '\n');

  return len;
}


// parse an option line, return name and value
void parseOption (char *line, char delim, char *&name, char *&value)
{
  char *pos = lcrop(line);

  // option name
  name = pos;
  while (*pos && (*pos != delim && *pos != ' ' && *pos != '\t')) pos++;

  if (*pos) *pos++ = '\0';

  // chars between strings
  while (*pos && (*pos == ' ' || *pos == '\t' || *pos == delim)) pos++;

  // option value
  value = pos;
  cropesp(value);
}


// find name part and address part of address
void parseAddress (const char *na, int &name, int &n_len,
                                   int &addr, int &a_len)
{
  char *pos;

  n_len = a_len = 0;

  if ((pos = strchr((char*)na, '(')))
  {
    name = pos - na + 1;
    addr = 0;
    a_len = pos - na - 1;
    while (*pos && (*pos != ')'))
    {
      n_len++;
      pos++;
    }
    n_len--;
  }
  else if ((pos = strchr((char*)na, '<')))
  {
    name = 0;
    addr = pos - na + 1;
    n_len = addr - 2;
    while (*pos && (*pos != '>'))
    {
      a_len++;
      pos++;
    }
    a_len--;

    if (*na == '"')
    {
      name++;
      n_len--;
    }
    int end = n_len - 1;
    if (end >= 0 && na[name + end] == '"') n_len--;
  }
  else if ((pos = strchr((char*)na, '@')))
  {
    name = addr = 0;
    a_len = strlen(na);
  }
  else
  {
    name = addr = 0;
    n_len = strlen(na);
  }

  if (n_len < 0) n_len = 0;
  if (a_len < 0) a_len = 0;
}


// simple check
bool isEmailAddress (const char *addr, int len)
{
  const char *at = NULL, *dot = NULL;

  if (len > 0)
    for (int i = 0; i < len; i++)
    {
      if (addr[i] == '@')
      {
        if (!at) at = addr + i;
        else
        {
          at = NULL;
          break;
        }
      }
      if (addr[i] == '.') dot = addr + i;
    }

  return (at && dot && (at > addr) && (dot > at + 1) && (dot < addr + len - 1));
}


// get next argument from a command line string
// (i.e. either something without blanks or something put in quotation marks)
const char *nextArg (const char *src, char *dest)
{
  char c, *d = dest;

  while (*src && (*src == ' ')) src++;

  bool qmark = (*src == '"');

  if (qmark) src++;

  while (*src && (*src != (qmark ? '"' : ' ')))
  {
    c = *src++;

    if (c == '\\' && *src == '"')
    {
      c = '"';
      src++;
    }

    *dest++ = c;
  }

  if (*src == '"') src++;

  *dest = '\0';

  return (*d || qmark ? src : NULL);
}


// get the numeric extension of a filename
int getNumExt (const char *fname)
{
  int result = -1;
  const char *ext = strrchr(fname, '.');

  if (ext && (strlen(++ext) == 3) && is_digit(ext[0]) &&
                                     is_digit(ext[1]) &&
                                     is_digit(ext[2]))
    result = atoi(ext);

  return result;
}


// support function for strexcmp, returning normalized s2
char *strex (int &isNot, const char *s2)
{
  int offset;

  if (*s2 == '!' && s2[1] != '\0')
  {
    isNot = 1;
    offset = 1;
  }
  else if (*s2 == '\\' && (s2[1] == '\\' || s2[1] == '!'))
  {
    isNot = 0;
    offset = 1;
  }
  else
  {
    isNot = 0;
    offset = 0;
  }

  return (char *) s2 + offset;
}


// case-insensitive string comparison, supporting '!' as logical NOT
char *strexcmp (const char *s1, const char *s2)
{
  char *match;
  int isNot, found;

  match = strcasestr(s1, strex(isNot, s2));
  found = (match ? 1 : 0);

                          // if isNot == 1 && found == 0, we have to return a
                          // valid pointer, but we have none as match == NULL,
                          // thus, we return s1, but only if it's not empty
  return (isNot ^ found ? (match ? match : (s1 && *s1 ? (char *) s1 : NULL))
                        : NULL);
}


// decode an UTF-8 / RFC2279 encoded string (delim '\0') or line (delim '\n')
// and return amount of bytes data has been shortened
size_t utf8_decode (char *data, char delim)
{
  size_t shorter = 0;
  char *source, *dest, c;
  bool isEnd;
  unsigned char u[3] = {0, 0, 0};

  if (data)
  {
    source = data;
    dest = data;

    while (*source && (*source != delim))
    {
      c = *source++;
      isEnd = (*source == '\0' || *source == delim);

      // select data for later unicode check
      if (!isEnd)
      {
        u[0] = (unsigned char) c;
        u[1] = (unsigned char) *source;
        u[2] = (unsigned char) source[1];
      }

      // check for U+0080 to U+00FF characters,
      // encoded as binary 110 0001x  10 xxxxxx
      if ((c & 0xFE) == 0xC2 && (*source & 0xC0) == 0x80)
      {
        *dest++ = ((c & 0x03) << 6) | (*source & 0x3F);
        if (!isEnd) source++;
      }
      // check for some U+0800 to U+FFFF unicode characters
      // encoded as binary 1110 xxxx  10 xxxxxx  10 xxxxxx
      // (so far, decode only the euro sign)
      else if (!isEnd && (u[0] == 0xE2 && u[1] == 0x82 && u[2] == 0xAC))
      {
        *dest++ = (char) 0xA4;   // Latin-15 euro sign
        source += (source[1] && (source[1] != delim) ? 2 : 1);
      }
      else *dest++ = c;
    }

    *dest = (*source == delim ? delim : '\0');
    shorter = source - dest;
  }

  return shorter;
}


// calculate a crc32 checksum
uint32_t crc32 (const char *s)
{
  uint32_t crc32 = 0;

  if (s)
  {
    while (*s)
    {
      for (int m = 0x80; m != 0; m >>= 1)
        if ((crc32 & 0x80000000 ? 1 : 0) != (*s & m))
          crc32 = (crc32 << 1) ^ CRC32POLYNOM;
        else crc32 <<= 1;

      s++;
    }
  }

  return crc32;
}
