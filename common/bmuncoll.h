/*
 * blueMail offline mail reader
 * program to extract files from a collection

 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef BMUNCOLL_H
#define BMUNCOLL_H


void usage();
void error(const char *);
bool uncollect(const char *, long, const char *, bool);


#endif
