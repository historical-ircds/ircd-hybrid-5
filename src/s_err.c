/************************************************************************
 *   IRC - Internet Relay Chat, src/s_err.c
 *   Copyright (C) 1992 Darren Reed
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
 */

#include "struct.h"
#include "numeric.h"
#include "h.h"

#ifndef lint
static  char sccsid[] = "@(#)s_err.c	1.11 5/17/93 (C) 1992 Darren Reed";
static char *rcs_version = "$Id$";
#endif

typedef	struct	{
	int	num_val;
	char	*num_form;
} Numeric;

static	char	*prepbuf (char *, int, char *);
static	char	numbuff[512];
static	char	numbers[] = "0123456789";

#ifdef CUSTOM_ERR
#include "s_err.h"
#else
static	Numeric	local_replies[] = {
/* 000 */	0, (char *)NULL,
/* 001 */	RPL_WELCOME, ":Welcome to the Internet Relay Network %s",
/* 002 */	RPL_YOURHOST, ":Your host is %s, running version %s",
/* 003 */	RPL_CREATED, ":This server was created %s",
/* 004 */	RPL_MYINFO, "%s %s oiwszcrkfydn biklmnopstv",
		0, (char *)NULL
};

static	Numeric	numeric_errors[] = {
/* 401 */	ERR_NOSUCHNICK, "%s :No such nick/channel",
/* 402 */	ERR_NOSUCHSERVER, "%s :No such server",
/* 403 */	ERR_NOSUCHCHANNEL, "%s :No such channel",
/* 404 */	ERR_CANNOTSENDTOCHAN, "%s :Cannot send to channel",
/* 405 */	ERR_TOOMANYCHANNELS, "%s :You have joined too many channels",
/* 406 */	ERR_WASNOSUCHNICK, "%s :There was no such nickname",
/* 407 */	ERR_TOOMANYTARGETS,
		"%s :Duplicate recipients. No message delivered",
/* 408 */	0, (char *)NULL,
/* 409 */	ERR_NOORIGIN, ":No origin specified",
/* 410 */	0, (char *)NULL,
/* 411 */	ERR_NORECIPIENT, ":No recipient given (%s)",
/* 412 */	ERR_NOTEXTTOSEND, ":No text to send",
/* 413 */	ERR_NOTOPLEVEL, "%s :No toplevel domain specified",
/* 414 */	ERR_WILDTOPLEVEL, "%s :Wildcard in toplevel Domain",
/* 415 */	0, (char *)NULL,
/* 416 */	0, (char *)NULL,
/* 417 */	0, (char *)NULL,
/* 418 */	0, (char *)NULL,
/* 419 */	0, (char *)NULL,
/* 420 */	0, (char *)NULL,
/* 421 */	ERR_UNKNOWNCOMMAND, "%s :Unknown command",
/* 422 */	ERR_NOMOTD, ":MOTD File is missing",
/* 423 */	ERR_NOADMININFO,
		"%s :No administrative info available",
/* 424 */	ERR_FILEERROR, ":File error doing %s on %s",
/* 425 */	0, (char *)NULL,
/* 426 */	0, (char *)NULL,
/* 427 */	0, (char *)NULL,
/* 428 */	0, (char *)NULL,
/* 429 */	0, (char *)NULL,
/* 430 */	0, (char *)NULL,
/* 431 */	ERR_NONICKNAMEGIVEN, ":No nickname given",
/* 432 */	ERR_ERRONEUSNICKNAME, "%s :Erroneus Nickname",
/* 433 */	ERR_NICKNAMEINUSE, "%s :Nickname is already in use.",
/* 434 */	0, (char *)NULL,
/* 435 */	0, (char *)NULL,
/* 436 */       ERR_NICKCOLLISION, "%s :Nickname collision KILL",
/* 437 */	0, (char *)NULL,
/* 438 */	0, (char *)NULL,
/* 439 */	0, (char *)NULL,
/* 440 */	0, (char *)NULL,
/* 441 */	ERR_USERNOTINCHANNEL, "%s %s :They aren't on that channel",
/* 442 */	ERR_NOTONCHANNEL, "%s :You're not on that channel",
/* 443 */	ERR_USERONCHANNEL, "%s %s :is already on channel",
/* 444 */	ERR_NOLOGIN, "%s :User not logged in",
/* 445 */	ERR_SUMMONDISABLED, ":SUMMON has been removed",
/* 446 */	ERR_USERSDISABLED, ":USERS has been removed",
/* 447 */	0, (char *)NULL,
/* 448 */	0, (char *)NULL,
/* 449 */	0, (char *)NULL,
/* 450 */	0, (char *)NULL,
/* 451 */	ERR_NOTREGISTERED, ":You have not registered",
/* 452 */	0, (char *)NULL,
/* 453 */	0, (char *)NULL,
/* 454 */	0, (char *)NULL,
/* 455 */	0, (char *)NULL,
/* 456 */	0, (char *)NULL,
/* 457 */	0, (char *)NULL,
/* 458 */	0, (char *)NULL,
/* 459 */	0, (char *)NULL,
/* 460 */	0, (char *)NULL,
/* 461 */	ERR_NEEDMOREPARAMS, "%s :Not enough parameters",
/* 462 */	ERR_ALREADYREGISTRED, ":You may not reregister",
/* 463 */	ERR_NOPERMFORHOST, ":Your host isn't among the privileged",
/* 464 */	ERR_PASSWDMISMATCH, ":Password Incorrect",
/* 465 */	ERR_YOUREBANNEDCREEP, ":You are banned from this server- %s",
/* 466 */	ERR_YOUWILLBEBANNED, (char *)NULL,
/* 467 */	ERR_KEYSET, "%s :Channel key already set",
/* 468 */	0, (char *)NULL,
/* 469 */	0, (char *)NULL,
/* 470 */	0, (char *)NULL,
/* 471 */	ERR_CHANNELISFULL, "%s :Cannot join channel (+l)",
/* 472 */	ERR_UNKNOWNMODE  , "%c :is unknown mode char to me",
/* 473 */	ERR_INVITEONLYCHAN, "%s :Cannot join channel (+i)",
/* 474 */	ERR_BANNEDFROMCHAN, "%s :Cannot join channel (+b)",
/* 475 */	ERR_BADCHANNELKEY, "%s :Cannot join channel (+k)",
/* 476 */	ERR_BADCHANMASK, "%s :Bad Channel Mask",
/* 477 */	0, (char *)NULL,
/* 478 */	0, (char *)NULL,
/* 479 */	0, (char *)NULL,
/* 480 */	0, (char *)NULL,
/* 481 */	ERR_NOPRIVILEGES,
		":Permission Denied- You're not an IRC operator",
/* 482 */	ERR_CHANOPRIVSNEEDED, "%s :You're not channel operator",
/* 483 */	ERR_CANTKILLSERVER, ":You cant kill a server!",
/* 484 */	0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
/* 491 */	ERR_NOOPERHOST, ":Only few of mere mortals may try to enter the twilight zone",
/* 492 */	0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
/* 501 */	ERR_UMODEUNKNOWNFLAG, ":Unknown MODE flag",
/* 502 */	ERR_USERSDONTMATCH, ":Cant change mode for other users",
/* 503 */	ERR_GHOSTEDCLIENT, ":Message could not be delivered to %s",
/* LAST */	ERR_LAST_ERR_MSG, ":Last Error Message",
		0, (char *)NULL
};

