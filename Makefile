#---------------------------------
# blueMail Makefile * Definitions
#---------------------------------

PREFIX     = /usr/local
BINDIR     = $(PREFIX)/bin
MANDIR     = $(PREFIX)/share/man
MAN1DIR    = $(MANDIR)/man1
MAN5DIR    = $(MANDIR)/man5
DOCDIR     = $(PREFIX)/share/doc
BMDOC      = bmail
BMEXAMPLES = examples

MKDIR      = mkdir
INSTALL    = install -c
INSTALLBIN = $(INSTALL) -s -m 755
INSTALLMAN = $(INSTALL) -m 644

# With debug:
#OPTS = -g -fno-rtti -pedantic -Wall -W -Wmissing-declarations

# Optimized, no debug:
OPTS = -O2 -fno-rtti -pedantic -Wall -W -Wmissing-declarations -fpermissive

#---------------------------------------------------------------------
# Defaults are for Linux, with ncurses:
# ("." is used as a placeholder)

CURS_HEADER = \<curses.h\>
CURS_INCDIR = .
CURS_LIBDIR = .
LIBS        = -lncurses
RANLIB      = ranlib
RM          = rm -f
SEP         = ;
POST        =
ARQUIET     = 2>&1 | sed "/^ar: creating $*/d"
MOPTS       = -fno-exceptions -D_GNU_SOURCE -DUSE_SHADOWS -DSECURE_TEMP \
		-DFIXSTANDOUT -DWITH_CLOCK

#CURS_HEADER    (name of the curses header file)
#CURS_INCDIR    (where curses header file can be found)
#CURS_LIBDIR    (where ncurses library can be found)
#LIBS           (library that need to be linked in)
#RANLIB         (ranlib command)
#RM             (delete command)
#SEP            (multi-statement lines separator)
#POST           (any post-processing that should take place)
#MOPTS          (more compiler options)
#
#                USE_SHADOWS: "shadowed" windows
#                SECURE_TEMP: safer way of creating temporary files
#                FIXSTANDOUT: nicer selection bar for list windows
#                WITH_CLOCK:  enable clock in the letter window

#---------------------------------------------------------------------
# Linux, with PDCurses (aka XCurses) 2.7:

#CURS_HEADER = \<xcurses.h\>
#CURS_INCDIR = /usr/X11R6/include
#CURS_LIBDIR = /usr/X11R6/lib
#LIBS        = -lXCurses -lXaw -lXmu -lXt -lSM -lICE -lXext -lX11 -lXpm
#MOPTS       = -fno-exceptions -D_GNU_SOURCE -DUSE_SHADOWS -DSECURE_TEMP \
#		-DFIXSTANDOUT -DWITH_CLOCK -DXCURSES -DHAVE_PROTO

#---------------------------------------------------------------------
# DJGPP (MSDOS), with PDCurses 2.7:

#LIBS        = -lpdcurses
#RM          = del
# use UPX executable packer instead of: strip bmail $(SEP) \
# use PMODE/DJ stub instead of automatically stubbed bmail.exe
#POST        = @if exist bmail.exe $(RM) bmail.exe $(SEP) \
#		upx -qq --best --overlay=strip --coff bmail $(SEP) \
#		copy /b c:\dos\compiler\gc\bin\pmodstub.exe + bmail \
#		BMAIL.EXE >nul $(SEP) $(RM) bmail $(SEP) \
#		if exist bmuncoll.exe $(RM) bmuncoll.exe $(SEP) \
#		upx -qq --best --overlay=strip --coff bmuncoll $(SEP) \
#		copy /b c:\dos\compiler\gc\bin\pmodstub.exe + bmuncoll \
#		BMUNCOLL.EXE >nul $(SEP) $(RM) bmuncoll
#ARQUIET     =
#MOPTS       = -DUSE_SHADOWS -DSECURE_TEMP -DFIXSTANDOUT -DWITH_CLOCK

#---------------------------------------------------------------------
# Cygwin (Windows), with PDCurses 2.7:

#LIBS        = /usr/lib/pdcurses.a
#POST        = strip bmail.exe bmuncoll.exe
#ARQUIET     =
#MOPTS       = -fno-exceptions -DUSE_SHADOWS -DSECURE_TEMP -DFIXSTANDOUT \
#		-DWITH_CLOCK

#---------------------------------------------------------------------
# MinGW (Windows), with PDCurses 2.7:

#LIBS        = /lib/mingw/pdcurses.a
#POST        = strip bmail.exe bmuncoll.exe
#ARQUIET     =
#MOPTS       = -fno-exceptions -DUSE_SHADOWS -DFIXSTANDOUT -DWITH_CLOCK

#---------------------------------------------------------------------
# NetBSD, with ncurses:

#CURS_INCDIR = /usr/include/ncurses
#CURS_LIBDIR = /usr/local/lib
#MOPTS       = -DUSE_SHADOWS

#---------------------------------------------------------------------
# Solaris, with standard curses:

#LIBS        = -lcurses
#MOPTS       =

#---------------------------------------------------------------------
# EMX (OS/2), with GNU make, and PDCurses 2.3:
# (Note: If you get "g++: Command not found",
# then type "set cxx=gcc" before running make.)

#CURS_INCDIR = /emx/PDCurses-2.3
#LIBS        = /emx/PDCurses-2.3/os2/pdcurses.a
#RANLIB      = ar -s
#RM          = del /n
#SEP         = &&
# (remove "emxbind -s" for a debug executable):
#POST        = emxbind bmail $(SEP) $(RM) bmail $(SEP) emxbind -s bmail
#MOPTS       = -DUSE_SHADOWS

