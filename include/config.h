/************************************************************************
 *   IRC - Internet Relay Chat, include/config.h
 *   Copyright (C) 1990 Jarkko Oikarinen
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation; either version 1, or (at your option)
 *   any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

#ifndef	__config_include__
#define	__config_include__

#include "setup.h"

/***************** MAKE SURE THIS IS CORRECT!!!!!!!!! **************/
/* ONLY EDIT "HARD_FDLIMIT_" and "INIT_MAXCLIENTS" */

#define HARD_FDLIMIT_	1024
#define INIT_MAXCLIENTS	800

/*
 * This is how many 'buffer connections' we allow... 
 * Remember, MAX_BUFFER + MAX_CLIENTS can't exceed HARD_FDLIMIT :)
 */
#define MAX_BUFFER	60

/* Don't change this... */
#define HARD_FDLIMIT	(HARD_FDLIMIT_ - 10)
#define MASTER_MAX	(HARD_FDLIMIT - MAX_BUFFER)
/*******************************************************************/

#include "defs.h"

/* Also define this for SunOS 4.1.4_DBE */

#undef SUNDBE			

#ifdef SUNDBE
#define FD_SETSIZE HARD_FDLIMIT_
#define NOFILE HARD_FDLIMIT_
#endif /* SUNDBE */

/* USE_FAST_FD_ISSET - experimental! - crawl inside of FD_ISSET macro
 * This one is experimental, though it works on SUNOS and freebsd
 * It is NOT portable, and "crawls" carnally inside the FD_ISSET macro
 * but should be a faster way of doing select() in s_bsd.c
 *
 * The idea is to pre-calculate the bit map mask needed shifting it
 * over as needed, the FD_ISSET calculates the bitmask and array
 * offset every single time.
 */
#undef USE_FAST_FD_ISSET

/* DPATH SPATH CPATH MPATH KPATH - directoy and files locations
 * Full pathnames and defaults of irc system's support files. Please note that
 * these are only the recommended names and paths. Change as needed.
 * You must define these to something, even if you don't really want them.
 *
 * DPATH = directory,
 * SPATH = server executable,
 * CPATH = conf file,
 * MPATH = MOTD
 * KPATH = kline conf file 
 * 
 * For /restart to work, SPATH needs to be a full pathname
 * (unless "." is in your exec path). -Rodder
 * Leave KPATH undefined if you want klines in main conf file.
 * HPATH is the opers help file, seen by opers on /quote help.
 *
 * -Dianora
 */

#define	DPATH	"./"
#define	SPATH	"/usr/local/ircd/ircd"
#define	CPATH	"ircd.conf"
#define KPATH   "kline.conf"
#define	MPATH	"ircd.motd"
#define	LPATH	"ircd.log"
#define	PPATH	"ircd.pid"
#define HPATH	"opers.txt"

/* Contributed by Shadowfax */
/* LOCKFILE
 *
 * There is a saying  about too many cooks in the kitchen.  It happens quite
 * often while one person is editting the ircd.conf, another person adds a
 * kline using the /quote kline command.  Or, another person tries to edit
 * the config file at the same time you are.  This should deal with the
 * problem.  When a kline is added, rather than writing it to a file, it is
 * stuck into a list.  At rehash time, the list is checked, and klines are
 * added to the file if it can be locked.  This way, every time a kline is
 * added or the server is rehashed (which often happens after editting the
 * conf file), pending klines will be added.
 *
 * If you do not wish to use this feature, leave LOCKFILE #undef 
 */
#define LOCKFILE "/tmp/ircd.conf.lock"

/* UNKLINE - /quote unkline - remove klines on the fly
 * if you choose to support this, an oper can do a /quote UNKLINE
 * of an exact matching KLINE to remove the kline
 */
#define UNKLINE

/* CHECK_PENDING_KLINES - a timer (IN MINUTES)
 * that will try to write out pending klines if
 * there are still klines in queue
 */
#define	CHECK_PENDING_KLINES	10	/* in minutes */


/* FNAME_USERLOG and FNAME_OPERLOG - logs of local USERS and OPERS
 * Define this filename to maintain a list of persons who log
 * into this server. Logging will stop when the file does not exist.
 * Logging will be disable also if you do not define this.
 * FNAME_USERLOG just logs user connections, FNAME_OPERLOG logs every
 * successful use of /oper.  These are either full paths or files within DPATH.
 *
 * These need to be defined if you want to use SYSLOG logging, too.
 */