static	Numeric	numeric_replies[] = {
/* 300 */	RPL_NONE, (char *)NULL,
/* 301 */	RPL_AWAY, "%s :%s",
/* 302 */	RPL_USERHOST, ":",
/* 303 */	RPL_ISON, ":",
/* 304 */	RPL_TEXT, (char *)NULL,
/* 305 */	RPL_UNAWAY, ":You are no longer marked as being away",
/* 306 */	RPL_NOWAWAY, ":You have been marked as being away",
/* 307 */	0, (char *)NULL,
/* 308 */	0, (char *)NULL,
/* 309 */	0, (char *)NULL,
/* 310 */	0, (char *)NULL,
/* 311 */	RPL_WHOISUSER, "%s %s %s * :%s",
/* 312 */	RPL_WHOISSERVER, "%s %s :%s",
/* 313 */	RPL_WHOISOPERATOR, "%s :is an IRC Operator",
/* 314 */	RPL_WHOWASUSER, "%s %s %s * :%s",
/* 315 */	RPL_ENDOFWHO, "%s :End of /WHO list.",
/* 316 */	RPL_WHOISCHANOP, (char *)NULL,
/* 317 */	RPL_WHOISIDLE, "%s %ld %ld :seconds idle, signon time",
/* 318 */	RPL_ENDOFWHOIS, "%s :End of /WHOIS list.",
/* 319 */	RPL_WHOISCHANNELS, "%s :%s",
		0, (char *)NULL,
/* 321 */	RPL_LISTSTART, "Channel :Users  Name",
/* 322 */	RPL_LIST, "%s %d :%s",
/* 323 */	RPL_LISTEND, ":End of /LIST",
/* 324 */       RPL_CHANNELMODEIS, "%s %s %s",
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
/* 329 */       RPL_CREATIONTIME, "%s %lu", 
		0, (char *)NULL,
/* 331 */	RPL_NOTOPIC, "%s :No topic is set.",
/* 332 */	RPL_TOPIC, "%s :%s",
#ifdef TOPIC_INFO
/* 333 */       RPL_TOPICWHOTIME, "%s %s %lu",
#else
/* 333 */       0, (char *)NULL,
#endif
/* 334 */	0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
		0, (char *)NULL,
/* 341 */	RPL_INVITING, "%s %s",
/* 342 */	RPL_SUMMONING, "%s :User summoned to irc",
		0, (char *)NULL, 0, (char *)NULL,
		0, (char *)NULL, 0, (char *)NULL, 0, (char *)NULL,
		0, (char *)NULL, 0, (char *)NULL, 0, (char *)NULL,
/* 351 */	RPL_VERSION, "%s.%s %s :%s",
/* 352 */	RPL_WHOREPLY, "%s %s %s %s %s %s :%d %s",
/* 353 */	RPL_NAMREPLY, "%s",
		0, (char *)NULL, 0, (char *)NULL, 0, (char *)NULL,
		0, (char *)NULL, 0, (char *)NULL, 0, (char *)NULL,
		0, (char *)NULL,
/* 361 */	RPL_KILLDONE, (char *)NULL,
/* 362 */	RPL_CLOSING, "%s :Closed. Status = %d",
/* 363 */	RPL_CLOSEEND, "%d: Connections Closed",
/* 364 */	RPL_LINKS, "%s %s :%d %s",
/* 365 */	RPL_ENDOFLINKS, "%s :End of /LINKS list.",
/* 366 */	RPL_ENDOFNAMES, "%s :End of /NAMES list.",
#ifdef BAN_INFO
/* 367 */       RPL_BANLIST, "%s %s %s %lu",
#else
/* 367 */	RPL_BANLIST, "%s %s",
#endif
/* 368 */	RPL_ENDOFBANLIST, "%s :End of Channel Ban List",
/* 369 */	RPL_ENDOFWHOWAS, "%s :End of WHOWAS",
		0, (char *)NULL,
/* 371 */	RPL_INFO, ":%s",
/* 372 */	RPL_MOTD, ":- %s",
/* 373 */	RPL_INFOSTART, ":Server INFO",
/* 374 */	RPL_ENDOFINFO, ":End of /INFO list.",
/* 375 */	RPL_MOTDSTART, ":- %s Message of the Day - ",
/* 376 */	RPL_ENDOFMOTD, ":End of /MOTD command.",
		0, (char *)NULL,
		0, (char *)NULL, 0, (char *)NULL,
		0, (char *)NULL,
/* 381 */	RPL_YOUREOPER, ":You have entered... the Twilight Zone!.",
/* 382 */	RPL_REHASHING, "%s :Rehashing",
/* 383 */	0, (char *)NULL,
/* 384 */	RPL_MYPORTIS, "%d :Port to local server is\r\n",
/* 385 */	RPL_NOTOPERANYMORE, (char *)NULL,
		0, (char *)NULL, 0, (char *)NULL, 0, (char *)NULL,
		0, (char *)NULL, 0, (char *)NULL,
/* 391 */	RPL_TIME, "%s :%s",
#ifdef	ENABLE_USERS
/* 392 */	RPL_USERSSTART, ":UserID   Terminal  Host",
/* 393 */	RPL_USERS, ":%-8s %-9s %-8s",
/* 394 */	RPL_ENDOFUSERS, ":End of Users",
/* 395 */	RPL_NOUSERS, ":Nobody logged in.",
#else
		0, (char *)NULL, 0, (char *)NULL, 0, (char *)NULL,
		0, (char *)NULL,
#endif
		0, (char *)NULL, 0, (char *)NULL, 0, (char *)NULL,
		0, (char *)NULL,
/* 200 */	RPL_TRACELINK, "Link %s%s %s %s",
/* 201 */	RPL_TRACECONNECTING, "Try. %d %s",
/* 202 */	RPL_TRACEHANDSHAKE, "H.S. %d %s",
/* 203 */	RPL_TRACEUNKNOWN, "???? %d %s %d",
/* 204 */	RPL_TRACEOPERATOR, "Oper %d %s",
/* 205 */	RPL_TRACEUSER, "User %d %s",
/* 206 */	RPL_TRACESERVER, "Serv %d %dS %dC %s %s!%s@%s",
/* 207 */	0, (char *)NULL,
/* 208 */	RPL_TRACENEWTYPE, "<newtype> 0 %s",
/* 209 */	RPL_TRACECLASS, "Class %d %d",
		0, (char *)NULL,
/* 211 */	RPL_STATSLINKINFO, (char *)NULL,
/* 212 */	RPL_STATSCOMMANDS, "%s %u %u",
/* 213 */	RPL_STATSCLINE, "%c %s * %s %d %d",
/* 214 */	RPL_STATSNLINE, "%c %s * %s %d %d",
/* 215 */	RPL_STATSILINE, "%c %s * %s %d %d",
#ifdef K_COMMENT_ONLY
/* 216 */	RPL_STATSKLINE, "%c %s * %s %s",
#else
/* 216 */	RPL_STATSKLINE, "%c %s %s %s %d %d",
#endif
/* 217 */	RPL_STATSQLINE, "%c %s * %s %d %d",
/* 218 */	RPL_STATSYLINE, "%c %d %d %d %d %ld",
/* 219 */	RPL_ENDOFSTATS, "%c :End of /STATS report",
		0, (char *)NULL,
/* 221 */	RPL_UMODEIS, "%s",
/* 222 */	RPL_STATSBLINE, "%c %s * %s %d %d",
/* 223 */	RPL_STATSELINE, "%c %s * %s %d %d",
/* 224 */	RPL_STATSFLINE, "%c %s * %s %d %d",
/* 225 */	RPL_STATSDLINE, "%c %s %s",
		0, (char *)NULL, 0, (char *)NULL,
		0, (char *)NULL, 0, (char *)NULL, 0, (char *)NULL,
/* 231 */	0, (char *)NULL,
/* 232 */	0, (char *)NULL,
/* 233 */	0, (char *)NULL,
/* 234 */	RPL_SERVLIST, (char *)NULL,
/* 235 */	RPL_SERVLISTEND, (char *)NULL,
		0, (char *)NULL, 0, (char *)NULL, 0, (char *)NULL,
		0, (char *)NULL, 0, (char *)NULL,
/* 241 */	RPL_STATSLLINE, "%c %s * %s %d %d",
/* 242 */	RPL_STATSUPTIME, ":Server Up %d days, %d:%02d:%02d",
/* 243 */	RPL_STATSOLINE, "%c %s * %s %d %d",
/* 244 */	RPL_STATSHLINE, "%c %s * %s %d %d", 
/* 245 */	RPL_STATSSLINE, "%c %s * %s %d %d", 
		0, (char *)NULL, 0, (char *)NULL, 0, (char *)NULL,
		0, (char *)NULL,
#ifdef HIGHEST_CONNECTION
/* 250 */       RPL_STATSCONN,
                    ":Highest connection count: %d (%d clients)",
#else
                0, (char *)NULL,
#endif
/* 251 */	RPL_LUSERCLIENT,
		":There are %d users and %d invisible on %d servers",
/* 252 */	RPL_LUSEROP, "%d :IRC Operators online",
/* 253 */	RPL_LUSERUNKNOWN, "%d :unknown connection(s)",
/* 254 */	RPL_LUSERCHANNELS, "%d :channels formed",
/* 255 */	RPL_LUSERME, ":I have %d clients and %d servers",
/* 256 */	RPL_ADMINME, ":Administrative info about %s",
/* 257 */	RPL_ADMINLOC1, ":%s",
/* 258 */	RPL_ADMINLOC2, ":%s",
/* 259 */	RPL_ADMINEMAIL, ":%s",
		0, (char *)NULL,
/* 261 */	RPL_TRACELOG, "File %s %d",
/* 262 */	RPL_ENDOFTRACE, "End of TRACE",
/* 263 */       RPL_LOAD2HI, ":Server load is temporarily too heavy. Please wait a while and try again.",
		0, (char *)NULL,
/* 265 */	RPL_LOCALUSERS, ":Current local  users: %d  Max: %d",
/* 266 */	RPL_GLOBALUSERS, ":Current global users: %d  Max: %d"
		};
