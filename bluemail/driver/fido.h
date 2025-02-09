/*
 * Data Structure Definitions for FidoNet(r) Messages and Packets

   according to FidoNet(r) Technical Standard FTS-0001 and
   FidoNet(r) Standards Proposal FSC-0048

 Copyright (c) 2003 Ingo Brueckl <ib@wupperonline.de>

 Distributed under the GNU General Public License.
 For details, see the file COPYING in the parent directory. */

#ifndef FIDO_H
#define FIDO_H


/* If you are programming for a system that employs a CPU which stores multi-
   byte integers in a manner other than in Intel format (LSB-MSB, or "little
   endian"), simply #define BIG_ENDIAN before #including this header. As
   shown below, this will define the data types as arrays of bytes; the
   drawback is that *YOU* will have to write functions to convert the data,
   since the Fido message and packet specification requires the data to be in
   Intel-style little-endian format. */

#ifndef HAS_UINT
  #ifdef BIG_ENDIAN
    typedef unsigned char UINT[2];     /* little-endian 16 bit unsigned */
  #else
    typedef unsigned short UINT;       /* 16 bit unsigned */
  #endif
#endif


/* ----------- */
/*  Bit Masks  */
/* ----------- */

/* message attribute bits */

#define FIDO_MSG_PRIVATE       0x0001
#define FIDO_MSG_CRASH         0x0002
#define FIDO_MSG_RECEIVED      0x0004
#define FIDO_MSG_SENT          0x0008
#define FIDO_MSG_FILEATTACHED  0x0010
#define FIDO_MSG_INTRANSIT     0x0020
#define FIDO_MSG_ORPHAN        0x0040
#define FIDO_MSG_KILL          0x0080
#define FIDO_MSG_LOCAL         0x0100
#define FIDO_MSG_HOLD          0x0200
#define FIDO_MSG_UNUSED        0x0400
#define FIDO_MSG_FILEREQUEST   0x0800
#define FIDO_MSG_RETRECPTREQ   0x1000
#define FIDO_MSG_ISRETRECPT    0x2000
#define FIDO_MSG_AUDITREQUEST  0x4000
#define FIDO_MSG_FILEUPDATEREQ 0x8000


/* ------------------- */
/*  Header Structures  */
/* ------------------- */

typedef struct
{
  char from[36];            /* sender who posted message */
  char to[36];              /* recipient to whom message is sent */
  char subject[72];         /* subject line of message */
  char date[20];            /* date and time (DD MMM YY  HH:MM:SS)
                               message body was last edited */
  UINT times_read;          /* number of times message was read */
  UINT dest_node;           /* destination node */
  UINT orig_node;           /* origin node */
  UINT cost;                /* cost of message */
  UINT orig_net;            /* origin net */
  UINT dest_net;            /* destination net */
  UINT dest_zone;           /* destination zone */
  UINT orig_zone;           /* origin zone */
  UINT dest_point;          /* destination point */
  UINT orig_point;          /* origin point */
  UINT reply_to;            /* message number to which this message replies */
  UINT attribute;           /* message attribute bits as follows: */
                            /*  0 : Private                       */
                            /*  1 : Crash                         */
                            /*  2 : Recd                          */
                            /*  3 : Sent                          */
                            /*  4 : FileAttached                  */
                            /*  5 : InTransit                     */
                            /*  6 : Orphan                        */
                            /*  7 : KillSent                      */
                            /*  8 : Local                         */
                            /*  9 : HoldForPickup                 */
                            /* 10 : unused                        */
                            /* 11 : FileRequest                   */
                            /* 12 : ReturnReceiptRequest          */
                            /* 13 : IsReturnReceipt               */
                            /* 14 : AuditRequest                  */
                            /* 15 : FileUpdateReq                 */
  UINT next_reply;          /* message number which replies to this message */
} FIDO_MSG_HEADER;


typedef struct
{
  UINT type;                 /* message type, always 0x02 0x00 */
  UINT orig_node;            /* origin node */
  UINT dest_node;            /* destination node */
  UINT orig_net;             /* origin net */
  UINT dest_net;             /* destination net */
  UINT attribute;            /* message attribute bits as follows: */
                             /*  0 : Private                       */
                             /*  1 : Crash                         */
                             /*  2 : Recd                          */
                             /*  3 : Sent                          */
                             /*  4 : FileAttached                  */
                             /*  5 : InTransit                     */
                             /*  6 : Orphan                        */
                             /*  7 : KillSent                      */
                             /*  8 : Local                         */
                             /*  9 : HoldForPickup                 */
                             /* 10 : unused                        */
                             /* 11 : FileRequest                   */
                             /* 12 : ReturnReceiptRequest          */
                             /* 13 : IsReturnReceipt               */
                             /* 14 : AuditRequest                  */
                             /* 15 : FileUpdateReq                 */
  UINT cost;                 /* cost of message */
  char date[20];             /* date and time (DD MMM YY  HH:MM:SS)
                                message body was last edited */
/* now, variable length strings follow: */
/* char to[];                   (max. 36) recipient to whom message is sent */
/* char from[];                 (max. 36) sender who posted message */
/* char subject[];              (max. 72) subject line of message */
} FIDO_PACKED_MSG_HEADER;

#ifdef FIDO_PACKED_MSG_HEADER_VARIABLES
  char FIDO_PACKED_MSG_HEADER_to[36];
  char FIDO_PACKED_MSG_HEADER_from[36];
  char FIDO_PACKED_MSG_HEADER_subject[72];
#endif


typedef struct
{
  UINT orig_node;                 /* origin node of packet */
  UINT dest_node;                 /* destination node of packet */
  UINT year;                      /* of packet creation */
  UINT month;                     /* of packet creation */
  UINT day;                       /* of packet creation */
  UINT hour;                      /* of packet creation */
  UINT minute;                    /* of packet creation */
  UINT second;                    /* of packet creation */
  UINT baud;                      /* maximum baud rate of orig and dest */
  UINT type;                      /* packet type, always 0x02 0x00 */
  UINT orig_net;                  /* origin net of packet */
  UINT dest_net;                  /* destination net of packet */
  unsigned char prod_code_low;    /* of program which created the packet */
  unsigned char revision_major;   /* binary serial number */
  unsigned char password[8];      /* session password */
  UINT orig_zone1;                /* origin zone of packet */
  UINT dest_zone1;                /* destination zone of packet */
  UINT aux_net;                   /* contains orig_net if origin is a point */
  UINT capability_copy;           /* must be a swapped copy of capability */
  unsigned char prod_code_high;   /* of program which created the packet */
  unsigned char revision_minor;   /* binary serial number */
  UINT capability;                /* bundle type of packet */
  UINT orig_zone2;                /* origin zone of packet */
  UINT dest_zone2;                /* destination zone of packet */
  UINT orig_point;                /* origin point of packet */
  UINT dest_point;                /* destination point of packet */
  unsigned char prod_data[4];     /* product specific filler */
} FIDO_PACKET_HEADER;


#endif
