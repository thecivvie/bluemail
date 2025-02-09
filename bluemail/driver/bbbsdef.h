/* $Id: bbbsdef.h,v 1.94 1999/12/19 13:19:57 Kim.Heino Exp $ */

/*****************************************************************************
 *                                                                           *
 *    BBBS is Copyright 1990-1999, Kim B. Heino and Tapani T. Salmi.         *
 *                                                                           *
 *    Definitions and variables for BBBS.                                    *
 *    You may use this file as long as this is totally unmodified.           *
 *    We take no responsibility over any program using this file.            *
 *                                                                           *
 *****************************************************************************/

#if defined(__OS2__) || defined(__EMX__)
#pragma pack(1)
#endif

typedef unsigned char         boolean;    /*  8 bit, true or false */
typedef unsigned char         byte;       /*  8 bit, unsigned */
typedef signed short          integer;    /* 16 bit, signed */
typedef unsigned short        word;       /* 16 bit, unsigned */
#if defined(__ALPHA__)
typedef signed int            longint;    /* 32 bit, signed */
typedef unsigned int          unlong;     /* 32 bit, unsigned */
#else
typedef signed long           longint;    /* 32 bit, signed */
typedef unsigned long         unlong;     /* 32 bit, unsigned */
#endif

#define true                  1
#define false                 0

#if defined(__MSDOS__)
#define lines_in_editor       1000        /* max lines in editor */
#define max_msg               2048        /* max unread messages per area */
#define desclen               768
#define chat_last_saved       10          /* /last */
#else
#define lines_in_editor       4096        /* max lines in editor */
#define max_msg               8192        /* max unread messages per area */
#define desclen               4096
#define chat_last_saved       20          /* /last */
#endif
#define disk_lastread         128         /* word-pairs in tmp lastread file */
#if defined(__MSDOS__)
#define buffer_len            4096        /* com buffer length, must be 2^something */
#define buffer_open           3000
#define buffer_close          3900
#else
#define buffer_len            8192        /* com buffer length, must be 2^something */
#define buffer_open           6144
#define buffer_close          8000
#endif
#define txtsize               713         /* bbbstxt lines */
#define commands_saved        20          /* inputs saved */
#define fileaccess_NONE       0
#define fileaccess_B          1
#define fileaccess_W          2
#define fileaccess_R          4
#define fileaccess_U          8
#define linkoverflow          128
#define num_of_akas           44
#define num_of_events         90
#define num_of_limits         128
#define usernumcolors         80

#define stat_must       0x00000001L       /* status bits */
#define stat_member     0x00000002L
#define stat_invite     0x00000004L
#define stat_fidoarea   0x00000008L
#define stat_postarea   0x00000010L
#define stat_allowpriv  0x00000020L
#define stat_nomarks    0x00000040L
#define stat_noreply    0x00000080L
#define stat_nostrip    0x00000100L
#define stat_allfix     0x00000200L
#define stat_namefix    0x00000400L
#define stat_alias      0x00000800L
#define stat_allowtag   0x00001000L
#define stat_agnet      0x00002000L
#define stat_moderated  0x00004000L       /* used internally, DO NOT SET */
#define stat_nntp       0x00008000L       /* used internally, DO NOT SET */
#define stat_acc_read   0x10000000L       /* used internally, DO NOT SET */
#define stat_acc_write  0x20000000L       /* used internally, DO NOT SET */
#define stat_acc_sigop  0x40000000L       /* used internally, DO NOT SET */

#define mstat_killed    1       /* mstatust bits */
#define mstat_listsent  2
#define mstat_private   4
#define mstat_sent      8
#define mstat_readed    16
#define mstat_extraline 32
#define mstat_nntpsent  64
#define mstat_mempty3   128
#define mstat_mempty4   256
#define mstat_mempty5   512
#define mstat_mempty6   1024
#define mstat_mempty7   2048
#define mstat_mempty8   4096
#define mstat_mempty9   8192
#define mstat_mempty10  16384
#define mstat_mempty11  32768

#define char_IBM        0       /* charsets */
#define char_SF7        1
#define char_ISO        2
#define char_IBN        3
#define char_US7        4
#define char_GE7        5
#define char_NO7        6
#define char_FR7        7
#define char_IT7        8
#define char_SP7        9
#define char_MAC        10

#define CTABLE_IN       0
#define CTABLE_OUT      256

#define bzs_alias_len   1024
#define bzs_set_len     2048
#define bzs_end         0
#define bzs_foo2        1
#define bzs_alias       2
#define bzs_set         3
#define bzs_script      4

