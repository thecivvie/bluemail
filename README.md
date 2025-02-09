blueMail Offline Mail Reader
============================

What is blueMail?
-----------------

blueMail is a multi-format offline mail reader for Unix, DOS, Win32, and
other systems.

It supports the Blue Wave, QWK, QWKE, SOUP, OMEN and Hippo packet formats,
the Hudson and BBBS Message Bases, Unix (mbox) mail, Eudora and is designed
to be a reasonable alternative to the Blue Wave mail reader. It has a full
screen, colored user interface built with the curses library.

blueMail is free, "open source" software, provided "as is", without
warranty of any kind, distributed under the GNU General Public License.
Its current maintainer is Ingo Brueckl <ib@wupperonline.de> and will not
accept liability resulting from your use or inability to use blueMail.

You can get the latest version from

  http://home.wtal.de/ib/bluemail


Requirements
------------

 * Unix-compatible (POSIX) system, DOS with DJGPP, Windows with MinGW
   or Cygwin, or OS/2 with EMX
 * curses -- tested with ncurses and PDCurses
 * gcc (g++) -- may work with other C++ compilers, not tested
 * InfoZip, and/or other compressing archivers
 * GNU make *may* be required on some systems
 * PMODE/DJ (by Thomas Pytel & Matthias Grimrath) and UPX (by Markus
   Oberhumer & Laszlo Molnar) are recommended (and used) for the DOS
   binaries


What's new in this version?
---------------------------

You can find a list of new features and improvements in the history file
together with a list of new options for the configuration file since
version 0.01.

See INSTALL for the installation procedure, and the man page (bmail.1, or
bmail.txt for DOS, Windows or OS/2) for information on usage.


Tested Systems
--------------

blueMail will run on multiple platforms, like

  MSDOS 6.22 (DJGPP, PDCurses)
  Linux 2.0, 2.2, 2.4, 2.6 (gcc, egcs, ncurses, PDCurses aka XCurses)
  Windows 9x/NT/XP/2000 (MinGW or Cygwin, PDCurses)

and - untested -

  OS/2 (emx, PDCurses)
  NetBSD (ncurses)
  Solaris (curses)

(Please understand that it is impossible to continuously test blueMail on
all platforms, so there may be bugs on platforms other than my developing
platform DOS, but I'll try to fix all bugs I'll be told.)

If you get it running on a dissimilar system, please let me know (along
with any necessary patches).