#define FNAME_USERLOG "/usr/local/ircd/users" /* */
#define FNAME_OPERLOG "/usr/local/ircd/opers" /* */

/* ANTI_IP_SPOOF - protects against TCP sequence guessing attacks
 * Define this if you want the server to send a random ping at USER or NICK 
 * to defeat IP sequence spoofers. Note, this will stop _some_ very few
 * clients from using the server. MIRC/IRCII clients are fine with this though
 *
 * N.B. This code would be totally unnecessary if you only get
 * the proper anti-sequence prediction patches from your OS vendor,
 * but if you absolutely need this until you do, here it is.
 */
#undef ANTI_IP_SPOOF

/* FOLLOW_IDENT_RFC
 * Defining this allows all users to set whatever username they wish.
 */
#undef FOLLOW_IDENT_RFC 

/* RFC1035_ANAL
 * Defining this causes ircd to reject hostnames with non-compliant chars.
 * undef'ing it will allow hostnames with _ or / to connect
 */
#define RFC1035_ANAL

/* WARN_NO_NLINE
 * Define this if you want ops to get noticed about "things" trying to
 * connect as servers that don't have N: lines.  Twits with misconfigured
 * servers can get really annoying with enabled.
 */
#define WARN_NO_NLINE

/* CUSTOM_ERR - colorful notice/error/messages
 * Defining this will use custom notice/error/messages from include/s_err.h
 * instead of stock ones in ircd/s_err.c.  If you prefer the "colorful"
 * messages that Hybrid is known for, or if you wish to customize the
 * messages, define this.  Otherwise leave it undef'd for plain ole
 * boring messages.
 */
#undef CUSTOM_ERR


/* FAILED_OPER_NOTICE - send a notice to all opers when someone
 * tries to /oper and uses an incorrect password.
 */
#define FAILED_OPER_NOTICE

/* CLIENT_SERVER - Don't be so fascist about idle clients ;)
 * changes behaviour of HTM code to make clients lag less.
 */
#define CLIENT_SERVER

/* RK_NOTICES - Show notices when rline/kline'd connections are denied.
 * This generates a TON of crap, I reccomend leaving it undef'd. 
 */
#undef RK_NOTICES

/* ACKPATCH - no longer used - see new Y: line features in example.conf
 *
 * Anti Clones Kludge - restrict a domain to one connection per ip#
 * For this to work they should use the password 'ONE' in the I: line
 * Example, I:*@*.gnn.com:ONE:*@*.gnn.com:0:10
 * #define ACKPATCH
 * 
 * always enabled, and is done as a Y line in the connfreq
 */


/* NO_MIXED_CASE - reject mixed case usernames
 * define this if you wish to reject usernames like: FuckYou which
 * don't have all one case
 */
#undef NO_MIXED_CASE

/* IGNORE_FIRST_CHAR -
 * define this for NO_MIXED_CASE if you wish to ignore the first character
 */
#define IGNORE_FIRST_CHAR

/* NO_SPECIAL - reject usernames containing strange chars
 * define this if you want no "special" characters in usernames.
 * A character is "special" if it's not "a-z", "A-Z", "0-9", ".", "-",
 * and "_"
 */
#undef NO_SPECIAL

/* REJECT_IPHONE - reject I-phone clients
 * Define if you want to reject I-phoners
 */
#define REJECT_IPHONE

/* CLIENT_NOTICES - no longer used - use umode +/- c
 *
 * Define this if you wish to see client connecting and exiting notices.
 * Always defined now. If you don't want to see them, /umode -c silly.
 *
 * NOTE NOTE NOTE
 * hybrid ircd will display the users IP# in square brackets
 * after original notice. I'm sorry if this breaks scripts, but
 * It also means a lot less I/O doing DNS spoof checking if you wish.
 * -Dianora
 */

/* DLINES_IN_KPATH - put (/quote) DLINES in your KPATH file.
 * Define this if you want DLINES in your KPATH file.  Otherwise DLINES
 * will be added to the main ircd.conf
 */
#ifdef KPATH
#undef DLINES_IN_KPATH
#endif /* KPATH */

