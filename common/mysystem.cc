/*
 * blueMail offline mail reader
 * some operating system specific routines

 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2005 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#ifdef __CYGWIN__
#include <sys/cygwin.h>
#include <sys/wait.h>
#endif

#ifdef __MINGW32__
#include <sys/locking.h>
#endif

#ifndef SECURE_TEMP
#include <fcntl.h>
#endif

#include <ctype.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#ifndef __MINGW32__
#include <sys/utsname.h>
#endif
#include "mysystem.h"
#include "auxil.h"
#include "error.h"
#include "../interfac/interfac.h"


// system-dependent initialization
void myinit ()
{
#ifdef __MSDOS__
  __opendir_flags = __OPENDIR_PRESERVE_CASE;
#endif
}


#ifdef __CYGWIN__
/* Cygwin returns POSIX style pathnames by default which the user shall not
   see. */
char *mygetcwd (char *buffer, size_t maxlen)
{
  if (getcwd(buffer, maxlen))
  {
    cygwin32_conv_to_win32_path(buffer, buffer);
    strcpy(buffer, convert(buffer, '\\', '/'));
  }
  return buffer;
}


/* Cygwin returns POSIX style pathnames by default which the user shall not
   see. */
char *mygetenv (const char *var)
{
  char *env, ebuf[MYMAXPATH];

  if ((env = getenv(var)) && (strlen(env) < MYMAXPATH))
  {
    // cygwin lies if HOME is not set and returns "/"
    if (strcmp(var, "HOME") == 0 && strcmp(env, "/") == 0) return NULL;

    cygwin32_conv_to_win32_path(env, ebuf);
    return convert(ebuf, '\\', '/');
  }
  else return NULL;
}


/* Cygwin's system() call uses and needs a SH.EXE,
   but we want a call using COMMAND.COM */
int cygwinsystem (const char *cmd)
{
  pid_t pid;
  int rc;
  static int result;

  if (!(pid = fork()))
  {
    char *com = mygetenv("COMSPEC");
    if (!com) com = "COMMAND.COM";

    if (cmd) rc = execlp(com, com, "/c", cmd, NULL);
    else rc = execlp(com, com, NULL);

    exit(rc);
  }
  wait(&result);
  return result;
}
#endif


// system call
int mysystem (const char *cmd)
{
  char savdir[MYMAXPATH];

  *savdir = '\0';

  if (interface) interface->syscallwin();

  mygetcwd(savdir, sizeof(savdir));
  int retcode =
#ifdef __CYGWIN__
                cygwinsystem(cmd);
#else
                system(cmd);
#endif
  mychdir(savdir);

  if (interface)
  {
    if (retcode) interface->ErrorWindow("Execution failure!", false);
    interface->endsyscall();
  }

  return retcode;
}


// shell call
void myshell ()
{
#if defined(__MSDOS__) || defined(__EMX__)
  mysystem("");
#elif defined(__MINGW32__)
  char *com = mygetenv("COMSPEC");
  mysystem(com ? com : "COMMAND.COM");
#elif defined(__CYGWIN__)
  mysystem(NULL);
#elif defined(XCURSES)
  mysystem("xterm");
#else
  char *sh = mygetenv("SHELL");
  mysystem(sh ? sh : "/bin/sh");
#endif
}


// return a pattern for a temporary filename
char *mytmpname (const char *path)
{
  const char *p;
  DIR *dir;
  static char tmp[] = "/tmp", tmpname[MYMAXPATH];

  if (path && *path) p = path;
  else
  {
    p = mygetenv("TEMP");

    if (!p) p = mygetenv("TMP");

    if (!p && (dir = opendir(tmp)))
    {
      p = tmp;
      closedir(dir);
    }

    if (!p) fatalError("Could not find temporary directory.");
  }

  mkfname(tmpname, p, TMPFILEPATTERN);
  return tmpname;
}


// create a temporary file
char *mymktmpfile (const char *path)
{
  char *tmpfile;
  int fd;

  tmpfile = mytmpname(path);

#ifdef SECURE_TEMP
  fd = mkstemp(tmpfile);
#else
  fd = (mktemp(tmpfile) ? open(tmpfile, O_RDWR | O_CREAT | O_EXCL,
                                        S_IRUSR | S_IWUSR)
                        : -1);
#endif

  if (fd == -1) fatalError("Out of temporary filenames.");
  else close(fd);

  return tmpfile;
}