#endif /* CUSTOM_ERR */

char	*err_str(int numeric)
{
  Reg	Numeric	*nptr;
  Reg	int	num = numeric;

  num -= numeric_errors[0].num_val;
  if (num < 0 || num > ERR_LAST_ERR_MSG)
    (void)ircsprintf(numbuff,
		     ":%%s %d %%s :INTERNAL ERROR: BAD NUMERIC! %d",
		     numeric, num);
  else
    {
      nptr = &numeric_errors[num];
      if (!nptr->num_form || !nptr->num_val)
	(void)ircsprintf(numbuff,
			 ":%%s %d %%s :NO ERROR FOR NUMERIC ERROR %d",
			 numeric, num);
      else
	(void)prepbuf(numbuff, nptr->num_val, nptr->num_form);
    }
  return numbuff;
}


char	*rpl_str(int numeric)
{
  Reg	Numeric	*nptr;
  Reg	int	num = numeric;
  
  if (num > 4)
    num -= (num > 300) ? 300 : 100;
  
  if (num < 0 || num > 200)
    (void)ircsprintf(numbuff,
		     ":%%s %d %%s :INTERNAL REPLY ERROR: BAD NUMERIC! %d",
		     numeric, num);
  else
    {
      if (numeric > 99)
	nptr = &numeric_replies[num];
      else
	nptr = &local_replies[num];
      Debug((DEBUG_NUM, "rpl_str: numeric %d num %d nptr %x %d %x",
	     numeric, num, nptr, nptr->num_val, nptr->num_form));
      if (!nptr->num_form || !nptr->num_val)
	(void)ircsprintf(numbuff,
			 ":%%s %d %%s :NO REPLY FOR NUMERIC ERROR %d",
			 numeric, num);
      else
	(void)prepbuf(numbuff, nptr->num_val, nptr->num_form);
    }
  return numbuff;
}

static	char	*prepbuf(char *buffer,
			 int num,
			 char *tail)
{
  Reg	char	*s;

  (void)strcpy(buffer, ":%s ");
  s = buffer + 4;

  *s++ = numbers[num/100];
  num %= 100;
  *s++ = numbers[num/10];
  *s++ = numbers[num%10];
  (void)strcpy(s, " %s ");
  (void)strcpy(s+4, tail);
  return buffer;
}
