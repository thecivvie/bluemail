/*
 * blueMail offline mail reader
 * address book

 Copyright (c) 1996 Kolossvary Tamas <thomas@vma.bme.hu>
 Copyright (c) 1998 William McBrine <wmcbrine@clark.net>
 Copyright (c) 2010 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */


#include <string.h>
#include "interfac.h"
#include "../common/auxil.h"


AddressBook::Person::Person ()
{
  *name = '\0';
  subject = NULL;
  next = NULL;
}


AddressBook::Person::~Person ()
{
  delete[] subject;
}


void AddressBook::Person::setName (const char *name)
{
  int len = sizeof(this->name) - 1;

  strncpy(this->name, name, len);
  this->name[len] = '\0';
}


const char *AddressBook::Person::getAddress () const
{
  const char *fido = this->address.get();
  const char *other = this->address.get(true);

  return (fido ? fido : (other ? other : ""));
}


AddressBook::AddressBook ()
{
  addrsort = bm.resourceObject->isYes(SortAddressbook);
  addrdisp = true;

  // see EnterAddress() and oneLine() for these
  fake = false;
  highlight = true;

  noOfPersons = noOfListed = 0;
  person = NULL;
  first = NULL;
  listed = NULL;
  filter = NULL;

  MakeChain();
}


AddressBook::~AddressBook ()
{
  delete[] filter;
  DestroyChain();
}


void AddressBook::MakeActive ()
{
  inLetter = ((interface->getprevstate() == letter ||
               interface->getprevstate() == littlearealist)
              && !bm.areaList->isReplyArea());

  net_address null;
  interface->letterwin.setLetterParam(NULL, false, null, NULL, false);

  list_max_x = COLS - 6;
  list_max_y = LINES - 11;

  len1 = list_max_x * 41 / 100;   // 41% of window width
  len2 = list_max_x - len1 - 4;   // 4 spaces in format below

  sprintf(format, " %%-%d.%ds  %%-%d.%ds ", len1, len1, len2, len2);

  int x = (COLS - list_max_x - 2) >> 1;

  list = new Window(list_max_y + 7, list_max_x + 2,
                    (LINES - list_max_y - 7) >> 1, x,
                    C_ADDRBKBORDER, "Addressbook", C_ADDRBKTOPTEXT, 7);

  list->attrib(C_ADDRBKHEADER);
  sprintf(list->lineBuf, format, "Name", (addrdisp ? "Address" : "Subject"));
  list->putstring(1, 1, list->lineBuf);

  if (addrsort && bm.resourceObject->isYes(DrawSortMark))
  {
    const char *smark = "( )";

    list->putstring(1, 6, smark);
    list->putch(1, 7, ACS_DARROW);
  }

  list->attrib(C_ADDRBKBORDER);
  list->horizline(list_max_y + 2, list_max_x);

  int midpos = interface->midpos((x + 1) << 1, 21, 20, 19) + 1;
  int endpos = interface->endpos((x + 1) << 1, 19) + 1;
  // 21 is max length of all strings (left column) below
  // 20 is max length of all strings (middle column) below
  // 19 is max length of all strings (right column) below

  list->attrib(C_ADDRBKKEYS);
  list->putstring(list_max_y + 3, 3, "E");
  list->putstring(list_max_y + 4, 2, "^E");
  list->putstring(list_max_y + 5, 3, "K");
  list->putstring(list_max_y + 3, midpos, "O");
  if (inLetter) list->putstring(list_max_y + 4, midpos, "P");
  list->putstring(list_max_y + 5, midpos, "S");
  list->putstring(list_max_y + 5, endpos, "Q");

  list->attrib(C_ADDRBKDESCR);
  list->putstring(list_max_y + 3, 4, ": Enter new address");
  list->putstring(list_max_y + 4, 4, ": Edit address");
  list->putstring(list_max_y + 5, 4, ": Kill address");
  list->putstring(list_max_y + 3, midpos + 1, ": change sort Order");
  if (inLetter) list->putstring(list_max_y + 4, midpos + 1, ": Pick from letter");
  list->putstring(list_max_y + 5, midpos + 1, (addrdisp ? ": display Subjects"
                                                        : ": default display"));
  list->putstring(list_max_y + 5, endpos + 1, ": Quit addressbook");

  if (filter)
  {
    chtype fmark[2] = {' ' | C_ADDRBKBORDER, FILTER_SIGN | C_ADDRBKBORDER};
    list->putchnstr(0, list_max_x, fmark, 2);
  }

  list->delay_update();

  relist();
  Draw();
}


void AddressBook::Delete ()
{
  delete list;
}