/* NO_LOCAL_KLINE - restrict QUOTE KLINE to global opers
 * Define this if you don't want local opers using /QUOTE KLINE.
 */
#undef NO_LOCAL_KLINE

/* GLINES - global Kline-like bans
 * Define this if you want GLINE support
 * when this is defined, 3 completely different opers from
 * three different servers must do the identical GLINE in order
 * for the G line to take effect. I don't know what comstud did
 * (nor do I really care) I have not looked at his code. - Dianora
 */
#define GLINES

/* GLINE_TIME - local expire time for GLINES
 * As configured here, a GLINE will last 12 hours
 */
#define GLINE_TIME	(12*3600)

/* HIGHEST_CONNECTION - track highest connection count
 * Define this if you want to keep track of your max connections.
 */
#define HIGHEST_CONNECTION

/* USERNAMES_IN_TRACE - show usernames in trace
 * Define this if you want to see usernames in /trace.
 */
#define USERNAMES_IN_TRACE

/* BAN_INFO - Shows you who and when someone did a ban
 */
#define BAN_INFO

/* USE_UH - include user@host for BAN_INFO
 * define this if you want to use n!u@h for BAN_INFO
 */
#define USE_UH

/* TOPIC_INFO - Shows you who and when someone set the topic
 */
#define TOPIC_INFO

/* ANTI_NICK_FLOOD - prevents nick flooding
 * define if you want to block local clients from nickflooding
 */
#define ANTI_NICK_FLOOD
/* defaults allow 5 nick changes in 20 seconds */
#define MAX_NICK_TIME 20
#define MAX_NICK_CHANGES 5 

/* DO_IDENTD - check identd
 * if you undefine this, ircd will never check identd regardless of
 * @'s in I:lines
 */
#define DO_IDENTD

/* K_COMMENT_ONLY - 2nd field in K-line is ALWAYS a comment.
 * If you define this, ircd will NOT put ^O's in K: line comments.
 * Undef this if you think you might ever run a non-hybrid
 * or non-th ircd.
 */
#define K_COMMENT_ONLY

/* KLINE_WITH_REASON - show comment to client on exit
 * define this if you want users to exit with the kline/dline reason
 * (i.e. instead of "You have been K-lined" they will see the reason
 * and to see the kline/dline reason when they try to connect
 * It's a neat feature except for one thing... If you use a tcm
 * and it shows the nick of the oper doing the kline (as it does by default)
 * Your opers can be hit with retalitation... Or if your opers use
 * scripts that stick an ID into the comment field. etc. It's up to you
 * whether you want to use it or not.
 */
#define KLINE_WITH_REASON

/* TIMED_KLINES - bad bad evil
 * PLEASE don't define this unless you are nuts or absolutely need
 * timed kline support. its not necessary, and is a CPU hog... :-)
 * besides. it hasn't been tested. if you use it, you debug it :-) -Dianora
 * THIS IS NOT THE SAME THING AS TEMP KLINES. -Rodder
 */
#undef TIMED_KLINES

/* NON_REDUNDANT_KLINES - If you want the server to flag and not apply
 * redundant klines
 */
#define NON_REDUNDANT_KLINES

/* BOTCHECK - rudimentary bot checking
 */
#define BOTCHECK

/* IDENTD_COMPLAIN - yell at users that don't have identd installed
 */
#undef IDENTD_COMPLAIN

/* CLIENT_COUNT - keep a running count of Local & Global users
 *                also redefine the /USERS command
 */
#define CLIENT_COUNT
#ifdef CLIENT_COUNT
#undef HIGHEST_CONNECTION
#endif

/* X_LINES_OPER_ONLY - Allow only local opers to see these stats
 *
 * B_LINES - ComStud's B-lines (bot exemption lines)
 * E_LINES - ComStud's E-lines (exception lines)
 * F_LINES - Taner's F-lines (super-exception lines)
 *  Any one with an F line can almost always get on the server, as
 *  some file descriptors are reserved for people with this F line
 *  especially useful for your opers
 */
#define B_LINES_OPER_ONLY
#define E_LINES_OPER_ONLY
#define F_LINES_OPER_ONLY

/* STATS_NOTICE - See a notice when a user does a /stats
 */
#define STATS_NOTICE