#define bun_year(l)     (int)((((unlong)(l)>>25) & 127)+80)
#define bun_month(l)    (int)(((l)>>21) & 15)
#define bun_day(l)      (int)(((l)>>16) & 31)
#define bun_hour(l)     (int)(((l)>>11) & 31)
#define bun_minute(l)   (int)(((l)>>5) & 63)
#define bun_second(l)   (int)(((l) & 31)<<1)

#define outputstopped_no_break  16
#define outputstopped_show_log  32
#define outputstopped_no_local  64
#define outputstopped_no_remote 128
#define outputstopped_reset     0xf8
#define outputstopped_set       1
#define outputstopped_check     7

struct userrec {                /* maindir/bbbsuser.dat */
  char            name[30];     /*   0 */
  byte            password[16]; /*  30 */ /* MD5 of password */
  char            address[30];  /*  46 */
  char            city[30];     /*  76 */
  char            phone[18];    /* 106 */
  char            birth[18];    /* 124 */
  byte            ok2login;     /* 142 */ /* 0=yes,1=getlost,2=killed,3=kill+boot, 4=unverified */
  byte            termcap;      /* 143 */ /* Bit 0-3: 0=TTY,1=ANSI,2=dummy,3=VT320,4=RIP, Bit 4-7: 0=Line,1=FSE,2=MG,3=local/script */
  byte            pagelength;   /* 144 */
  byte            charset;      /* 145 */
  byte            language;     /* 146 */ /* 0..9 (0=English,1=Suomi,2=Svenska,3=Norsk) */
  byte            readmode;     /* 147 */ /* Bit 0-3: 0=Marked,1=Reference,2=Forward, Bit 4-7: 0=Text,1=Hippo1,2=Hippo2,3=OMEN,4=QWK,5=BW,6=QWKREP */
  byte            packtype;     /* 148 */ /* 0=text,1=arc,2=zip,3=lzh,4=arj,5=zoo,6=hpk,7=rar */
  byte            protocol;     /* 149 */ /* 0=Z,1=Y,2=X,3=SHYDRA,4=XCRC,5=YB,6=SZmodem,7=HYDRA,8=ZedZap,9=Kermit,10=UUcode */
  byte            nodemsgfilt;  /* 150 */ /* 0=feelings, 1=login/logout, 2=entered message, 3=public chat, 4=priv msgs */
  integer         timelimit;    /* 151 */
  integer         timeleft;     /* 153 */
  word            fchecked;     /* 155 */
  word            timebank;     /* 157 */
  word            limits;       /* 159 */
  unlong          timeson;      /* 161 */
  unlong          ptimeson;     /* 165 */
  unlong          msgleft;      /* 169 */
  unlong          pmsgleft;     /* 173 */
  unlong          msgread;      /* 177 */
  unlong          pmsgread;     /* 181 */
  unlong          msgdumped;    /* 185 */
  unlong          pmsgdumped;   /* 189 */
  unlong          uploaded;     /* 193 */
  unlong          puploaded;    /* 197 */
  unlong          downloaded;   /* 201 */
  unlong          pdownloaded;  /* 205 */
  unlong          kbup;         /* 209 */
  unlong          pkbup;        /* 213 */
  unlong          kbdown;       /* 217 */
  unlong          pkbdown;      /* 221 */
  unlong          resume;       /* 225 */
  unlong          access;       /* 229 */ /* See uacc_* below */
  unlong          utoggles;     /* 233 */ /* See utog_* below */
  unlong          envpos;       /* 237 */
  unlong          lasttime;     /* 241 */
  unlong          firsttime;    /* 245 */
  unlong          account;      /* 249 */
  unlong          todaydown;    /* 253 */
  unlong          userbits;     /* 257 */ /* Feel free to use this variable as you want - just note that somebody else might also use it */
  byte            uempty[8];    /* 261 */
  unlong          bzlong;       /* 269 */ /* tampered check */
};                              /* 273 */

#define uacc_sysop_mask         0x000000FF
#define uacc_dos                0x00000001
#define uacc_confs              0x00000002
#define uacc_files              0x00000004
#define uacc_priv               0x00000008
#define uacc_pass               0x00000010
#define uacc_useredit           0x00000020
#define uacc_fido               0x00000040
#define uacc_chat               0x00000080
#define uacc_download           0x40000000
#define uacc_upload             0x80000000

#define utog_not_insert         0x00000001
#define utog_indent             0x00000002
#define utog_xydisp             0x00000004
#define utog_not_flash          0x00000008
#define utog_confs              0x00000010
#define utog_expert_bit1        0x00000020
#define utog_colors             0x00000080
#define utog_review             0x00000100
#define utog_vt100key           0x00000200
#define utog_quote_include      0x00000400
#define utog_silent             0x00000800
#define utog_return             0x00001000
#define utog_expert_bit0        0x00002000

