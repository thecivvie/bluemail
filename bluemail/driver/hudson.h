/*
 * Data Structure Definitions for the Hudson Message Base Files

 Copyright (c) 2003 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef HUDSON_H
#define HUDSON_H


/* If you are programming for a system that employs a CPU which stores multi-
   byte integers in a manner other than in Intel format (LSB-MSB, or "little
   endian"), simply #define BIG_ENDIAN before #including this header. As
   shown below, this will define the data types as arrays of bytes; the
   drawback is that *YOU* will have to write functions to convert the data,
   since the Hudson Message Base specification requires the data to be in
   Intel-style little-endian format. */

#ifndef HAS_UINT
  #ifdef BIG_ENDIAN
    typedef unsigned char UINT[2];     /* little-endian 16 bit unsigned */
  #else
    typedef unsigned short UINT;       /* 16 bit unsigned */
  #endif
#endif


/* ----------------------------------------------------------------- */
/*  Structure Lengths                                                */
/*  (Use instead of sizeof(), sizeof() is only save for BIG_ENDIAN!) */
/* ----------------------------------------------------------------- */

#define SIZEOF_HMB_INFO  406
#define SIZEOF_HMB_IDX     3
#define SIZEOF_HMB_TOIDX  36
#define SIZEOF_HMB_HDR   187
#define SIZEOF_HMB_TXT   256


/* ----------- */
/*  Bit Masks  */
/* ----------- */

/* message attribute bits */

#define HMB_MSG_DELETED  0x01
#define HMB_MSG_UNSENT   0x02
#define HMB_MSG_NETMAIL  0x04
#define HMB_MSG_PRIVATE  0x08
#define HMB_MSG_RECEIVED 0x10
#define HMB_MSG_OUTECHO  0x20
#define HMB_MSG_LOCAL    0x40
#define HMB_MSG_RESERVED 0x80

/* netmail attribute bits */

#define HMB_NET_KILL     0x01
#define HMB_NET_SENT     0x02
#define HMB_NET_FATTACH  0x04
#define HMB_NET_CRASH    0x08
#define HMB_NET_RECPTREQ 0x10
#define HMB_NET_AUDITREQ 0x20
#define HMB_NET_RETRECPT 0x40
#define HMB_NET_RESERVED 0x80


/* ----------------- */
/*  File Structures  */
/* ----------------- */

typedef struct   /* MSGINFO.BBS */
{
  UINT msg_low;                /* lowest message number in message base */
  UINT msg_high;               /* highest message number in message base */
  UINT msgs_total;             /* total number of messages in message base */
  UINT msgs_on_board[200];     /* number of messages on each board */
} HMB_INFO;


typedef struct   /* MSGIDX.BBS */
{
  UINT msgnum;                 /* message number */
  unsigned char board;         /* board number where message is stored */
} HMB_IDX;


typedef struct   /* MSGTOIDX.BBS */
{
  unsigned char length;        /* number of characters of addressee */
  char addressee[35];          /* addressee of record */
} HMB_TOIDX;


typedef struct   /* MSGHDR.BBS */
{
  UINT msgnum;                 /* message number */
  UINT prev_reply;             /* message number of previous reply, 0 if none */
  UINT next_reply;             /* message number of next reply, 0 if none */
  UINT times_read;             /* number of times message was read, UNUSED */
  UINT start_record;           /* record number of message in MSGTXT.BBS */
  UINT records;                /* number of records in MSGTXT.BBS */
  UINT dest_net;               /* destination net */
  UINT dest_node;              /* destination node */
  UINT orig_net;               /* origin net */
  UINT orig_node;              /* origin node */
  unsigned char dest_zone;     /* destination zone */
  unsigned char orig_zone;     /* origin zone */
  UINT cost;                   /* cost (netmail) */
  unsigned char msg_attr;      /* message attribute bits as follows: */
                               /* 0 : Deleted                        */
                               /* 1 : Unsent                         */
                               /* 2 : Netmail                        */
                               /* 3 : Private                        */
                               /* 4 : Received                       */
                               /* 5 : Unmoved outgoing echo          */
                               /* 6 : Local                          */
                               /* 7 : RESERVED                       */
  unsigned char net_attr;      /* netmail attribute bits as follows: */
                               /* 0 : Kill/Sent                      */
                               /* 1 : Sent                           */
                               /* 2 : File attach                    */
                               /* 3 : Crash                          */
                               /* 4 : Receipt request                */
                               /* 5 : Audit request                  */
                               /* 6 : Is a return receipt            */
                               /* 7 : RESERVED                       */
  unsigned char board;         /* message board number */
  unsigned char time_length;   /* number of characters used in time */
  char time[5];                /* time HH:MM message was posted */
  unsigned char date_length;   /* number of characters used in date */
  char date[8];                /* date MM-DD-YY message was posted */
  unsigned char to_length;     /* number of characters used in recipient */
  char to[35];                 /* recipient to whom message is sent */
  unsigned char from_length;   /* number of characters used in sender */
  char from[35];               /* sender who posted message */
  unsigned char subj_length;   /* number of characters used in subject */
  char subject[72];            /* subject line of message */
} HMB_HDR;


typedef struct   /* MSGTXT.BBS */
{
  unsigned char length;        /* number of characters used in chunk */
  char chunk[255];             /* a chunk of the message body */
} HMB_TXT;


#endif