#-----------------------------
# blueMail Makefile * Targets
#-----------------------------

all: bmail
	$(POST)

bluemail/bmail.a: bluemail/%: FORCE
	@$(MAKE) $* -C bluemail

common/common.a: common/%: FORCE
	@$(MAKE) $* -C common

interfac/interfac.a: interfac/%: FORCE
	@$(MAKE) $* -C interfac

common/bmuncoll.o: common/%: FORCE
	@$(MAKE) $* -C common

bmail: bluemail/bmail.a common/common.a interfac/interfac.a bmuncoll
	@echo "  LD  " $@
	@$(CXX) -o bmail interfac/interfac.a bluemail/bmail.a common/common.a \
	  -L$(CURS_LIBDIR) $(LIBS)

bmuncoll: common/bmuncoll.o
	@echo "  LD  " $@
	@$(CXX) -o bmuncoll common/bmuncoll.o

dep:
	@$(MAKE) $@ -C bluemail
	@$(MAKE) $@ -C common
	@$(MAKE) $@ -C interfac

txt:
	@$(MAKE) $@ -C misc

clean:
	$(RM) bmail
	$(RM) bmail.exe
	$(RM) bmuncoll
	$(RM) bmuncoll.exe
	@$(MAKE) $@ -C bluemail
	@$(MAKE) $@ -C common
	@$(MAKE) $@ -C interfac
	@$(MAKE) $@ -C misc

check:
	@XFILES=`find -name '*.*x'` $(SEP) \
	if [ -n "$$XFILES" ]; then \
	  echo "*** Es gibt *.*x Sicherungsdateien!" $(SEP) \
	fi

	@MAJOR=`grep BM_MAJOR common/mysystem.h | cut -d" " -f3` $(SEP) \
	MINOR=`grep BM_MINOR common/mysystem.h | cut -d" " -f3` $(SEP) \
	RPMVER=`head -1 misc/bmail.spec | cut -d" " -f3` $(SEP) \
	MAN1DATE=`head -1 misc/bmail.1 | sed 's:\.TH blueMail 1 "\(.*, [0-9]\+\)".*:\1:'` $(SEP) \
	MAN5DATE=`head -1 misc/bmailrc.5 | sed 's:\.TH bmailrc 5 "\(.*, [0-9]\+\)".*:\1:'` $(SEP) \
	MANUNCOLL=`head -1 misc/bmuncoll.1 | sed 's:\.TH BMUNCOLL 1 "\(.*, [0-9]\+\)".*:\1:'` $(SEP) \
	echo "Version: $$MAJOR.$$MINOR" $(SEP) \
	echo "Manpage: (1) $$MAN1DATE" $(SEP) \
	echo "         (5) $$MAN5DATE" $(SEP) \
	echo "  (bmuncoll) $$MANUNCOLL" $(SEP) \
	PWDVER=`pwd | cut -d"-" -f2` $(SEP) \
        if [ "$$MAJOR.$$MINOR" != "$$PWDVER" ]; then \
	  echo "*** Verzeichnisname bmail-$$PWDVER passt nicht zur Version!" $(SEP) \
	fi $(SEP) \
	if [ "$$MAJOR.$$MINOR" != "$$RPMVER" ]; then \
	  echo "*** Versionskonflikt mit RPM.spec!" $(SEP) \
	fi

pack: clean dep txt check
	@BMAIL=`pwd` $(SEP) \
	echo "TAR:" $${BMAIL##*/}.tar.bz2 $(SEP) \
	cd .. $(SEP) tar cjf ~/$${BMAIL##*/}.tar.bz2 $${BMAIL##*/} $(SEP) cd $${BMAIL##*/}

install: install-bin install-man install-misc

install-bin:
	$(INSTALLBIN) bmail $(BINDIR)
	$(INSTALLBIN) bmuncoll $(BINDIR)

install-man:
	$(INSTALLMAN) misc/bmail.1 $(MAN1DIR)
	$(INSTALLMAN) misc/bmailrc.5 $(MAN5DIR)
	$(INSTALLMAN) misc/bmuncoll.1 $(MAN1DIR)

install-misc:
	$(MKDIR) $(DOCDIR)/$(BMDOC)
	$(MKDIR) $(DOCDIR)/$(BMDOC)/$(BMEXAMPLES)
	$(INSTALLMAN) misc/bmail.def $(DOCDIR)/$(BMDOC)/$(BMEXAMPLES)
	$(INSTALLMAN) misc/bmail16.png $(DOCDIR)/$(BMDOC)/$(BMEXAMPLES)
	$(INSTALLMAN) misc/bmail32.png $(DOCDIR)/$(BMDOC)/$(BMEXAMPLES)
	$(INSTALLMAN) misc/demo.pkt $(DOCDIR)/$(BMDOC)/$(BMEXAMPLES)
	$(INSTALLMAN) misc/tz-linux.sh $(DOCDIR)/$(BMDOC)/$(BMEXAMPLES)
	$(INSTALLMAN) misc/uncoll.sh $(DOCDIR)/$(BMDOC)/$(BMEXAMPLES)

dos: all
	bmail.bat /install

.EXPORT_ALL_VARIABLES:
.PHONY: all dep txt clean check install install-bin install-man install-misc dos FORCE
FORCE:
