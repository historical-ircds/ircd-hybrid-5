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

static	char	numbuff[512];

#ifdef CUSTOM_ERR
#include "s_err.h"
#else
static	Numeric	local_replies[] = {
/* 000 */	0, (char *)NULL,
/* 001 */	RPL_WELCOME, ":%s 001 %s :Welcome to the Internet Relay Network %s",
/* 002 */	RPL_YOURHOST, ":%s 002 %s :Your host is %s, running version %s",
/* 003 */	RPL_CREATED, ":%s 003 %s :This server was created %s",
/* 004 */	RPL_MYINFO, ":%s 004 %s %s %s oiwszcrkfydn biklmnopstv",
};


static	Numeric	numeric_replies[] = {
/* 200 */	RPL_TRACELINK, ":%s 200 %s Link %s%s %s %s",
/* 201 */	RPL_TRACECONNECTING, ":%s 201 %s Try. %d %s",
/* 202 */	RPL_TRACEHANDSHAKE, ":%s 202 %s H.S. %d %s",
/* 203 */	RPL_TRACEUNKNOWN, ":%s 203 %s ???? %d %s %d",
/* 204 */	RPL_TRACEOPERATOR, ":%s 204 %s Oper %d %s",
/* 205 */	RPL_TRACEUSER, ":%s 205 %s User %d %s 0 %d",
/* 206 */	RPL_TRACESERVER, ":%s 206 %s Serv %d %dS %dC %s %s!%s@%s",
/* 207 */	0, (char *)NULL,
/* 208 */	RPL_TRACENEWTYPE, ":%s 208 %s <newtype> 0 %s",
/* 209 */	RPL_TRACECLASS, ":%s 209 %s Class %d %d",
/* 210 */	0, (char *)NULL,
/* 211 */	RPL_STATSLINKINFO, (char *)NULL,
/* 212 */	RPL_STATSCOMMANDS, ":%s 212 %s %s %u %u",
/* 213 */	RPL_STATSCLINE, ":%s 213 %s %c %s * %s %d %d",
/* 214 */	RPL_STATSNLINE, ":%s 214 %s %c %s * %s %d %d",
/* 215 */	RPL_STATSILINE, ":%s 215 %s %c %s * %s %d %d",
#ifdef K_COMMENT_ONLY
/* 216 */	RPL_STATSKLINE, ":%s 216 %s %c %s * %s %s",
#else
/* 216 */	RPL_STATSKLINE, ":%s 216 %s %c %s %s %s %d %d",
#endif
/* 217 */	RPL_STATSQLINE, ":%s 217 %s %c %s * %s %d %d",
/* 218 */	RPL_STATSYLINE, ":%s 218 %s %c %d %d %d %d %ld",
/* 219 */	RPL_ENDOFSTATS, ":%s 219 %s %c :End of /STATS report",
/* 220 */	0, (char *)NULL,
/* 221 */	RPL_UMODEIS, ":%s 221 %s %s",
/* 222 */	RPL_STATSBLINE, ":%s 222 %s %c %s * %s %d %d",
/* 223 */	RPL_STATSELINE, ":%s 223 %s %c %s * %s %d %d",
/* 224 */	RPL_STATSFLINE, ":%s 224 %s %c %s * %s %d %d",
/* 225 */	RPL_STATSDLINE, ":%s 225 %s %c %s %s",
/* 226 */	0, (char *)NULL,
/* 227 */	0, (char *)NULL,
/* 228 */	0, (char *)NULL,
/* 229 */	0, (char *)NULL,
/* 230 */	0, (char *)NULL,
/* 231 */	0, (char *)NULL,
/* 232 */	0, (char *)NULL,
/* 233 */	0, (char *)NULL,
/* 234 */	RPL_SERVLIST, (char *)NULL,
/* 235 */	RPL_SERVLISTEND, (char *)NULL,
/* 236 */	0, (char *)NULL,
/* 237 */	0, (char *)NULL,
/* 238 */	0, (char *)NULL,
/* 239 */	0, (char *)NULL,
/* 240 */	0, (char *)NULL,
/* 241 */	RPL_STATSLLINE, ":%s 241 %s %c %s * %s %d %d",
/* 242 */	RPL_STATSUPTIME, ":%s 242 %s :Server Up %d days, %d:%02d:%02d",
/* 243 */	RPL_STATSOLINE, ":%s 243 %s %c %s * %s %d %d",
/* 244 */	RPL_STATSHLINE, ":%s 244 %s %c %s * %s %d %d", 
/* 245 */	RPL_STATSSLINE, ":%s 245 %s %c %s * %s %d %d", 
/* 246 */	0, (char *)NULL,
/* 247 */	0, (char *)NULL,
/* 248 */	0, (char *)NULL,
/* 249 */	0, (char *)NULL,
#ifdef HIGHEST_CONNECTION
/* 250 */       RPL_STATSCONN,
                    ":%s 250 %s :Highest connection count: %d (%d clients)",
#else
/* 250 */	0, (char *)NULL,
#endif
/* 251 */	RPL_LUSERCLIENT,
		":%s 251 %s :There are %d users and %d invisible on %d servers",
/* 252 */	RPL_LUSEROP, ":%s 252 %s %d :IRC Operators online",
/* 253 */	RPL_LUSERUNKNOWN, ":%s 253 %s %d :unknown connection(s)",
/* 254 */	RPL_LUSERCHANNELS, ":%s 254 %s %d :channels formed",
/* 255 */	RPL_LUSERME, ":%s 255 %s :I have %d clients and %d servers",
/* 256 */	RPL_ADMINME, ":%s 256 %s :Administrative info about %s",
/* 257 */	RPL_ADMINLOC1, ":%s 257 %s :%s",
/* 258 */	RPL_ADMINLOC2, ":%s 258 %s :%s",
/* 259 */	RPL_ADMINEMAIL, ":%s 259 %s :%s",
/* 260 */	0, (char *)NULL,
/* 261 */	RPL_TRACELOG, ":%s 261 %s File %s %d",
/* 262 */	RPL_ENDOFTRACE, ":%s 262 %s %s :End of TRACE",
/* 263 */       RPL_LOAD2HI, ":%s 263 %s :Server load is temporarily too heavy. Please wait a while and try again.",
/* 264 */	0, (char *)NULL,
/* 265 */	RPL_LOCALUSERS, ":%s 265 %s :Current local  users: %d  Max: %d",
/* 266 */	RPL_GLOBALUSERS, ":%s 266 %s :Current global users: %d  Max: %d",
/* 267 */	0, (char *)NULL,
/* 268 */	0, (char *)NULL,
/* 269 */	0, (char *)NULL,
/* 270 */	0, (char *)NULL,
/* 271 */	0, (char *)NULL,
/* 272 */	0, (char *)NULL,
/* 273 */	0, (char *)NULL,
/* 274 */	0, (char *)NULL,
/* 275 */	0, (char *)NULL,
/* 276 */	0, (char *)NULL,
/* 277 */	0, (char *)NULL,
/* 278 */	0, (char *)NULL,
/* 279 */	0, (char *)NULL,
/* 280 */	0, (char *)NULL,
/* 281 */	0, (char *)NULL,
/* 281 */	0, (char *)NULL,
/* 282 */	0, (char *)NULL,
/* 283 */	0, (char *)NULL,
/* 284 */	0, (char *)NULL,
/* 285 */	0, (char *)NULL,
/* 286 */	0, (char *)NULL,
/* 287 */	0, (char *)NULL,
/* 288 */	0, (char *)NULL,
/* 289 */	0, (char *)NULL,
/* 290 */	0, (char *)NULL,
/* 291 */	0, (char *)NULL,
/* 292 */	0, (char *)NULL,
/* 293 */	0, (char *)NULL,
/* 294 */	0, (char *)NULL,
/* 295 */	0, (char *)NULL,
/* 296 */	0, (char *)NULL,
/* 298 */	0, (char *)NULL,
/* 299 */	0, (char *)NULL,
/* 300 */	RPL_NONE, (char *)NULL,
/* 301 */	RPL_AWAY, ":%s 301 %s %s :%s",
/* 302 */	RPL_USERHOST, ":%s 302 %s :%s 302 %s :",
/* 303 */	RPL_ISON, ":%s 303 %s :",
/* 304 */	RPL_TEXT, (char *)NULL,
/* 305 */	RPL_UNAWAY, ":%s 305 %s :You are no longer marked as being away",
/* 306 */	RPL_NOWAWAY, ":%s 306 %s :You have been marked as being away",
/* 307 */	0, (char *)NULL,
/* 308 */	0, (char *)NULL,
/* 309 */	0, (char *)NULL,
/* 310 */	0, (char *)NULL,
/* 311 */	RPL_WHOISUSER, ":%s 311 %s %s %s %s * :%s",
/* 312 */	RPL_WHOISSERVER, ":%s 312 %s %s %s :%s",
/* 313 */	RPL_WHOISOPERATOR, ":%s 313 %s %s :is an IRC Operator",
/* 314 */	RPL_WHOWASUSER, ":%s 314 %s %s %s %s * :%s",
/* 315 */	RPL_ENDOFWHO, ":%s 315 %s %s :End of /WHO list.",
/* 316 */	RPL_WHOISCHANOP, (char *)NULL,
/* 317 */	RPL_WHOISIDLE, ":%s 317 %s %s %ld %ld :seconds idle, signon time",
/* 318 */	RPL_ENDOFWHOIS, ":%s 318 %s %s :End of /WHOIS list.",
/* 319 */	RPL_WHOISCHANNELS, ":%s 319 %s %s :%s",
/* 320 */	0, (char *)NULL,
/* 321 */	RPL_LISTSTART, ":%s 321 %s Channel :Users  Name",
/* 322 */	RPL_LIST, ":%s 322 %s %s %d :%s",
/* 323 */	RPL_LISTEND, ":%s 323 %s :End of /LIST",
/* 324 */       RPL_CHANNELMODEIS, ":%s 324 %s %s %s %s",
/* 325 */	0, (char *)NULL,
/* 326 */	0, (char *)NULL,
/* 327 */	0, (char *)NULL,
/* 328 */	0, (char *)NULL,
/* 329 */       RPL_CREATIONTIME, ":%s 329 %s %s %lu", 
/* 330 */	0, (char *)NULL,
/* 331 */	RPL_NOTOPIC, ":%s 331 %s %s :No topic is set.",
/* 332 */	RPL_TOPIC, ":%s 332 %s %s :%s",
#ifdef TOPIC_INFO
/* 333 */       RPL_TOPICWHOTIME, ":%s 333 %s %s %s %lu",
#else
/* 333 */       0, (char *)NULL,
#endif
/* 334 */	0, (char *)NULL,
/* 335 */	0, (char *)NULL,
/* 336 */	0, (char *)NULL,
/* 337 */	0, (char *)NULL,
/* 338 */	0, (char *)NULL,
/* 339 */	0, (char *)NULL,
/* 340 */	0, (char *)NULL,
/* 341 */	RPL_INVITING, ":%s 341 %s %s %s",
/* 342 */	RPL_SUMMONING, ":%s 342 %s %s :User summoned to irc",
/* 343 */	0, (char *)NULL,
/* 344 */	0, (char *)NULL,
/* 345 */	0, (char *)NULL,
/* 346 */	0, (char *)NULL,
/* 347 */	0, (char *)NULL,
/* 348 */	0, (char *)NULL,
/* 349 */	0, (char *)NULL,
/* 350 */	0, (char *)NULL,
/* 351 */	RPL_VERSION, ":%s 351 %s %s.%s %s :%s",
/* 352 */	RPL_WHOREPLY, ":%s 352 %s %s %s %s %s %s %s :%d %s",
/* 353 */	RPL_NAMREPLY, ":%s 353 %s %s",
/* 354 */	0, (char *)NULL,
/* 355 */	0, (char *)NULL,
/* 356 */	0, (char *)NULL,
/* 357 */	0, (char *)NULL,
/* 358 */	0, (char *)NULL,
/* 359 */	0, (char *)NULL,
/* 360 */	0, (char *)NULL,
/* 361 */	RPL_KILLDONE, (char *)NULL,
/* 362 */	RPL_CLOSING, ":%s 362 %s %s :Closed. Status = %d",
/* 363 */	RPL_CLOSEEND, ":%s 363 %s %d: Connections Closed",
/* 364 */	RPL_LINKS, ":%s 364 %s %s %s :%d %s",
/* 365 */	RPL_ENDOFLINKS, ":%s 365 %s %s :End of /LINKS list.",
/* 366 */	RPL_ENDOFNAMES, ":%s 366 %s %s :End of /NAMES list.",
#ifdef BAN_INFO
/* 367 */       RPL_BANLIST, ":%s 367 %s %s %s %s %lu",
#else
/* 367 */	RPL_BANLIST, ":%s 367 %s %s %s",
#endif
/* 368 */	RPL_ENDOFBANLIST, ":%s 368 %s %s :End of Channel Ban List",
/* 369 */	RPL_ENDOFWHOWAS, ":%s 369 %s %s :End of WHOWAS",
/* 370 */	0, (char *)NULL,
/* 371 */	RPL_INFO, ":%s 371 %s :%s",
/* 372 */	RPL_MOTD, ":%s 372 %s :- %s",
/* 373 */	RPL_INFOSTART, ":%s 373 %s :Server INFO",
/* 374 */	RPL_ENDOFINFO, ":%s 374 %s :End of /INFO list.",
/* 375 */	RPL_MOTDSTART, ":%s 375 %s :- %s Message of the Day - ",
/* 376 */	RPL_ENDOFMOTD, ":%s 376 %s :End of /MOTD command.",
/* 377 */	0, (char *)NULL,
/* 378 */	0, (char *)NULL,
/* 379 */	0, (char *)NULL,
/* 380 */	0, (char *)NULL,
/* 381 */	RPL_YOUREOPER, ":%s 381 %s :You have entered... the Twilight Zone!.",
/* 382 */	RPL_REHASHING, ":%s 382 %s %s :Rehashing",
/* 383 */	0, (char *)NULL,
/* 384 */	RPL_MYPORTIS, ":%s 384 %s %d :Port to local server is\r\n",
/* 385 */	RPL_NOTOPERANYMORE, (char *)NULL,
/* 386 */	0, (char *)NULL,
/* 387 */	0, (char *)NULL,
/* 388 */	0, (char *)NULL,
/* 389 */	0, (char *)NULL,
/* 390 */	0, (char *)NULL,
/* 391 */	RPL_TIME, ":%s 391 %s %s :%s",
#ifdef	ENABLE_USERS
/* 392 */	RPL_USERSSTART, ":%s 392 %s :UserID   Terminal  Host",
/* 393 */	RPL_USERS, ":%s 393 %s :%-8s %-9s %-8s",
/* 394 */	RPL_ENDOFUSERS, ":%s 394 %s :End of Users",
/* 395 */	RPL_NOUSERS, ":%s 395 %s :Nobody logged in.",
#else
/* 392 */	0, (char *)NULL,
/* 393 */	0, (char *)NULL,
/* 394 */	0, (char *)NULL,
/* 395 */	0, (char *)NULL,
#endif
/* 396 */	0, (char *)NULL,
/* 397 */	0, (char *)NULL,
/* 398 */	0, (char *)NULL,
/* 399 */	0, (char *)NULL
};

static	Numeric	numeric_errors[] = {
/* 400 */	0, (char *)NULL,
/* 401 */	ERR_NOSUCHNICK, ":%s 401 %s %s :No such nick/channel",
/* 402 */	ERR_NOSUCHSERVER, ":%s 402 %s %s :No such server",
/* 403 */	ERR_NOSUCHCHANNEL, ":%s 403 %s %s :No such channel",
/* 404 */	ERR_CANNOTSENDTOCHAN, ":%s 404 %s %s :Cannot send to channel",
/* 405 */	ERR_TOOMANYCHANNELS, ":%s 405 %s %s :You have joined too many channels",
/* 406 */	ERR_WASNOSUCHNICK, ":%s 406 %s %s :There was no such nickname",
/* 407 */	ERR_TOOMANYTARGETS,
		":%s 407 %s %s :Duplicate recipients. No message delivered",
/* 408 */	0, (char *)NULL,
/* 409 */	ERR_NOORIGIN, ":%s 409 %s :No origin specified",
/* 410 */	0, (char *)NULL,
/* 411 */	ERR_NORECIPIENT, ":%s 411 %s :No recipient given (%s)",
/* 412 */	ERR_NOTEXTTOSEND, ":%s 412 %s :No text to send",
/* 413 */	ERR_NOTOPLEVEL, ":%s 413 %s %s :No toplevel domain specified",
/* 414 */	ERR_WILDTOPLEVEL, ":%s 414 %s %s :Wildcard in toplevel Domain",
/* 415 */	0, (char *)NULL,
/* 416 */	0, (char *)NULL,
/* 417 */	0, (char *)NULL,
/* 418 */	0, (char *)NULL,
/* 419 */	0, (char *)NULL,
/* 420 */	0, (char *)NULL,
/* 421 */	ERR_UNKNOWNCOMMAND, ":%s 421 %s %s :Unknown command",
/* 422 */	ERR_NOMOTD, ":%s 422 %s :MOTD File is missing",
/* 423 */	ERR_NOADMININFO,
		":%s 423 %s %s :No administrative info available",
/* 424 */	ERR_FILEERROR, ":%s 424 %s :File error doing %s on %s",
/* 425 */	0, (char *)NULL,
/* 426 */	0, (char *)NULL,
/* 427 */	0, (char *)NULL,
/* 428 */	0, (char *)NULL,
/* 429 */	0, (char *)NULL,
/* 430 */	0, (char *)NULL,
/* 431 */	ERR_NONICKNAMEGIVEN, ":%s 431 %s :No nickname given",
/* 432 */	ERR_ERRONEUSNICKNAME, ":%s 432 %s %s :Erroneus Nickname",
/* 433 */	ERR_NICKNAMEINUSE, ":%s 433 %s %s :Nickname is already in use.",
/* 434 */	0, (char *)NULL,
/* 435 */	0, (char *)NULL,
/* 436 */       ERR_NICKCOLLISION, ":%s 436 %s %s :Nickname collision KILL",
/* 437 */	0, (char *)NULL,
/* 438 */	0, (char *)NULL,
/* 439 */	0, (char *)NULL,
/* 440 */	0, (char *)NULL,
/* 441 */	ERR_USERNOTINCHANNEL, ":%s 441 %s %s %s :They aren't on that channel",
/* 442 */	ERR_NOTONCHANNEL, ":%s 442 %s %s :You're not on that channel",
/* 443 */	ERR_USERONCHANNEL, ":%s 443 %s %s %s :is already on channel",
/* 444 */	ERR_NOLOGIN, ":%s 444 %s %s :User not logged in",
/* 445 */	ERR_SUMMONDISABLED, ":%s 445 %s :SUMMON has been removed",
/* 446 */	ERR_USERSDISABLED, ":%s 446 %s :USERS has been removed",
/* 447 */	0, (char *)NULL,
/* 448 */	0, (char *)NULL,
/* 449 */	0, (char *)NULL,
/* 450 */	0, (char *)NULL,
/* 451 */	ERR_NOTREGISTERED, ":%s 451 %s :You have not registered",
/* 452 */	0, (char *)NULL,
/* 453 */	0, (char *)NULL,
/* 454 */	0, (char *)NULL,
/* 455 */	0, (char *)NULL,
/* 456 */	0, (char *)NULL,
/* 457 */	0, (char *)NULL,
/* 458 */	0, (char *)NULL,
/* 459 */	0, (char *)NULL,
/* 460 */	0, (char *)NULL,
/* 461 */	ERR_NEEDMOREPARAMS, ":%s 461 %s %s :Not enough parameters",
/* 462 */	ERR_ALREADYREGISTRED, ":%s 462 %s :You may not reregister",
/* 463 */	ERR_NOPERMFORHOST, ":%s 463 %s :Your host isn't among the privileged",
/* 464 */	ERR_PASSWDMISMATCH, ":%s 464 %s :Password Incorrect",
/* 465 */	ERR_YOUREBANNEDCREEP, ":%s 465 %s :You are banned from this server- %s",
/* 466 */	ERR_YOUWILLBEBANNED, (char *)NULL,
/* 467 */	ERR_KEYSET, ":%s 467 %s %s :Channel key already set",
/* 468 */	0, (char *)NULL,
/* 469 */	0, (char *)NULL,
/* 470 */	0, (char *)NULL,
/* 471 */	ERR_CHANNELISFULL, ":%s 471 %s %s :Cannot join channel (+l)",
/* 472 */	ERR_UNKNOWNMODE  , ":%s 472 %s %c :is unknown mode char to me",
/* 473 */	ERR_INVITEONLYCHAN, ":%s 473 %s %s :Cannot join channel (+i)",
/* 474 */	ERR_BANNEDFROMCHAN, ":%s 474 %s %s :Cannot join channel (+b)",
/* 475 */	ERR_BADCHANNELKEY, ":%s 475 %s %s :Cannot join channel (+k)",
/* 476 */	ERR_BADCHANMASK, ":%s 476 %s %s :Bad Channel Mask",
/* 477 */	0, (char *)NULL,
/* 478 */	0, (char *)NULL,
/* 479 */	0, (char *)NULL,
/* 480 */	0, (char *)NULL,
/* 481 */	ERR_NOPRIVILEGES,
		":%s 481 %s :Permission Denied- You're not an IRC operator",
/* 482 */	ERR_CHANOPRIVSNEEDED, ":%s 482 %s %s :You're not channel operator",
/* 483 */	ERR_CANTKILLSERVER, ":%s 483 %s :You cant kill a server!",
/* 484 */	0, (char *)NULL,
/* 485 */	0, (char *)NULL,
/* 486 */	0, (char *)NULL,
/* 487 */	0, (char *)NULL,
/* 488 */	0, (char *)NULL,
/* 489 */	0, (char *)NULL,
/* 490 */	0, (char *)NULL,
/* 491 */	ERR_NOOPERHOST, ":%s 491 %s :Only few of mere mortals may try to enter the twilight zone",
/* 492 */	0, (char *)NULL,
/* 493 */	0, (char *)NULL,
/* 494 */	0, (char *)NULL,
/* 495 */	0, (char *)NULL,
/* 496 */	0, (char *)NULL,
/* 497 */	0, (char *)NULL,
/* 498 */	0, (char *)NULL,
/* 499 */	0, (char *)NULL,
/* 500 */	0, (char *)NULL,
/* 501 */	ERR_UMODEUNKNOWNFLAG, ":%s 501 %s :Unknown MODE flag",
/* 502 */	ERR_USERSDONTMATCH, ":%s 502 %s :Cant change mode for other users",
/* 503 */	ERR_GHOSTEDCLIENT, ":%s 503 %s :Message could not be delivered to %s",
/* 504 LAST */	ERR_LAST_ERR_MSG, ":%s 504 %s :Last Error Message",
		0, (char *)NULL
};
#endif /* CUSTOM_ERR */

char	*err_str(int numeric)
{
  Reg	Numeric	*nptr;
  Reg	int	num = numeric;

  if ( (num < 401) || (num > ERR_LAST_ERR_MSG))
    {
      (void)ircsprintf(numbuff,
		       ":%%s %d %%s :INTERNAL ERROR: BAD NUMERIC! %d",
		       numeric, numeric);
      return(numbuff);
    }

  num -= 400;
  nptr = &numeric_errors[num];
  if (!nptr->num_form || !nptr->num_val)
    {
      (void)ircsprintf(numbuff,
		       ":%%s %d %%s :NO ERROR FOR NUMERIC ERROR %d",
		       numeric, numeric);
      return(numbuff);
    }
  else
    return(nptr->num_form);
}


char	*rpl_str(int numeric)
{
  Reg	Numeric	*nptr;
  Reg	int	num = numeric;
  
  if(num < 5)
    {
      nptr = &local_replies[num];
      return(nptr->num_form);
    }
  
  if( (num < 200) || (num > 400) )
    {
      (void)ircsprintf(numbuff,
		       ":%%s %d %%s :INTERNAL REPLY ERROR: BAD NUMERIC! %d",
		       numeric, numeric);
      return(numbuff);
    }

  num -= 200;
  nptr = &numeric_replies[num];
  
  Debug((DEBUG_NUM, "rpl_str: numeric %d num %d nptr %x %d %x",
	 numeric, num, nptr, nptr->num_val, nptr->num_form));
  if (!nptr->num_form || !nptr->num_val)
    {
      (void)ircsprintf(numbuff,
		       ":%%s %d %%s :NO REPLY FOR NUMERIC ERROR %d",
		       numeric, numeric);
      return(numbuff);
    }
  else
    return(nptr->num_form);
}

