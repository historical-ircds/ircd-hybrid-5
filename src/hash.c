/************************************************************************
 *   IRC - Internet Relay Chat, src/hash.c
 *   Copyright (C) 1991 Darren Reed
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

#ifndef lint
static char sccsid[] = "@(#)hash.c	2.10 03 Jul 1993 (C) 1991 Darren Reed";

static char *rcs_version = "$Id$";
#endif

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "msg.h"
#include "hash.h"
#include "h.h"

static	aHashEntry	clientTable[U_MAX];
static	aHashEntry	channelTable[CH_MAX];

static void client_stats(aClient *,aClient*,int,char**);
static void channel_stats(aClient *,aClient*,int,char**);
/*

look in whowas.c for the missing ...[WW_MAX]; entry
  - Dianora
*/

/*
 * Hashing.
 *
 *   The server uses a chained hash table to provide quick and efficient
 * hash table mantainence (providing the hash function works evenly over
 * the input range).  The hash table is thus not susceptible to problems
 * of filling all the buckets or the need to rehash.
 *    It is expected that the hash table would look somehting like this
 * during use:
 *                   +-----+    +-----+    +-----+   +-----+
 *                ---| 224 |----| 225 |----| 226 |---| 227 |---
 *                   +-----+    +-----+    +-----+   +-----+
 *                      |          |          |
 *                   +-----+    +-----+    +-----+
 *                   |  A  |    |  C  |    |  D  |
 *                   +-----+    +-----+    +-----+
 *                      |
 *                   +-----+
 *                   |  B  |
 *                   +-----+
 *
 * A - GOPbot, B - chang, C - hanuaway, D - *.mu.OZ.AU
 *
 * The order shown above is just one instant of the server.  Each time a
 * lookup is made on an entry in the hash table and it is found, the entry
 * is moved to the top of the chain.
 *
 *    ^^^^^^^^^^^^^^^^ **** Not anymore - Dianora
 *
 */

unsigned hash_nick_name(nname)
     char *nname;
{
  Reg unsigned hash = 0;
  Reg hash2 = 0;
  Reg ret;
  Reg char lower;
  while (*nname)
    {
      lower = tolower(*nname);
      hash = (hash << 1) + lower;
      hash2 = (hash2 >> 1) + lower;
      nname++;
    }
  ret = ((hash & U_MAX_INITIAL_MASK) << BITS_PER_COL) +
    (hash2 & BITS_PER_COL_MASK);
  return ret;
}

/*
 * hash_channel_name
 *
 * calculate a hash value on at most the first 30 characters of the channel
 * name. Most names are short than this or dissimilar in this range. There
 * is little or no point hashing on a full channel name which maybe 255 chars
 * long.
 */
int	hash_channel_name(char *name)
{
  register unsigned char *hname = (unsigned char *)name;
  register unsigned int hash = 0;
  register int hash2 = 0;
  register char lower;
  register int i = 30;

  while(*hname && --i)
    {
      lower = tolower(*hname);
      hash = (hash << 1) + lower;
      hash2 = (hash2 >> 1) + lower;
      hname++;
    }
  return ((hash & CH_MAX_INITIAL_MASK) << BITS_PER_COL) +
    (hash2 & BITS_PER_COL_MASK);
}

unsigned int hash_whowas_name(char *name)
{
  register unsigned char *nname = (unsigned char *) name;
  register unsigned int hash = 0;
  register int hash2 = 0;
  register int ret;
  register char lower;
  
  while (*nname)
    {
      lower = tolower(*nname);
      hash = (hash << 1) + lower;
      hash2 = (hash2 >> 1) + lower;
      nname++;
    }
  ret = ((hash & WW_MAX_INITIAL_MASK) << BITS_PER_COL) +
    (hash2 & BITS_PER_COL_MASK);
  return ret;
}

/*
 * clear_*_hash_table
 *
 * Nullify the hashtable and its contents so it is completely empty.
 */
void	clear_client_hash_table()
{
  bzero((char *)clientTable, sizeof(aHashEntry) * U_MAX);
}

void	clear_channel_hash_table()
{
  bzero((char *)channelTable, sizeof(aHashEntry) * CH_MAX);
}

/*
 * add_to_client_hash_table
 */
int	add_to_client_hash_table(char *name, aClient *cptr)
{
  Reg	int	hashv;

  hashv = hash_nick_name(name);
  cptr->hnext = (aClient *)clientTable[hashv].list;
  clientTable[hashv].list = (void *)cptr;
  clientTable[hashv].links++;
  clientTable[hashv].hits++;
  return 0;
}

/*
 * add_to_channel_hash_table
 */
int	add_to_channel_hash_table(char *name,aChannel *chptr)
{
  Reg	int	hashv;

  hashv = hash_channel_name(name);
  chptr->hnextch = (aChannel *)channelTable[hashv].list;
  channelTable[hashv].list = (void *)chptr;
  channelTable[hashv].links++;
  channelTable[hashv].hits++;
  return 0;
}

/*
 * del_from_client_hash_table
 */
int	del_from_client_hash_table(char *name,aClient *cptr)
{
  Reg	aClient	*tmp, *prev = NULL;
  Reg	int	hashv;

  hashv = hash_nick_name(name);
  for (tmp = (aClient *)clientTable[hashv].list; tmp; tmp = tmp->hnext)
    {
      if (tmp == cptr)
	{
	  if (prev)
	    prev->hnext = tmp->hnext;
	  else
	    clientTable[hashv].list = (void *)tmp->hnext;
	  tmp->hnext = NULL;
	  if (clientTable[hashv].links > 0)
	    {
	      clientTable[hashv].links--;
	      return 1;
	    } 
	  else
	    /*
	     * Should never actually return from here and
	     * if we do it is an error/inconsistency in the
	     * hash table.
	     */
	    return -1;
	}
      prev = tmp;
    }
  return 0;
}

/*
 * del_from_channel_hash_table
 */
int	del_from_channel_hash_table(char *name, aChannel *chptr)
{
  Reg	aChannel	*tmp, *prev = NULL;
  Reg	int	hashv;

  hashv = hash_channel_name(name);
  for (tmp = (aChannel *)channelTable[hashv].list; tmp;
       tmp = tmp->hnextch)
    {
      if (tmp == chptr)
	{
	  if (prev)
	    prev->hnextch = tmp->hnextch;
	  else
	    channelTable[hashv].list=(void *)tmp->hnextch;
	  tmp->hnextch = NULL;
	  if (channelTable[hashv].links > 0)
	    {
	      channelTable[hashv].links--;
	      return 1;
	    }
	  else
	    return -1;
	}
      prev = tmp;
    }
  return 0;
}


/*
 * hash_find_client
 */
aClient	*hash_find_client(char *name, aClient *cptr)
{
  Reg	aClient	*tmp;
  Reg	aClient	*prv = NULL;
  Reg	aHashEntry	*tmp3;
  int	hashv;

  hashv = hash_nick_name(name);
  tmp3 = &clientTable[hashv];

  /*
   * Got the bucket, now search the chain.
   */
  for (tmp = (aClient *)tmp3->list; tmp; prv = tmp, tmp = tmp->hnext)
    if (mycmp(name, tmp->name) == 0)
      {
	return(tmp);
      }
  return (cptr);

  /*
   * If the member of the hashtable we found isnt at the top of its
   * chain, put it there.  This builds a most-frequently used order into
   * the chains of the hash table, giving speedier lookups on those nicks
   * which are being used currently.  This same block of code is also
   * used for channels and servers for the same performance reasons.
   *
   * I don't believe it does.. it only wastes CPU, lets try it and
   * see....
   *
   * - Dianora
   */
}

/*
 * hash_find_nickserver
 */
aClient	*hash_find_nickserver(char *name, aClient *cptr)
{
  Reg	aClient	*tmp;
  Reg	aClient	*prv = NULL;
  Reg	aHashEntry	*tmp3;
  int	hashv;
  char	*serv;
  
  serv = index(name, '@');
  *serv++ = '\0';
  hashv = hash_nick_name(name);
  tmp3 = &clientTable[hashv];

  /*
   * Got the bucket, now search the chain.
   */
  for (tmp = (aClient *)tmp3->list; tmp; prv = tmp, tmp = tmp->hnext)
    if (mycmp(name, tmp->name) == 0 && tmp->user &&
	mycmp(serv, tmp->user->server) == 0)
      {
	*--serv = '\0';
	return (tmp);
      }

  *--serv = '\0';
  return (cptr);
}

/*
 * hash_find_server
 */
aClient	*hash_find_server(char *server,aClient *cptr)
{
  Reg	aClient	*tmp, *prv = NULL;
  Reg	char	*t;
  Reg	char	ch;
  aHashEntry	*tmp3;

  int hashv;

  hashv = hash_nick_name(server);
  tmp3 = &clientTable[hashv];

  for (tmp = (aClient *)tmp3->list; tmp; prv = tmp, tmp = tmp->hnext)
    {
      if (!IsServer(tmp) && !IsMe(tmp))
	continue;
      if (mycmp(server, tmp->name) == 0)
	{
	  return(tmp);
	}
    }
  t = ((char *)server + strlen(server));
  /*
   * Whats happening in this next loop ? Well, it takes a name like
   * foo.bar.edu and proceeds to earch for *.edu and then *.bar.edu.
   * This is for checking full server names against masks although
   * it isnt often done this way in lieu of using matches().
   */
  for (;;)
    {
      t--;
      for (; t > server; t--)
	if (*(t+1) == '.')
	  break;
      if (*t == '*' || t == server)
	break;
      ch = *t;
      *t = '*';
      /*
       * Dont need to check IsServer() here since nicknames cant
       *have *'s in them anyway.
       */
      if (((tmp = hash_find_client(t, cptr))) != cptr)
	{
	  *t = ch;
	  return (tmp);
	}
      *t = ch;
    }
  return (cptr);
}

/*
 * hash_find_channel
 */
aChannel	*hash_find_channel(char *name,aChannel *chptr)
{
  int	hashv;
  Reg	aChannel	*tmp, *prv = NULL;
  aHashEntry	*tmp3;
  
  hashv = hash_channel_name(name);
  tmp3 = &channelTable[hashv];

  for (tmp = (aChannel *)tmp3->list; tmp; prv = tmp, tmp = tmp->hnextch)
    if (mycmp(name, tmp->chname) == 0)
      {
	return (tmp);
      }
  return chptr;
}

/*
 * NOTE: this command is not supposed to be an offical part of the ircd
 *       protocol.  It is simply here to help debug and to monitor the
 *       performance of the hash functions and table, enabling a better
 *       algorithm to be sought if this one becomes troublesome.
 *       -avalon
 *
 * partially rewritten (finally) -Dianora
 */

int	m_hash(aClient *cptr,aClient *sptr,int parc,char *parv[])
{
  char *command;

   if (!MyClient(sptr) || !IsOper(sptr))
    {
      sendto_one(sptr, err_str(ERR_NOPRIVILEGES), me.name, parv[0]);
      return 0;
    }

  if (parc > 1)
    {
      command = parv[1];
      if(!strcmp(command,"dhash"))
        dhash_stats(cptr,sptr,parc,parv);
      else if(!strcmp(command,"iphash"))
	iphash_stats(cptr,sptr,parc,parv);
      else if(!strcmp(command,"client"))
	client_stats(cptr,sptr,parc,parv);
      else if(!strcmp(command,"channel"))
	channel_stats(cptr,sptr,parc,parv);
    }
  else
    {
      sendto_one(sptr, ":%s NOTICE %s :hash [dhash|iphash|client|channel|dump]",
        me.name, parv[0]);
    }

  return 0;
}

/*
client_stats()

inputs		- 
output		-
side effects
*/

static void client_stats(aClient *cptr, aClient *sptr,int parc, char *parv[])
{
  int i;
  aHashEntry *p;
  aClient *client_ptr;
  int collision_count;

  sendto_one(sptr,":%s NOTICE %s :*** hash stats for client",
	     me.name,cptr->name);

  for(i = 0; i < U_MAX; i++)
    {
      p = &clientTable[i];

      collision_count = 0;
      if(p->list)
	{
	  client_ptr = (aClient *)p->list;
	  collision_count++;
	  while(client_ptr->hnext)
	    {
	      collision_count++;
	      client_ptr = client_ptr->hnext;
	    }
	}
      if(collision_count)
        sendto_one(sptr,":%s NOTICE %s :Entry %d (0x%X) Collisions %d",
		   me.name,cptr->name,i,i,collision_count);
    }
}

/*
channel_stats()

inputs		- 
output		-
side effects
*/

static void channel_stats(aClient *cptr, aClient *sptr,int parc, char *parv[])
{
  int i;
  aHashEntry *p;
  aChannel *channel_ptr;
  int collision_count;

  sendto_one(sptr,":%s NOTICE %s :*** hash stats for channel",
	     me.name,cptr->name);

  for(i = 0; i < CH_MAX; i++)
    {
      p = &channelTable[i];

      collision_count = 0;
      if(p->list)
	{
	  channel_ptr = (aChannel *)p->list;
	  collision_count++;
	  while(channel_ptr->hnextch)
	    {
	      collision_count++;
	      channel_ptr = channel_ptr->hnextch;
	    }
	}
      if(collision_count)
        sendto_one(sptr,":%s NOTICE %s :Entry %d (0x%X) Collisions %d",
		   me.name,cptr->name,i,i,collision_count);
    }
}