void AddressBook::Quit ()
{
  if (filter)
  {
    delete[] filter;
    filter = NULL;
    MakeChain();     // call only, when filter changes, not in MakeActive()
  }
}


void AddressBook::MakeChain ()
{
  FILE *f;
  Person head, *curr;
  bool end = false;
  char buffer[MYMAXLINE];

  if ((f = fopen(bm.resourceObject->get(addressbookFile), "rt")))
  {
    DestroyChain();
    curr = &head;

    while (!end)
    {
      // find non-empty line
      do
        if (!fgetsnl(buffer, sizeof(buffer), f)) end = true;
      while (!end && (*mkstr(buffer) == '\0'));

      if (!end)
      {
        curr->next = new Person;
        curr = curr->next;
        curr->setName(buffer);
        noOfPersons++;

        // try to get address
        if (!fgetsnl(buffer, sizeof(buffer), f)) end = true;

        if (!end && *mkstr(buffer)) curr->address.set(buffer);
        else continue; // with next entry

        // try to get subject
        if (!fgetsnl(buffer, sizeof(buffer), f)) end = true;

        if (!end && *mkstr(buffer)) curr->subject = strdupplus(buffer);
        else continue; // with next entry
      }

      // skip everything until next entry
      do
        if (!fgetsnl(buffer, sizeof(buffer), f)) end = true;
      while (!end && *mkstr(buffer));
    }

    fclose(f);
  }

  // fill index array
  if (noOfPersons)
  {
    person = new Person *[noOfPersons];
    first = curr = head.next;
    int i = 0;

    while (curr)
    {
      person[i++] = curr;
      curr = curr->next;
    }

    // sort
    if (addrsort) qsort(person, noOfPersons, sizeof(Person *), asortbyname);

    // filter
    listed = new int[noOfPersons];
    for (i = 0; i < noOfPersons; i++)
    {
      // initialize listed, so that oneSearch() can be used to check filter
      listed[i] = i;

      if (!filter || (oneSearch(i, filter) == FND_YES))
        listed[noOfListed++] = i;
    }
  }
}


void AddressBook::DestroyChain ()
{
  while (noOfPersons) delete person[--noOfPersons];
  delete[] person;
  person = NULL;
  first = NULL;

  noOfListed = 0;
  delete[] listed;
  listed = NULL;
}


int AddressBook::noOfItems ()
{
  return noOfListed;
}


void AddressBook::oneLine (int i)
{
  int l = topline + i;

  // may be displayed with faked noOfListed
  Person *curr = (fake && (l == noOfListed - 1) ? NULL : person[listed[l]]);
  const char *data = (curr ? (addrdisp ? curr->getAddress()
                                       : curr->subject)
                           : NULL);

  sprintf(list->lineBuf, format, (curr ? curr->name : ""), (data ? data : ""));

  int oldact = active;
  // avoid a selected line in highlighted color
  if (!highlight) active = -1;

  list->PUTLINE(i, C_ADDRBKLIST);
  active = oldact;
}


searchtype AddressBook::oneSearch (int l, const char *what)
{
  bool found1, found2;
  int isNot;

  found1 = (strexcmp(person[listed[l]]->name, what) != NULL);

  const char *data = (addrdisp ? person[listed[l]]->getAddress()
                               : person[listed[l]]->subject);
  found2 = (strexcmp((data ? data : ""), what) != NULL);

  (void) strex(isNot, what);
  return ((isNot ? found1 && found2 : found1 || found2) ? FND_YES : FND_NO);
}