struct account {
  char    name[30];             /*   0 */
  longint money;                /*  30 */
  word    flags;                /*  34 */ /* bit 0:allow nega */
  word    users;                /*  36 */ /* # of users in this account */
};                              /*  38 */

#define account_negative        0x0001

struct msgrec {                 /* maindir/ *.dat */
  unlong          number;       /*   0 */
  char            msgfrom[72];  /*   4 */
  char            msgto[72];    /*  76 */
  char            subject[72];  /* 148 */
  unlong          msgfromn;     /* 220 */
  unlong          msgton;       /* 224 */
  unlong          lines;        /* 228 */
  unlong          offset;       /* 232 */
  unlong          dated;        /* 236 */
  unlong          replyto;      /* 240 */
  unlong          nextreply;    /* 244 */
  unlong          firstreply;   /* 248 */
  word            timegot;      /* 252 */
  word            status;       /* 254 */
  word            zonefrom;     /* 256 */
  word            netfrom;      /* 258 */
  word            nodefrom;     /* 260 */
  word            pointfrom;    /* 262 */
  unlong          msgid;        /* 264 */
};                              /* 268 */

struct hihhi {                  /* maindir/bbbshi.dat */
  word            times;        /*   0 */
  word            daynro;       /*   2 */
  char            name[30];     /*   4 */
  char            msg[80];      /*  34 */
};                              /* 114 */

struct noderec {                /* workdir/bbbsnode */
  word            split;        /*   0 */ /* reserved */
  byte            bstatus;      /*   2 */ /* Bit 0=FTPD, 1=mailsession, 2=groupchat_in_hydra, 3=keepalive, 4=keepidle, 5=bterm, 6=do_up_check, 7=fullmoon */
  byte            zstatus;      /*   3 */ /* 0=off,1=active,2=not,3=writing,4=grab,5=down,6=up,7=chat,8=door,9=groupchat,10=telnet,11=shelled */
  unlong          speed;        /*   4 */ /* 0=local,1=net,2=daemon */
  word            time;         /*   8 */ /* hour*0x100+min */
  word            endtime;      /*  10 */ /* when downloading, hour*0x100+min */
  char            nick[11];     /*  12 */
  char            sex;          /*  23 */ /* 1=male, 0=female */
  char            realname[30]; /*  24 */
  unlong          idle;         /*  54 */ /* timestamp (time()), ignore diffs less than 120 seconds! */
};                              /*  58 */

struct chatrec {                /* workdir/bbbsmsg.* */
  long            node;         /*   0 */
  word            filter;       /*   4 */
  char            msg[256];     /*   6 */
  char            prefix[80];   /* 262 */ /* "#[* ] :nick{ color}" for feelings, "#channel:nick" for chat, "nick" for private, "*channel:nick" for info messages */
};                              /* 342 */

struct bstatrec {               /* maindir/bbbsstat.dat */
  unlong          data [7]      /*   0 */ /* day (6=today) */
                       [3]                /* user/sysop/fido */
                       [4];               /* date/min/call/msg   date=day+31*month+372*year */
};                              /* 336 */

struct cfgrec2 {
  unlong          count_of_confs;   /*   0 */
  unlong          post_conf;        /*   4 */
  unlong          resume_conf;      /*   8 */
  unlong          fileinfo_conf;    /*  12 */
};                                  /*  16 */

struct confrec1 {
  unlong          lastread;         /*   0 */
  unlong          status;           /*   4 */
  char            confname[72];     /*   8 */
  char            description[72];  /*  80 */
};                                  /* 152 */

struct confrec2 {
  char            fidopath[72];     /*   0 */
  char            nntpname[72];     /*  72 */
  unlong          nntpnumber;       /* 144 */
  byte            charsets;         /* 148 */ /* hi: bbbs->msg, lo: bbbs<-msg */
  byte            nntphost;         /* 149 */
  byte            nodenumber;       /* 150 */
  byte            originnumber;     /* 151 */
  word            moderator_zone;   /* 152 */
  word            moderator_net;    /* 154 */
  word            moderator_node;   /* 156 */
  word            moderator_point;  /* 158 */
  unlong          bpc_min;          /* 160 */
  unlong          bpc_max;          /* 164 */
};                                  /* 168 */

/* next: 3084/4300 */