/* LINKS_NOTICE - See a notice when a user does a /links
 */
#define LINKS_NOTICE

/* LINK_WAIT - minimum seconds between use of LINKS
 * Allow a links request only every LINK_WAIT seconds to
 * discourage link lookers. IMO its still legit for "normal" non-opers
 * to do /links but not too often -Dianora
 */
#define LINK_WAIT 10

/* EXTRA_BOT_NOTICES - Have the server send extra bot notices?
 */
#undef EXTRA_BOT_NOTICES

/* BOT_GCOS_WARN - Check connecting clients gcos for possible bot ID's?
 */
#define BOT_GCOS_WARN

#if defined(BOT_GCOS_WARN) && !defined(EXTRA_BOT_NOTICES)
#define EXTRA_BOT_NOTICES
#endif

/* WHOIS_NOTICE - Shows a notice to an oper when a user does a
 * /whois on them (must be umode +r)
 */
#define WHOIS_NOTICE

/* SHOW_HEADERS - Shows messages like "looking up hostname" 
 */
#define SHOW_HEADERS

/* FORCE_MOTD - Force users to see the MOTD (use NOTICE)
 */
#define FORCE_MOTD

/* NO_OPER_FLOOD - disable flood control for opers
 * define this to remove flood control for opers
 */
#define NO_OPER_FLOOD

/* SHOW_UH - show the user@host everywhere
 */
#define SHOW_UH
#ifdef SHOW_UH
#define USERNAMES_IN_TRACE
#endif

/* SHOW_INVISIBLE_LUSERS - show invisible clients in LUSERS
 * As defined this will show the correct invisible count for anyone who does
 * LUSERS on your server. On a large net this doesnt mean much, but on a
 * small net it might be an advantage to undefine it.
 */
#define	SHOW_INVISIBLE_LUSERS

/* NO_DEFAULT_INVISIBLE - clients not +i by default
 * When defined, your users will not automatically be attributed with user
 * mode "i" (i == invisible). Invisibility means people dont showup in
 * WHO or NAMES unless they are on the same channel as you.
 */
#define	NO_DEFAULT_INVISIBLE

/* TS_WARNINGS - warn opers about broken/poorly hacked servers
 * When defined, +s users are warned of some things that should never
 * happen on an all-TS net.  Currently these are: server-generated MODE +o,
 * new nicks without a TS, and remote JOINs for non-existing channels.
 * This is useful to track down anomalies;  undefine it on a mixed TS/nonTS
 * net or you'll get a lot of warnings!
 */
#define TS_WARNINGS

/* OPER_KILL OPER_REHASH OPER_RESTART OPER_DIE OPER_REMOTE -
 *      restrict what local global-Opers can do
 *
 * If you dont believe operators should be allowed to use the /KILL command
 * or believe it is uncessary for them to use it, then leave OPER_KILL
 * undefined. This will not affect other operators or servers issuing KILL
 * commands however.  OPER_REHASH and OPER_RESTART allow operators to
 * issue the REHASH and RESTART commands when connected to your server.
 * Left undefined they increase the security of your server from wayward
 * operators and accidents.  Defining OPER_REMOTE removes the restriction
 * that O-lines only become fully effective for people on the 'same network'
 * as the server.  Undefined, it increases the secrity of the server by
 * placing restrictions on where people can use operator powers from.
 */
#define	OPER_KILL
#define	OPER_REHASH
#define	OPER_RESTART
#define	OPER_DIE
#define	OPER_REMOTE

/* LOCOP_REHASH LOCOP_RESTART LOCOP_DIE - restrict local opers
 * The 'LOCOP_' #defines are for making the respective commands available
 * to 'local' operators.  See above section.
 */
#define	LOCOP_REHASH
#undef	LOCOP_RESTART
#undef	LOCOP_DIE