void AddressBook::EnterAddress (Person *people)
{
  int inp_y, inp;
  Win *input1, *input2 = NULL;
  char buffer[MYMAXLINE];
  Person stranger;

  savePos();

  // edit
  if (people)
  {
    inp_y = active - topline + 2;
  }
  // enter
  else
  {
    inp_y = noOfListed + 2;
    if (inp_y > list_max_y + 1) inp_y = list_max_y + 1;

    fake = true;
    noOfListed++;   // to get an input line after the last entry in list
    Move(END);
    Draw();
  }

  // redraw the selected line in non-highlighted color
  highlight = false;
  oneLine(inp_y - 2);
  list->delay_update();

  input1 = new Win(1, len1, ((LINES - list_max_y - 7) >> 1) + inp_y,
                   ((COLS - list_max_x) >> 1) + 1, C_ADDRBKLIST);

  strncpy(buffer, (people ? people->name : ""), MYMAXLINE - 1);
  buffer[MYMAXLINE - 1] = '\0';

  // get first input
  inp = ((ShadowedWin *) input1)->getstring(0, 0, buffer,
                                            sizeof(stranger.name) - 1,
                                            C_ADDRBKINP, C_ADDRBKLIST,
                                            bm.resourceObject->isYes(Pos1Input),
                                            NULL, NULL, false);

  if (inp != GOT_ESC && *cropesp(buffer) == '\0')
  {
    if (people) extrakeys('K');
    inp = GOT_ESC;
  }

  if (inp != GOT_ESC)
  {
    stranger.setName(buffer);

    input2 = new Win(1, len2, ((LINES - list_max_y - 7) >> 1) + inp_y,
                     ((COLS - list_max_x) >> 1) + len1 + 3, C_ADDRBKLIST);

    strncpy(buffer, (people ? people->getAddress() : ""), MYMAXLINE - 1);
    buffer[MYMAXLINE - 1] = '\0';

    // get second input
    inp = ((ShadowedWin *) input2)->getstring(0, 0, buffer, OTHERADDRLEN,
                                              C_ADDRBKINP, C_ADDRBKLIST,
                                              bm.resourceObject->isYes(Pos1Input),
                                              NULL, (ShadowedWin *) input1, false);

    if ((inp != GOT_ESC) && (*cropesp(buffer) == '\0') &&
        people && people->subject)
    {
      interface->ErrorWindow("Entry with subject must have address");
      inp = GOT_ESC;
    }

    // either address or name must be different from original one
    if (strcmp(buffer, (people ? people->getAddress() : "")) == 0 &&
        strcmp(stranger.name, (people ? people->name : "")) == 0)
      inp = GOT_ESC;
  }

  highlight = true;

  if (fake)
  {
    noOfListed--;   // correct it
    fake = false;
  }

  if (inp != GOT_ESC)
  {
    stranger.address.set(buffer);

    if (!isDupe(&stranger))
    {
      // change entry
      if (people)
      {
        stranger.subject = strdupplus(people->subject);
        writeAddresses(active, true, &stranger);
      }
      // append entry to file
      else
      {
        FILE *f;

        if ((f = fopen(bm.resourceObject->get(addressbookFile), "at")))
        {
          writeAddress(&stranger, f);
          fclose(f);
        }
      }

      MakeChain();
    }

    gotoAddress(stranger.name);
  }
  // no input
  else restorePos();

  delete input2;
  delete input1;
}


bool AddressBook::isDupe (Person *people)
{
  Person *curr;

  const char *addr = strdupplus(people->getAddress());

  for (curr = first; curr; curr = curr->next)
  {
    if (strcmp(people->name, curr->name) == 0 &&
        strcmp(addr, curr->getAddress()) == 0)
    {
      interface->ErrorWindow("Address already exists!");
      break;
    }
  }

  delete[] addr;

  return (curr != NULL);
}


void AddressBook::writeAddress (Person *people, FILE *f)
{
  fprintf(f, "%s\n", people->name);

  const char *data = people->getAddress();
  if (*data)
  {
    fprintf(f, "%s\n", data);

    data = people->subject;
    if (data) fprintf(f, "%s\n", data);
  }

  fputc('\n', f);
}


void AddressBook::writeAddresses (int num, bool keep, Person *people)
{
  FILE *f;

  if ((f = fopen(bm.resourceObject->get(addressbookFile), "wt")))
  {
    // access and write in original (unsorted, unfiltered) order
    for (Person *curr = first; curr; curr = curr->next)
    {
      if (curr == person[listed[num]])
      {
        if (keep) writeAddress(people, f);
      }
      else writeAddress(curr, f);
    }
    fclose (f);
  }
}


void AddressBook::newAddress (const char *name, const char *addr)
{
  Person people;
  FILE *f;

  people.setName(name);
  people.address.set(addr);

  if (!isDupe(&people))
  {
    if ((f = fopen(bm.resourceObject->get(addressbookFile), "at")))
    {
      writeAddress(&people, f);
      fclose(f);
      MakeChain();
    }
  }
}


void AddressBook::gotoAddress (const char *name)
{
  int p, oldtop = topline;

  savePos();
  Move(HOME);

  if ((noOfListed == 0) && bm.resourceObject->isYes(ClearFilter)) Quit();

  for (p = 0; p < noOfListed; p++)
  {
    if (strcmp(name, person[listed[p]]->name) != 0) Move(DOWN);
    else break;
  }

  if (p == noOfListed) restorePos();
  else if (p >= oldtop && p < oldtop + list_max_y) topline = oldtop;
}