struct global_config {
  char    bbbs_name[30],                                /*   0     */
          sysop_name[30],                               /*   1     */
          newu_account[30],                             /* 591     */
          closed_password[8],                           /*   2     */
          grabfile[14],                                 /*   3     */
          maindir[70],                                  /*  10     */
          updir[70],                                    /*  12     */
          tempdir[70],                                  /*  14     */
          menudir[70],                                  /* 278     */
          origins[10][60],                              /*  26- 35 */
          tickdir[70],                                  /* 280     */
          netmail[70],                                  /* 281     */
          extlog[70],                                   /* 526     */
          inetlog[70],                                  /*3032     */
          tmp_in_pkt[70],                               /* 502     */
          tmp_out_pkt[70],                              /* 503     */
          bundle_dir[70],                               /* 504     */
          badechodir[70],                               /* 505     */
          badsecuredir[70],                             /* 506     */
          sitename[60],                                 /* 306     */
          location[60],                                 /* 307     */
          phone[60],                                    /* 308     */
          speed[60],                                    /* 309     */
          flags[60],                                    /* 310     */
          organization[60],                             /* 617     */
          hostname[60],                                 /* 618     */
          my_ip[60],                                    /*3080     */
          remotedomain[60],                             /* 952     */
          ircserver[60],                                /*3037     */
          cdtempdrives[40],                             /* 612     */
          akam[num_of_akas][60],                        /* 900-943, old 311-320 */
          freq_magic[70],                               /* 357     */
          freq_normal[70],                              /* 358     */
          dialconvfrom[10][27],                         /* 337-346 */
          dialconvto[10][27],                           /* 347-356 */
          yell_tune[128],                               /*3066     */
          faxdir[70],                                   /* 414     */
          feelingsdir[70],                              /* 3008    */
          scriptdir[70],                                /* 3009    */
          btermdown[70],                                /* 417     */
          btermup[70],                                  /*3071     */
          inbound[70];                                  /* 282     */
  word    max_nodes,                                    /*  36     */
          bankmax,                                      /*  40     */
          whodown_size,                                 /* 291     */
          maxbundlesize,                                /* 615     */
          boguspktsize,                                 /* 530     */
          smtpmaxsize,                                  /*3019     */
          tranxzone,                                    /* 302     */
          tranxnet,                                     /* 303     */
          tranxnode,                                    /* 304     */
          tranxpoint,                                   /* 305     */
          max_allfix_reply_size,                        /*3072     */
          freqlimit[3][4],                              /* 321-332 */
          limits_kbday[num_of_limits],                  /*3100-3217, old 542-557 */
          cost_start[num_of_limits],                    /*3300-3417, old 558-573 */
          cost_min[num_of_limits],                      /*3500-3617, old 574-589 */
          cost_hour[num_of_limits],                     /*3700-3817, old 596-611 */
          zone[num_of_akas],                            /* 700-743, old 41- 50 */
          net[num_of_akas],                             /* 750-793, old 51- 60 */
          node[num_of_akas],                            /* 800-843, old 61- 70 */
          point[num_of_akas];                           /* 850-893, old 71- 80 */
  integer newu_time;                                    /*  81     */
  byte    bankrate,                                     /*  84     */
          desc_max_lines,                               /* 289     */
          present_akas,                                 /* 425     */
          allow_internet[4],                            /*3074-3077*/
          limits_byte[num_of_limits],                   /*3900-4017, old 118-133 */
          limits_file[num_of_limits],                   /*4100-4217, old 102-117 */
          yell_length,                                  /*3067     */
          rescantime,                                   /* 300     */
          busydelay,                                    /* 333     */
          tries_busy,                                   /* 334     */
          tries_bad,                                    /* 522     */
          flood_max_count,                              /*3038     */
          max_open_files,                               /* 507     */
          bogus_dupes,                                  /* 529     */
          hydra_tx,                                     /* 520     */
          hydra_rx;                                     /* 521     */
  unlong  newu_access,                                  /*  86     */
          faxerror,                                     /* 416     */
          mailerror,                                    /* 301     */
          usermailerror,                                /* 411     */
          gtoggles,                                     /*  below  */
          htoggles;                                     /*  below  */
};