/* MAXIMUM LINKS - max links for class 0 if no Y: line configured
 *
 * This define is useful for leaf nodes and gateways. It keeps you from
 * connecting to too many places. It works by keeping you from
 * connecting to more than "n" nodes which you have C:blah::blah:6667
 * lines for.
 *
 * Note that any number of nodes can still connect to you. This only
 * limits the number that you actively reach out to connect to.
 *
 * Leaf nodes are nodes which are on the edge of the tree. If you want
 * to have a backup link, then sometimes you end up connected to both
 * your primary and backup, routing traffic between them. To prevent
 * this, #define MAXIMUM_LINKS 1 and set up both primary and
 * secondary with C:blah::blah:6667 lines. THEY SHOULD NOT TRY TO
 * CONNECT TO YOU, YOU SHOULD CONNECT TO THEM.
 *
 * Gateways such as the server which connects Australia to the US can
 * do a similar thing. Put the American nodes you want to connect to
 * in with C:blah::blah:6667 lines, and the Australian nodes with
 * C:blah::blah lines. Have the Americans put you in with C:blah::blah
 * lines. Then you will only connect to one of the Americans.
 *
 * This value is only used if you don't have server classes defined, and
 * a server is in class 0 (the default class if none is set).
 *
 */
#define MAXIMUM_LINKS 1

/* HUB - enable server-server routing
 * If your server is running as a a HUB Server then define this.
 * A HUB Server has many servers connect to it at the same as opposed
 * to a leaf which just has 1 server (typically the uplink). Define this
 * correctly for performance reasons.
 */
#undef	HUB

/* R_LINES - support for R: LINES
 * The conf file now allows the existence of R lines, or
 * restrict lines.  These allow more freedom in the ability to restrict
 * who is to sign on and when.  What the R line does is call an outside
 * program which returns a reply indicating whether to let the person on.
 * Because there is another program involved, Delays and overhead could
 * result. It is for this reason that there is a line in config.h to
 * decide whether it is something you want or need. -Hoppie
 *
 * The default is no R_LINES as most people probably don't need it. --Jto
 */
#undef	R_LINES

#ifdef	R_LINES
/* R_LINES_REHASH R_LINES_OFTEN - when to check R: lines
 * Also, even if you have R lines defined, you might not want them to be 
 * checked everywhere, since it could cost lots of time and delay.  Therefore, 
 * The following two options are also offered:  R_LINES_REHASH rechecks for 
 * R lines after a rehash, and R_LINES_OFTEN, which rechecks it as often
 * as it does K lines.  Note that R_LINES_OFTEN is *very* likely to cause 
 * a resource drain, use at your own risk.  R_LINES_REHASH shouldn't be too
 * bad, assuming the programs are fairly short.
 */
#define R_LINES_REHASH
#define R_LINES_OFTEN
#endif

/* CMDLINE_CONFIG - allow conf-file to be specified on command line
 * NOTE: defining CMDLINE_CONFIG and installing ircd SUID or SGID is a MAJOR
 * security problem - they can use the "-f" option to read any files
 * that the 'new' access lets them.
 */
#define	CMDLINE_CONFIG

/* M4_PREPROC - run ircd.conf through m4 preprocessor
 * To use m4 as a preprocessor on the ircd.conf file, define M4_PREPROC.
 * The server will then call m4 each time it reads the ircd.conf file,
 * reading m4 output as the server's ircd.conf file.
 */
#undef	M4_PREPROC

/* USE_SYSLOG - log errors and such to syslog()
 * If you wish to have the server send 'vital' messages about server
 * through syslog, define USE_SYSLOG. Only system errors and events critical
 * to the server are logged although if this is defined with FNAME_USERLOG,
 * syslog() is used instead of the above file. It is not recommended that
 * this option is used unless you tell the system administrator beforehand
 * and obtain their permission to send messages to the system log files.
 *
 * It is stronglt recomended that you DO use syslog.  Many fatal ircd errors
 * are only logged to syslog.
 */
#define	USE_SYSLOG

#ifdef	USE_SYSLOG
/* SYSLOG_KILL SYSLOG_SQUIT SYSLOG_CONNECT SYSLOG_USERS SYSLOG_OPER
 * If you use syslog above, you may want to turn some (none) of the
 * spurious log messages for KILL,SQUIT,etc off.
 */
#undef	SYSLOG_KILL	/* log all operator kills to syslog */
#undef	SYSLOG_SQUIT	/* log all remote squits for all servers to syslog */
#undef	SYSLOG_CONNECT	/* log remote connect messages for other all servs */
#undef	SYSLOG_USERS	/* send userlog stuff to syslog */
#undef	SYSLOG_OPER	/* log all users who successfully become an Op */