void AddressBook::PickAddress ()
{
  const char *from = interface->charconv_in(bm.letterList->isLatin1(),
                                            bm.letterList->getFrom());
  const char *fido = bm.letterList->getNetAddr().get();
  const char *other = bm.letterList->getNetAddr().get(true);

  if (fido)
  {
    newAddress(from, fido);
    if (other)
    {
      interface->update();
      interface->InfoWindow("Additional address");
      newAddress(from, other);
    }
  }
  else newAddress(from, other);

  gotoAddress(from);

  interface->update();
}


void AddressBook::EditSubject (Person *people)
{
  Win *input;
  char buffer[MYMAXLINE + 1];

  if (*people->getAddress() == '\0')
    interface->ErrorWindow("Entries without address cannot have subjects");
  else
  {
    int inp_y = active - topline + 2;

    // redraw the selected line in non-highlighted color
    highlight = false;
    oneLine(inp_y - 2);
    list->delay_update();

    input = new Win(1, len2, ((LINES - list_max_y - 7) >> 1) + inp_y,
                    ((COLS - list_max_x) >> 1) + len1 + 3, C_ADDRBKLIST);

    const char *subj = people->subject;
    strncpy(buffer, (subj ? subj : ""), MYMAXLINE);
    buffer[MYMAXLINE] = '\0';

    // get input
    int inp = ((ShadowedWin *) input)->getstring(0, 0, buffer, MYMAXLINE,
                                                 C_ADDRBKINP, C_ADDRBKLIST,
                                                 bm.resourceObject->isYes(Pos1Input),
                                                 NULL, NULL, false);

    highlight = true;

    if (inp != GOT_ESC && strcmp((subj ? subj : ""), cropesp(buffer)) != 0)
    {
      delete[] people->subject;
      people->subject = (*buffer ? strdupplus(buffer) : NULL);

      writeAddresses(active, true, people);

      if (filter)
      {
        char *name = strdupplus(people->name);   // people will be destroyed
        MakeChain();

        if ((noOfListed == 0) && bm.resourceObject->isYes(ClearFilter))
        {
          Quit();
          gotoAddress(name);
        }

        delete[] name;
      }
    }

    delete input;
  }
}


void AddressBook::askFilter ()
{
  char input[QUERYINPUT + 1];

  strncpy(input, (filter ? filter : ""), QUERYINPUT);
  input[QUERYINPUT] = '\0';

  if (interface->QueryBox("Filter on:", input, QUERYINPUT) != GOT_ESC)
  {
    savePos();

    delete[] filter;
    filter = strdupplus(input);
    MakeChain();

    // empty list isn't allowed
    if (noOfListed == 0)
    {
      if (*filter) interface->ErrorWindow("No match");
      delete[] filter;
      filter = NULL;
      MakeChain();
      restorePos();
    }
  }
}


void AddressBook::extrakeys (int key)
{
  bool olddisp;

  switch (key)
  {
    case 'E':
      if (!(olddisp = addrdisp))
      {
        addrdisp = true;
        interface->InfoWindow("Changing to default display", 1);
      }
      EnterAddress();
      addrdisp = olddisp;
      interface->update();
      break;

    case KEY_CTRL_E:
      if (noOfListed)
      {
        if (addrdisp) EnterAddress(person[listed[active]]);
        else EditSubject(person[listed[active]]);
        interface->update();
      }
      break;

    case 'K':
    case KEY_DC:
      if (noOfListed)
      {
        if (interface->WarningWindow("Do you really want to delete this "
                                     "entry?") == WW_YES)
        {
          writeAddresses(active, false);
          checkDel();
          MakeChain();
        }

        if ((noOfListed == 0) && bm.resourceObject->isYes(ClearFilter)) Quit();

        interface->update();
      }
      break;

    case 'O':
      addrsort = !addrsort;
      MakeChain();
      interface->update();
      break;

    case 'P':
      if (inLetter) PickAddress();
      break;

    case 'S':
      addrdisp = !addrdisp;
      interface->update();
      break;

    case '|':
      askFilter();
      interface->update();
      break;

    case KEY_ENTER:
      if (noOfListed)
      {
        interface->letterwin.setLetterParam(person[listed[active]]->name,
                                            interface->isLatin1(),
                                            person[listed[active]]->address,
                                            person[listed[active]]->subject,
                                            interface->isLatin1());
        Quit();
      }
      break;

    case KEY_LEFT:
      Move(UP);
      Draw();
      break;

    case KEY_RIGHT:
      Move(DOWN);
      Draw();
      break;
  }
}


/*
   address entry qsort compare function
*/

int asortbyname (const void *a, const void *b)
{
  const char *pa = strrword((*(AddressBook::Person **) a)->name);
  const char *pb = strrword((*(AddressBook::Person **) b)->name);

  int result = strcasecoll(pa, pb);

  return (result ? result : strcoll(pa, pb));
}