#define cfgg_nntp_cleanfeed          0x00000001         /*3078     */
#define cfgg_show_privates           0x00000002         /*  92     */
#define cfgg_brobocop                0x00000004         /*  93     */
#define cfgg_show_empty              0x00000010         /*  97     */
#define cfgg_pack_messages           0x00000040         /* 290     */
#define cfgg_fido_hydra              0x00000080         /* 292     */
#define cfgg_fido_zedzap             0x00000100         /* 293     */
#define cfgg_fido_tranx              0x00000200         /* 294     */
#define cfgg_fido_unlistnode         0x00000400         /* 295     */
#define cfgg_fido_unlistpoint        0x00000800         /* 296     */
#define cfgg_fido_unprotnode         0x00001000         /* 297     */
#define cfgg_fido_freq_answering     0x00002000         /* 298     */
#define cfgg_fido_freq_calling       0x00004000         /* 299     */
#define cfgg_show_sysop_in_stats     0x00008000         /* 410     */
#define cfgg_bmt_check_destination   0x00010000         /* 413     */
#define cfgg_disable_newu_address    0x00020000         /* 419     */
#define cfgg_disable_newu_birthday   0x00040000         /* 420     */
#define cfgg_users_are_hidden        0x00080000         /* 421     */
#define cfgg_upload_scan             0x00100000         /* 422     */
#define cfgg_poll_all_crashes        0x00200000         /* 423     */
#define cfgg_delete_dup_uploads      0x00400000         /* 424     */
#define cfgg_savebad                 0x00800000         /* 508     */
#define cfgg_savesecure              0x01000000         /* 509     */
#define cfgg_bmt_log_headers         0x02000000         /* 531     */
#define cfgg_no_remote_sysop         0x04000000         /* 532     */
#define cfgg_bogus_save_netmail      0x08000000         /* 541     */
#define cfgg_kb_day_relative         0x10000000         /* 590     */
#define cfgg_global_download         0x20000000         /* 613     */
#define cfgg_nntp_gateway            0x40000000         /* 950     */
#define cfgg_smtp_gateway            0x80000000         /* 951     */

#define cfgh_nntp_save_headers       0x00000001         /* 3001    */
#define cfgh_smtp_save_headers       0x00000002         /* 3002    */
#define cfgh_chat_uses_time          0x00000004         /* 3003    */
#define cfgh_sysnote_msg             0x00000008         /* 3004    */
#define cfgh_eom_to_uucp             0x00000010         /* 3006    */
#define cfgh_use_nodenum_not_nick    0x00000020         /* 3007    */
#define cfgh_allow_all_names         0x00000040         /* 3010    */
#define cfgh_grab_is_free            0x00000100         /* 3031    */
#define cfgh_uploader_owns_file      0x00000200         /* 3034    */
#define cfgh_upload_uses_time        0x00000400         /* 3056    */
#define cfgh_no_anonftp              0x00000800         /* 3081    */
#define cfgh_no_anonwww              0x00001000         /* 3082    */
#define cfgh_ignore_pkt_destination  0x00002000         /* 3083    */

struct local_config {
  char    modem_init_string1[60],                       /*   4     */
          modem_init_string2[60],                       /* 283     */
          modem_init_string3[60],                       /* 284     */
          modem_hangup_string[60],                      /*   5     */
          modem_busy_string[60],                        /*   6     */
          modem_aftercall[60],                          /* 527     */
          voice_init[80],                               /*3020     */
          voice_beep[60],                               /*3021     */
          voice_play[60],                               /*3022     */
          voice_save[60],                               /*3023     */
          voice_go_voice[60],                           /*3027     */
          voice_go_data[60],                            /*3028     */
          data_after_voice[60],                         /* 533     */
          voicedir[70],                                 /* 536     */
          voicegreetings[70],                           /* 537     */
          bterm_init[60],                               /* 427     */
          dont_crash_flags[60],                         /* 426     */
          only_crash_flags[60],                         /* 593     */
          sms_server[32],                               /*3068     */
          sms_sender[32],                               /*3069     */
          fd_dobbs[70],                                 /*   7     */
          logfile[70],                                  /*   8     */
          spyfile[70],                                  /* 3000    */
          grabdir[70],                                  /*  11     */
          menudir[70],                                  /*  13     */
          newdir[70],                                   /*  15     */
          voicepass[5],                                 /* 540     */
          lockpass[8],                                  /*3035     */
          ring_dont_answer[60],                         /*3040     */
          ring_regexp[5][35],                           /*3041-3045*/
          ring_answer[5][35],                           /*3046-3050*/
          dial1[5][20],                                 /* 447-451 */
          dial2[5][10],                                 /* 452-456 */
          dial3[5][50],                                 /* 457-461 */
          macros[10][50],                               /*  16- 25 */
          hotlogin[10][40];                             /* 429-438 */
  word    event_dial_zone[num_of_events],               /* 1000-1099, old 370-379 */
          event_dial_net[num_of_events],                /* 1100-1199, old 380-389 */
          event_dial_node[num_of_events],               /* 1200-1299, old 390-399 */
          event_dial_point[num_of_events],              /* 1300-1399, old 400-409 */
          voice_min_size,                               /*3030     */
          locktimeout,                                  /*3036     */
          base_address;                                 /*  39     */
  byte    irq,                                          /*  82     */
          ring_count[5],                                /*3051-3055*/
          pollrate,                                     /*  83     */
          checksleep,                                   /* 287     */
          ringingcount,                                 /* 539     */
          screensaver_timeout,                          /* 285     */
          aftercall_lines,                              /* 528     */
          voicecompression,                             /* 538     */
          voice_modem,                                  /*3024     */
          voice_speaker,                                /*3025     */
          voice_mic,                                    /*3026     */
          rush_hour[24],                                /* 134-157 */
          event_day[num_of_events],                     /* 1400-1499, old 158-167 */
          event_start_hour[num_of_events],              /* 1500-1599, old 168-177 */
          event_start_min[num_of_events],               /* 1600-1699, old 178-187 */
          event_end_hour[num_of_events],                /* 1700-1799, old 188-197 */
          event_end_min[num_of_events],                 /* 1800-1899, old 198-207 */
          event_toggles[num_of_events],                 /*  below  */
          event_last_dial[num_of_events],               /* 2000-2099, old 360-369 */
          event_last_day[num_of_events],                /* 2100-2199, old 248-257 */
          amy_priorities[8],                            /*3057-3064*/
          nt_priorities[8],                             /*3011-3018*/
          priorities[8];                                /* 439-446 */
  unlong  start_speed,                                  /*  37     */
          event_errorlevel[num_of_events],              /* 1900-1999, old 208-217 */
          faxbaud,                                      /* 415     */
          min_speed,                                    /*  38     */
          ltoggles;                                     /*  below  */
};