/* LOG_FACILITY - facility to use for syslog()
 * Define the facility you want to use for syslog().  Ask your
 * sysadmin which one you should use.
 */
#define LOG_FACILITY LOG_LOCAL4

#endif /* USE_SYSLOG */

/* CRYPT_OPER_PASSWORD - use crypted oper passwords in the ircd.conf
 * define this if you want to use crypted passwords for operators in your
 * ircd.conf file.
 */
#define	CRYPT_OPER_PASSWORD

/* CRYPT_LINK_PASSWORD - use crypted N-line passwords in the ircd.conf
 * If you want to store encrypted passwords in N-lines for server links,
 * define this.  For a C/N pair in your ircd.conf file, the password
 * need not be the same for both, as long as hte opposite end has the
 * right password in the opposite line.
 */
#undef	CRYPT_LINK_PASSWORD

/* IDLE_FROM_MSG - Idle-time nullified only from privmsg
 * Idle-time nullified only from privmsg, if undefined idle-time
 * is nullified from everything except ping/pong.
 * Added 3.8.1992, kny@cs.hut.fi (nam)
 */
#define IDLE_FROM_MSG

/* MAXSENDQLENGTH - Max amount of internal send buffering
 * Max amount of internal send buffering when socket is stuck (bytes)
 */
#define MAXSENDQLENGTH 5050000    /* Recommended value: 5050000 for efnet */

/*  BUFFERPOOL - the maximum size of the total of all sendq's.
 *  Recommended value is four times MAXSENDQLENGTH.
 */
#define	BUFFERPOOL     (4 * MAXSENDQLENGTH)

/* IRC_UID IRC_GID - user and group id ircd should switch to if run as root
 * If you start the server as root but wish to have it run as another user,
 * define IRC_UID to that UID.  This should only be defined if you are running
 * as root and even then perhaps not.
 */
#define	IRC_UID 1001
#define	IRC_GID 31

/* CLIENT_FLOOD - client excess flood threshold
 * this controls the number of bytes the server will allow a client to
 * send to the server without processing before disconnecting the client for
 * flooding it.  Values greater than 8000 make no difference to the server.
 */
#define	CLIENT_FLOOD	2560

/* IRCII_KLUDGE - leave it defined
 * Define this if you want the server to accomplish ircII standard
 * Sends an extra NOTICE in the beginning of client connection
 */
#define	IRCII_KLUDGE

/*   STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP STOP  */

/* You shouldn't change anything below this line, unless absolutely needed. */

/* VIRTUAL_HOST - bind to a specific IP address
 * Define this if you'd like to run two or more servers on the same port
 * of one machine, using IP aliasing.  ircd will do look up the IP address
 * for the server name defined in the M: line and bind to that IP. 
 */
#undef VIRTUAL_HOST

/* INITIAL_DBUFS - how many dbufs to preallocate
 */
#define INITIAL_DBUFS 1000 /* preallocate 4 megs of dbufs */ 

/* MAXBUFFERS - increase socket buffers
 *
 * Increase send & receive socket buffer up to 64k,
 * keeps clients at 8K and only raises servers to 64K
 */
#define MAXBUFFERS

#ifdef	OPER_KILL
/* LOCAL_KILL_ONLY - restricts KILLs to local clients
 * To be used, OPER_KILL must be defined. LOCAL_KILL_ONLY restricts KILLs
 * to clients which are connected to the server the Operator is connected
 * to (ie lets them deal with local problem users or 'ghost' clients)
 *
 * NOTE: #define'ing this on an IRC net with servers which have a version
 *	 earlier than 2.7 is prohibited.  Such an action and subsequent use
 *	 of KILL for non-local clients should be punished by removal of the
 *	 server's links (if only for ignoring this warning!).
 */
#undef	LOCAL_KILL_ONLY
#endif

/* PORTNUM - default port where ircd resides
 * Port where ircd resides. NOTE: This *MUST* be greater than 1024 if you
 * plan to run ircd under any other uid than root.
 */
#define PORTNUM 6667

