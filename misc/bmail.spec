%define ver 1.4

Summary: A multi-format offline mail reader.
Name: bmail
Version: %ver
Release: 1
Requires: ncurses
Source: bmail-%ver.tar.gz
Copyright: GNU General Public License
URL: http://home.wtal.de/ib/bluemail
Group: Applications/Communications
Packager: Ingo Brueckl <ib@wupperonline.de>
BuildRoot: /var/tmp/%{name}

%description
blueMail is a multi-format offline mail reader for Unix and other systems.
It supports the Blue Wave, QWK, QWKE, SOUP, OMEN and Hippo packet formats,
the Hudson and BBBS Message Bases, Unix (mbox) mail, Eudora and is designed
to be a reasonable alternative to the Blue Wave mail reader. blueMail has a
full screen, colored user interface built with the curses library.

%prep
%setup

%build
make

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/usr/bin
mkdir -p $RPM_BUILD_ROOT/usr/share/man/man1
mkdir -p $RPM_BUILD_ROOT/usr/share/man/man5
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/bmail/examples
make PREFIX= BINDIR=$RPM_BUILD_ROOT/usr/bin MANDIR=$RPM_BUILD_ROOT/usr/share/man DOCDIR=$RPM_BUILD_ROOT/usr/share/doc install

%files
%attr(755,root,root) /usr/bin/bmail
%attr(644,root,root) /usr/share/man/man1/bmail.1.gz
%attr(644,root,root) /usr/share/man/man5/bmailrc.5.gz
%attr(755,root,root) /usr/bin/bmuncoll
%attr(644,root,root) /usr/share/man/man1/bmuncoll.1.gz
%attr(644,root,root) /usr/share/doc/bmail/examples/bmail.def
%attr(644,root,root) /usr/share/doc/bmail/examples/bmail16.png
%attr(644,root,root) /usr/share/doc/bmail/examples/bmail32.png
%attr(644,root,root) /usr/share/doc/bmail/examples/demo.pkt
%attr(644,root,root) /usr/share/doc/bmail/examples/tz-linux.sh
%attr(644,root,root) /usr/share/doc/bmail/examples/uncoll.sh