#define cfgl_event_dont_allow_users  0x01               /* 2200-2299, old 218-227 */
#define cfgl_event_flexible          0x02               /* 2300-2399, old 228-237 */
#define cfgl_event_must              0x04               /* 2400-2499, old 238-247 */
#define cfgl_event_dont_allow_mail   0x08               /* 2500-2599, old 462-471 */
#define cfgl_event_dont_allow_pickup 0x10               /* 2600-2699, old 472-481 */
#define cfgl_event_dont_send_cm      0x20               /* 2700-2799, old 482-491 */
#define cfgl_event_send_all          0x40               /* 2800-2899, old 492-501 */
#define cfgl_event_dont_allow_freq   0x80               /* 2900-2999, old 510-519 */

#define cfgl_local_bell              0x00000001         /*  87     */
#define cfgl_rts_cts                 0x00000002         /*  88     */
#define cfgl_reset_speed             0x00000004         /*  89     */
#define cfgl_hangup_at_exit          0x00000008         /*  90     */
#define cfgl_local_sysop_keys        0x00000020         /*  96     */
#define cfgl_local_echo              0x00000040         /* 100     */
#define cfgl_save_screen             0x00000080         /* 101     */
#define cfgl_null_modem_login        0x00000100         /* 288     */
#define cfgl_send_crashmail          0x00000200         /* 359     */
#define cfgl_backdoor                0x00000400         /* 412     */
#define cfgl_fast_fax                0x00000800         /* 418     */
#define cfgl_dont_check_carrier      0x00001000         /* 428     */
#define cfgl_nocarrier_is_busy       0x00002000         /* 523     */
#define cfgl_show_shell_output       0x00004000         /* 524     */
#define cfgl_allow_shell_break       0x00008000         /* 525     */
#define cfgl_slow_protocols          0x00010000         /* 592     */
#define cfgl_fax_receive_revbit      0x00020000         /* 594     */
#define cfgl_fax_send_revbit         0x00040000         /* 595     */
#define cfgl_rockwell_kludge         0x00100000         /* 616     */
#define cfgl_fix_rar_bug             0x00200000         /*3033     */
#define cfgl_callback                0x00400000         /*3039     */
#define cfgl_amy_lowcols             0x00800000         /*3065     */
#define cfgl_change_titlebar         0x01000000         /*3073     */
#define cfgl_require_callback        0x02000000         /*3079     */

struct comstruct {            /* for internal / named memory communication */
  unlong modemhead, modemtail, s, c;
  byte   modembuf[buffer_len];
};

struct mapstruct {            /* for internal / named memory communication */
  char buf_out[buffer_len];   /* server -> client */
  int out_head, out_tail;
  char buf_in[buffer_len];    /* client -> server */
  int in_head, in_tail;
  int servercarrier;
  int clientcarrier;
  char device[128];
};

#if !defined(__BBBS_NO_EXTERNS__)

extern  char                  *ver;           /* "3.42 1s", etc.          */
extern  char                  hexs[17];       /* "0123456789ABCDEF";      */
extern  char                  *vername;       /* "BBBS/D", "BBBS/2", etc. */