/* MAXCONNECTIONS - don't touch - change the HARD_FDLIMIT_ instead
 * Maximum number of network connections your server will allow.  This should
 * never exceed max. number of open file descrpitors and wont increase this.
 * Should remain LOW as possible. Most sites will usually have under 30 or so
 * connections. A busy hub or server may need this to be as high as 50 or 60.
 * Making it over 100 decreases any performance boost gained from it being low.
 * if you have a lot of server connections, it may be worth splitting the load
 * over 2 or more servers.
 * 1 server = 1 connection, 1 user = 1 connection.
 * This should be at *least* 3: 1 listen port, 1 dns port + 1 client
 */
/* change the HARD_FDLIMIT_ instead */
#define MAXCONNECTIONS	HARD_FDLIMIT

/* NICKNAMEHISTORYLENGTH - size of WHOWAS array
 * this defines the length of the nickname history.  each time a user changes
 * nickname or signs off, their old nickname is added to the top of the list.
 * NOTE: this is directly related to the amount of memory ircd will use whilst
 *	 resident and running - it hardly ever gets swapped to disk!  Memory
 *       will be preallocated for the entire whowas array when ircd is started.
 */
#define NICKNAMEHISTORYLENGTH 15000

/* TIMESEC - Time interval to wait and if no messages have been received,
 * then check for PINGFREQUENCY and CONNECTFREQUENCY 
 */
#define TIMESEC  5		/* Recommended value: 5 */

/* PINGFREQUENCY - ping frequency for idle connections
 * If daemon doesn't receive anything from any of its links within
 * PINGFREQUENCY seconds, then the server will attempt to check for
 * an active link with a PING message. If no reply is received within
 * (PINGFREQUENCY * 2) seconds, then the connection will be closed.
 */
#define PINGFREQUENCY    120	/* Recommended value: 120 */

/* CONNECTFREQUENCY - time to wait before auto-reconencting
 * If the connection to to uphost is down, then attempt to reconnect every 
 * CONNECTFREQUENCY  seconds.
 */
#define CONNECTFREQUENCY 600	/* Recommended value: 600 */

/* HANGONGOODLINK and HANGONGOODLINK
 * Often net breaks for a short time and it's useful to try to
 * establishing the same connection again faster than CONNECTFREQUENCY
 * would allow. But, to keep trying on bad connection, we require
 * that connection has been open for certain minimum time
 * (HANGONGOODLINK) and we give the net few seconds to steady
 * (HANGONRETRYDELAY). This latter has to be long enough that the
 * other end of the connection has time to notice it broke too.
 * 1997/09/18 recommended values by ThemBones for modern Efnet
 */

#define HANGONRETRYDELAY 60	/* Recommended value: 30-60 seconds */
#define HANGONGOODLINK 3600	/* Recommended value: 30-60 minutes */

/* WRITEWAITDELAY - Number of seconds to wait for write to
 * complete if stuck.
 */
#define WRITEWAITDELAY     10	/* Recommended value: 15 */

/* CONNECTTIMEOUT -
 * Number of seconds to wait for a connect(2) call to complete.
 * NOTE: this must be at *LEAST* 10.  When a client connects, it has
 * CONNECTTIMEOUT - 10 seconds for its host to respond to an ident lookup
 * query and for a DNS answer to be retrieved.
 */
#define	CONNECTTIMEOUT	30	/* Recommended value: 30 */

/* KILLCHASETIMELIMIT -
 * Max time from the nickname change that still causes KILL
 * automaticly to switch for the current nick of that user. (seconds)
 */
#define KILLCHASETIMELIMIT 90   /* Recommended value: 90 */

/* MAXCHANNELSPERUSER -
 * Max number of channels a user is allowed to join.
 */
#define MAXCHANNELSPERUSER  10	/* Recommended value: 10 */

/* SENDQ_ALWAYS - should always be defined.
 * SendQ-Always causes the server to put all outbound data into the sendq and
 * flushing the sendq at the end of input processing. This should cause more
 * efficient write's to be made to the network.
 * There *shouldn't* be any problems with this method.
 * -avalon
 */
#define	SENDQ_ALWAYS

/* -------------------------------------------------------------------------
   Shadowfax's anti flud code... Michael took great care to minimize
   the effect of this code on the server CPU wise. However, you may not
   wish to take the risk. It should certainly not be deffed for a HUB
   You may also have objections to the fact it "sneaks a peek" at PRIVMSG's
   sneaking a peek at the inside contents of messages is already done by
   bot catching code, and no privacy is breached..

   You choose.
   - Dianora
*/