// create a temporary directory
char *mymktmpdir (const char *path)
{
  char *tmpdir;

  tmpdir = mytmpname(path);

#if defined(SECURE_TEMP) && defined(__USE_BSD)
  tmpdir = mkdtemp(tmpdir);
#else
  if (!mktemp(tmpdir)) tmpdir = NULL;
  if (tmpdir && (mymkdir(tmpdir, S_IRWXU) == -1)) tmpdir = NULL;
#endif

  if (!tmpdir) fatalError("Out of temporary directories.");

  return tmpdir;
}


// return system name
const char *sysname ()
{
#if defined(__CYGWIN__) || defined(__MINGW32__)
  // cygwin returns a cygwin version string rather than the os name,
  // mingw doesn't have uname
  return "Win32";
#elif defined(__MSDOS__)
  // djgpp returns MS-DOS, DOS looks nicer
  return "DOS";
#else
  static struct utsname u;

  uname(&u);
  return u.sysname;
#endif
}


// fopen with file locking
FILE *fxopen (const char *filename, const char *mode)
{
  char f_mode[4];
  int o_mode, fd;

  bool excl_read = false;
  bool excl_write = false;
  bool access_read = false;
  bool access_rdwr = false;
  bool open_binary = false;
  bool open_text = false;
  bool mode_error = false;

  const char *p = mode;

  while (*p)
  {
    switch (*p++)
    {
      case 'x':
        excl_read = true;
        break;

      case 'X':
        excl_write = true;
        break;

      case 'r':
        access_read = true;
        break;

      case '+':
        access_rdwr = true;
        break;

      case 'b':
        open_binary = true;
        break;

      case 't':
        open_text = true;
        break;

      default:
        mode_error = true;
        break;
    }
  }

  mode_error = mode_error || (excl_read && excl_write) || !access_read ||
               (open_binary && open_text) || (!open_binary && !open_text);

  if (mode_error) fatalError("Wrong file mode.");

  o_mode = (access_rdwr ? O_RDWR : O_RDONLY)
#if defined(__MSDOS__) || defined(__CYGWIN__) || defined(__EMX__) || defined(__MINGW32__)
         | (open_binary ? O_BINARY : O_TEXT)
#endif
                                            ;
// DOS locking
#if defined(__MSDOS__) || defined(__EMX__)
  if (excl_read) o_mode |= SH_DENYWR;
  if (excl_write) o_mode |= SH_DENYRW;
#endif

  strcpy(f_mode, "r");
  if (access_rdwr) strcat(f_mode, "+");
  strcat(f_mode, (open_binary ? "b" : "t"));

  if ((fd = open(filename, o_mode)) == -1) return NULL;

// other OS locking
#if !defined(__MSDOS__) && !defined(__EMX__)
  if (excl_read || excl_write)
  {
#if defined(__CYGWIN__)
    struct flock lockinfo;
    lockinfo.l_type = (excl_read ? F_RDLCK : F_WRLCK);
    lockinfo.l_whence = SEEK_SET;
    lockinfo.l_start = 0;
    lockinfo.l_len = 0;
    if (fcntl(fd, F_SETLK, &lockinfo) == -1)
#elif defined(__MINGW32__)
    if (_locking(fd, excl_read ? LK_NBRLCK : LK_NBLCK, fsize(filename)) == -1)
#else
    if (flock(fd, (excl_read ? LOCK_SH : LOCK_EX) | LOCK_NB) == -1)
#endif
    {
      close(fd);
      return NULL;
    }
  }
#endif

  return fdopen(fd, f_mode);
}


// ISO or non-ISO system?
bool isIso (const char *cc)
{
  return (cc ? toupper(*cc) == 'L' :
#if defined(__MSDOS__) || defined(__EMX__) || defined(__CYGWIN__) || defined(__MINGW32__)
                                     false);
#else
                                     true);
#endif
}


const char *myenvf ()
{
  return
#if defined(__MSDOS__) || defined(__EMX__) || defined(__CYGWIN__) || defined(__MINGW32__)
         "%%%s%%";
#else
         "$%s";
#endif
}


// If newname's extension has more than three characters, it will be cut
// to three characters during renaming, resulting in a newname the user
// probably didn't have in mind. Thus, we return an error in that case.
#ifdef __MSDOS__
int myrename (const char *oldname, const char *newname)
{
  const char *ext = strrchr(newname, '.');

  if (ext && (strlen(ext) > 4)) return -1;
  else return rename(oldname, newname);
}
#endif