extern  struct userrec        u;
extern  struct global_config  cfgg;
extern  struct local_config   cfgl;
extern  struct cfgrec2        cfg2;
extern  struct noderec        node;
extern  struct fos_struct     fos_info;
extern  struct confrec1       **confs;
extern  FILE                  *grabfile;
extern  char                  *buffer, *outbuffer, *txt[txtsize+1], *txtstart, commands[commands_saved][128], local_buffer[128], comstring[256], temp_string[256], *bzsalias, *bzsset, binterface, *areamod, *groups, crrdir[128], realdir[128], holdreal[128], hddesc[128], serna[21], *chat_last_lines[chat_last_saved];
extern  unlong                tempsys, startdump, startread, startleft, realbaud, baud, lastcall, nodenumber, spymode, foryou, newavail, msgreaded[max_msg+1], lastreaded, curmsg, confnro, lastmsg, firstmsg, *lastr, unum, errl;
extern  word                  reserved1, reserved2, reserved3, reserved4, reserved5, gnubuf, serno, reduced, nextevent, bzltc;
extern  time_t                logintime;
extern  integer               lastwarn, temptime;
extern  byte                  chartable[512], jobtype, com, commands_in_memory, pagecounter, lastinchar, dinged, bzlts[2], tomenu, getdown, remotey, fossil, forcefossil, script_count, pktcount, currentevent, outputstopped, usercolors[usernumcolors+1];
extern  boolean               carrier, files_checked, desqview, quicklogin, paged, script_running, rupted, bzchanged, askpass, userlocked, debugmode, reducedevent, safeprint, allow_locking;
#if defined(__OS2__) && !defined(BAG) && !defined(BCFG4)
extern  int                   socket1;
extern  boolean               freemodem, thread_active, broken_pipe;
extern  HFILE                 modem;
extern  char                  comdevice[80];
extern  byte                  pipecommunication;
#endif
#if defined(__WINDOWS__) && !defined(BAG) && !defined(BCFG4)
extern  HANDLE                modem;
extern  char                  comdevice[80];
extern  byte                  pipecommunication;
extern  boolean               freemodem;
#endif
#if (defined(__AMIGA__) || defined(__LINUX__) || defined(__SOLARIS__) || defined(__UNIXWARE__) || defined(__SUNOS__)) && !defined(BAG) && !defined(BCFG4)
extern  int                   modem;
extern  char                  comdevice[80];
extern  byte                  pipecommunication;
extern  boolean               freemodem;
#endif
#if defined(__MSDOS__)
extern  char                  filna[13];
#else
extern  char                  filna[80];
extern  char                  resolv_name[32];
#endif

#endif

/******************************************************************************

areah2o4.dat  long highwater[count_of_confs];

areausr4.dat  struct {                                    (unum*count_of_confs+confnro)*8L
                unlong lastread;                                0
                unlong status;                                  4
              } userconfstat[count_of_confs];

areacfg4.dat  unlong count_of_confs;                       0
              unlong post_conf;                            4
              unlong resume_conf;                          8
              unlong fileinfo_conf;                       12
              struct {                                    16+confnum*320
                unlong lastread;                                0
                unlong status;                                  4
                char confname[72];                              8
                char description[72];                          80
                char fidopath[72];                            152
                char nntpname[72];                            224
                unlong nntpnumber;                            296
                byte charsets;                                300
                byte nntphost;                                301
                byte nodenumber;                              302
                byte originnumber;                            303
                word moderator_zone;                          304
                word moderator_net;                           306
                word moderator_node;                          308
                word moderator_point;                         310
                long bpc_min;                                 312
                long bpc_max;                                 316
              } confstats[count_of_confs];                    320

*******************************************************************************

              A Quick Guide to Convert C struct to Pascal Record

struct foobar {                   type foobar = record
  char b1;                          b1 : shortint;
  char b2[128];                     b2 : array[0..127] of char; { 128-1=127 }
  byte b3;                          b3 : byte;
  word b4;                          b4 : word;
  word b5[128];                     b5 : array[0..127] of word; { 128-1=127 }
  integer b6;                       b6 : integer;
  longint b7;                       b7 : longint;
  unlong b8;                        b8 : longint;               { unsigned }
};                                end;


You still have to convert C-alike null-terminated strings (b2) to
Pascal-alike strings.

*******************************************************************************

global_config and local_config structures are saved in following format:

 word     id;                       -- see comments
 byte     len;                      -- in bytes
 byte     data[];                   -- actual data for item id

So, after reading id and len you just fread(&config.id,1,len,file) or
fseek(len,SEEK_CUR,file).

******************************************************************************/