/* FLUD - define this if you wish to enable server CTCP flood detection and
 * protection for your clients.  This works well against fludnets
 * and flood clones.  You may choose to tweak the following default
 * thresholds, but these seem to work well.  Not useful for routing
 * only servers. (Shadowfax)
 * 
 * The hybrid team strongly recommends you not use FLUD with HUB
 * if you do, you are on your own.
 */
#define FLUD


#ifdef HUB
#undef FLUD
#endif

#ifdef FLUD
/* FLUD_NUM FLUD_TIME FLUD_BLOCK - control behavior of FLUD protection
 */
#define FLUD_NUM	4	/* Number of flud messages to trip alarm */
#define FLUD_TIME	3	/* Seconds in which FLUD_NUM msgs must occur */
#define FLUD_BLOCK	15	/* Seconds to block fluds */
#endif


/* ----------------- archaic and/or broken secion -------------------- */
#undef DNS_DEBUG

/* SETUID_ROOT - plock - keep the ircd from being swapped out.
 * BSD swapping criteria do not match the requirements of ircd.
 * Note that the server needs to be setuid root for this to work.
 * The result of this is that the text segment of the ircd will be
 * locked in core; thus swapper cannot touch it and the behavior
 * noted above will not occur.  This probably doesn't work right
 * anymore.  IRCD_UID MUST be defined correctly if SETUID_ROOT.
 */
#undef SETUID_ROOT
 
/* SUN_GSO_BUG
 *
 * Recommended when ircd is run on SunOS host which hasn't got
 * kernel patch for getsockopt() bug applied. The patch is known
 * as Sun patch 100804-03 for SunOS 4.1.x.
 *
 * if you still have a machine with this bug, it doesn't belong on EFnet
 */
#undef SUN_GSO_BUG

/* CHROOTDIR - chroot() before reading conf
 * Define for value added security if you are paranoid.
 * All files you access must be in the directory you define as DPATH.
 * (This may effect the PATH locations above, though you can symlink it)
 *
 * You may want to define IRC_UID and IRC_GID
 */
#undef CHROOTDIR

/* DEBUGMODE is a bad bad bad evil thing that probably won't even compile
 * on this release.  Don't define it!
 */
#undef  DEBUGMODE               /* define DEBUGMODE to enable debugging mode.*/

/*
 * NOTE: On some systems, valloc() causes many problems.
 */
#undef  VALLOC                  /* Define this if you have valloc(3) */
 
/*
 * If your host supports varargs and has vsprintf(), vprintf() and vscanf()
 * C calls in its library, then you can define USE_VARARGS to use varargs
 * instead of imitation variable arg passing.
#undef USE_VARARGS
 
 * NOTE: with current server code, varargs doesn't survive because it can't
 *       be used in a chain of 3 or more funtions which all have a variable
 *       number of params.  If anyone has a solution to this, please notify
 *       the maintainer.
 */

/* ------------------------- END CONFIGURATION SECTION -------------------- */
#ifdef APOLLO
#define RESTARTING_SYSTEMCALLS
#endif                            /* read/write are restarted after signals
                                     defining this 1, gets siginterrupt call
                                     compiled, which attempts to remove this
                                     behaviour (apollo sr10.1/bsd4.3 needs
                                     this) */

#define HELPFILE HPATH
#define MOTD MPATH
#define	MYNAME SPATH
#define	CONFIGFILE CPATH
#ifdef KPATH
#define KLINEFILE  KPATH
#endif
#define	IRCD_PIDFILE PPATH

#define MAX_CLIENTS INIT_MAXCLIENTS

#if defined(CLIENT_FLOOD) && ((CLIENT_FLOOD > 8000) || (CLIENT_FLOOD < 512))
error CLIENT_FLOOD needs redefining.
#endif

#if !defined(CLIENT_FLOOD)
error CLIENT_FLOOD undefined.
#endif

#if defined(DEBUGMODE) || defined(DNS_DEBUG)
   extern void debug();
#  define Debug(x) debug x
#  define LOGFILE LPATH
#else
#  define Debug(x) ;
#  define LOGFILE "/dev/null"
#endif

#define CONFIG_H_LEVEL_5
#endif /* __config_include__ */