struct fos_struct {
  word            strsize;        /* size of the structure in bytes     */
  byte            majver;         /* FOSSIL spec driver conforms to     */
  byte            minver;         /* rev level of this specific driver  */
  char            *ident;         /* FAR pointer to ASCII ID string     */
  word            ibufr;          /* size of the input buffer (bytes)   */
  word            ifree;          /* number of bytes left in buffer     */
  word            obufr;          /* size of the output buffer (bytes)  */
  word            ofree;          /* number of bytes left in the buffer */
  byte            swidth;         /* width of screen on this adapter    */
  byte            sheight;        /* height of screen    "      "       */
  byte            speed;          /* ACTUAL speed, computer to modem    */
};

struct ftsc001 {
  char  fromuser[36];     /*   0 */
  char  touser[36];       /*  36 */
  char  subject[72];      /*  72 */
  char  datetime[20];     /* 144 */
  word  timesread;        /* 164 */ /* not used */
  word  destnode;         /* 166 */
  word  orignode;         /* 168 */
  word  cost;             /* 170 */ /* not used */
  word  orignet;          /* 172 */
  word  destnet;          /* 174 */
  word  destzone;         /* 176 */
  word  origzone;         /* 178 */
  word  destpoint;        /* 180 */
  word  origpoint;        /* 182 */
  word  replyto;          /* 184 */ /* not used */
  word  attribute;        /* 186 */
  word  nextreply;        /* 188 */ /* not used */
};                        /* 190 */

struct pktmessage {
  word version,           /*   0 */
       orignode,          /*   2 */
       destnode,          /*   4 */
       orignet,           /*   6 */
       destnet,           /*   8 */
       attribute,         /*  10 */
       cost;              /*  12 */
                          /*  14 */
  /* byte datetime[20]; */ /* OPUS does it wrong! */
  /* char to[], from[], subject[], text[]; */
};

#define fido_attr_private       0x00000001L
#define fido_attr_crash         0x00000002L
#define fido_attr_sent          0x00000008L
#define fido_attr_file          0x00000010L
#define fido_attr_kill          0x00000080L
#define fido_attr_local         0x00000100L
#define fido_attr_hold          0x00000200L
#define fido_attr_freq          0x00000800L
#define fido_attr_rrr           0x00001000L
#define fido_attr_isrr          0x00002000L
#define fido_attr_delete        0x00010000L
#define fido_attr_trunc         0x00020000L
#define fido_attr_direct        0x00040000L
#define fido_attr_lock          0x00080000L
#define fido_attr_immediate     0x00100000L

struct packetheader2 {
  word orignode,          /*   0 */
       destnode,          /*   2 */
       year,              /*   4 */
       month,             /*   6 */
       day,               /*   8 */
       hour,              /*  10 */
       minute,            /*  12 */
       second,            /*  14 */
       baud,              /*  16 */
       version,           /*  18 */
       orignet,           /*  20 */
       destnet;           /*  22 */
  byte prodcode,          /*  24 */
       serial,            /*  25 */
       password[8];       /*  26 */
  word origzone,          /*  34 */
       destzone;          /*  36 */
  byte empty[20];         /*  38 */
};                        /*  58 */

struct packetheader2p {
  word orignode,          /*   0 */
       destnode,          /*   2 */
       year,              /*   4 */
       month,             /*   6 */
       day,               /*   8 */
       hour,              /*  10 */
       minute,            /*  12 */
       second,            /*  14 */
       baud,              /*  16 */
       version,           /*  18 */
       orignet,           /*  20 */
       destnet;           /*  22 */
  byte prodcodel,         /*  24 */
       revisionh,         /*  25 */
       password[8];       /*  26 */
  word origzoneq,         /*  34 */
       destzoneq,         /*  36 */
       auxnet,            /*  38 */
       cwcopy;            /*  40 */
  byte prodcodeh,         /*  42 */
       revisionl;         /*  43 */
  word cw,                /*  44 */
       origzone,          /*  46 */
       destzone,          /*  48 */
       origpoint,         /*  50 */
       destpoint;         /*  52 */
  unlong psd;             /*  54 */
};                        /*  58 */

struct packetheader22 {
  word orignode,          /*   0 */
       destnode,          /*   2 */
       origpoint,         /*   4 */
       destpoint;         /*   6 */
  unlong res1,            /*   8 */
       res2;              /*  12 */
  word subversio,         /*  16 */
       version,           /*  18 */
       orignet,           /*  20 */
       destnet;           /*  22 */
  byte prodcode,          /*  24 */
       revision,          /*  25 */
       password[8];       /*  26 */
  word origzone,          /*  34 */
       destzone;          /*  36 */
  byte origdomain[8],     /*  38 */
       destdomain[8];     /*  46 */
  unlong psd;             /*  54 */
};                        /*  58 */

