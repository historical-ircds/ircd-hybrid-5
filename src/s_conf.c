/************************************************************************
 *   IRC - Internet Relay Chat, src/s_conf.c
 *   Copyright (C) 1990 Jarkko Oikarinen and
 *                      University of Oulu, Computing Center
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
static  char sccsid[] = "@(#)s_conf.c	2.56 02 Apr 1994 (C) 1988 University of Oulu, \
Computing Center and Jarkko Oikarinen";

static char *rcs_version = "$Id$";
#endif

#include "struct.h"
#include "common.h"
#include "sys.h"
#include "numeric.h"
#include "inet.h"
#include <sys/socket.h>
#include <fcntl.h>
#include <sys/wait.h>
#ifdef __hpux
#include "inet.h"
#endif
#if defined(AIX) || defined(DYNIXPTX) || defined(SVR3)
#include <time.h>
#endif
#ifdef	R_LINES
#include <signal.h>
#endif

#include <signal.h>
#include "h.h"
extern int rehashed;
#include "dich_conf.h"

#ifdef	TIMED_KLINES
static	int	check_time_interval(char *);
#endif

struct sockaddr_in vserv;
char	specific_virtual_host;

/* internally defined functions */

static	int	lookup_confhost (aConfItem *);
static  int     attach_iline(aClient *, aConfItem *,char *);
static  aConfItem *temporary_klines = (aConfItem *)NULL;

/* externally defined functions */
extern  void    outofmemory(void);	/* defined in list.c */

/* usually, with hash tables, you use a prime number...
   but in this case I am dealing with ip addresses, not ascii strings.
*/

#define IP_HASH_SIZE 0x1000

typedef struct ip_entry
{
  unsigned long ip;
  int	count;
  struct ip_entry *next;
}IP_ENTRY;

IP_ENTRY *ip_hash_table[IP_HASH_SIZE];

void init_ip_hash(void);
static int hash_ip(unsigned long);
static IP_ENTRY *find_or_add_ip(unsigned long);

/* externally defined routines */
int find_conf_match(aClient *,aConfList *,aConfList *,aConfList *);
int find_fline(aClient *);
extern	void  delist_conf(aConfItem *);

#ifdef GLINES
static  aConfItem *glines = (aConfItem *)NULL;
extern void expire_pending_glines();	/* defined in s_serv.c */
#endif

aConfItem	*conf = ((aConfItem *)NULL);

#ifdef LOCKFILE 
extern void do_pending_klines(void);
#endif

/*
 * remove all conf entries from the client except those which match
 * the status field mask.
 */
void	det_confs_butmask(aClient *cptr,int mask)
{
  Reg Link *tmp, *tmp2;

  for (tmp = cptr->confs; tmp; tmp = tmp2)
    {
      tmp2 = tmp->next;
      if ((tmp->value.aconf->status & mask) == 0)
	(void)detach_conf(cptr, tmp->value.aconf);
    }
}

/*
 * find the first (best) I line to attach.
 */
/*
  cleanup aug 3 1997 - Dianora
 */

int	attach_Iline(aClient *cptr,
		     struct hostent *hp,
		     char *sockhost)
{
  Reg	aConfItem	*aconf;
  Reg	char	*hname;
  Reg	int	i;
  static	char	uhost[HOSTLEN+USERLEN+3];
  static	char	uhost2[HOSTLEN+USERLEN+3];
  static	char	fullname[HOSTLEN+1];

  for (aconf = conf; aconf; aconf = aconf->next)
    {
      if (aconf->status != CONF_CLIENT)
	continue;

      if (aconf->port && aconf->port != cptr->acpt->port)
	continue;

      if (!aconf->host || !aconf->name)
	return(attach_iline(cptr,aconf,uhost));

      if (hp)
	for (i = 0, hname = hp->h_name; hname;
	     hname = hp->h_aliases[i++])
	  {
	    (void)strncpy(fullname, hname,
			  sizeof(fullname)-1);
	    add_local_domain(fullname,
			     HOSTLEN - strlen(fullname));
	    Debug((DEBUG_DNS, "a_il: %s->%s",
		   sockhost, fullname));
	    if (index(aconf->name, '@'))
	      {
		(void)strcpy(uhost, cptr->username);
		(void)strcat(uhost, "@");
		(void)strcpy(uhost2, cptr->username);
		(void)strcat(uhost2, "@");
	      }
	    else
	      {
		*uhost = '\0';
		*uhost2 = '\0';
	      }
	    (void)strncat(uhost, fullname,
			  sizeof(uhost) - strlen(uhost));
	    (void)strncat(uhost2, sockhost,
			  sizeof(uhost2) - strlen(uhost2));
	    if ((!match(aconf->name, uhost)) ||
		(!match(aconf->name, uhost2)))
	      return(attach_iline(cptr,aconf,uhost));
	  }

      if (index(aconf->host, '@'))
	{	
	  /* strncpyzt(uhost, username, sizeof(uhost));
	     username is limited in length -Sol */
	  strncpyzt(uhost, cptr->username, USERLEN+1);
	  (void)strcat(uhost, "@");
	}
      else
	*uhost = '\0';
      (void)strncat(uhost, sockhost, sizeof(uhost) - strlen(uhost));

      if (match(aconf->host, uhost) == 0)
	return(attach_iline(cptr,aconf,uhost));
    }

  return -1;	/* -1 on no match *bleh* */
}

/*
    rewrote to remove the "ONE" lamity *BLEH* I agree with comstud
    on this one. 
  - Dianora
 */

static int attach_iline(
		 aClient *cptr,
		 aConfItem *aconf,
		 char *uhost)
{
  IP_ENTRY *ip_found;

  if (index(uhost, '@'))
    cptr->flags |= FLAGS_DOID;
  get_sockhost(cptr, uhost);

/* every conf when created, has a class pointer set up. 
   if it isn't, well.  *BOOM* ! */

  ip_found = find_or_add_ip(cptr->ip.s_addr);
  cptr->flags |= FLAGS_IPHASH;
  ip_found->count++;

  /* only check it if its non zero */
  if ((aconf->class->conFreq) && (ip_found->count > aconf->class->conFreq))
     return -4; /* Already at maximum allowed ip#'s */

  return ( attach_conf(cptr, aconf) );
}

/* link list of free IP_ENTRY's */

static IP_ENTRY *free_ip_entries;

/*
init_ip_hash()

input		- NONE
output		- NONE
side effects	- clear the ip hash table

stole the link list pre-allocator from list.c
*/

void clear_ip_hash_table()
{
  void *block_IP_ENTRIES;	/* block of IP_ENTRY's */
  IP_ENTRY *new_IP_ENTRY;	/* new IP_ENTRY being made */
  IP_ENTRY *last_IP_ENTRY;	/* last IP_ENTRY in chain */
  int size;
  int n_left_to_allocate = MAXCONNECTIONS;

  /* ok. if the sizeof the struct isn't aligned with that of the
     smallest guaranteed valid pointer (void *), then align it
     ya. you could just turn 'size' into a #define. do it. :-)

     -Dianora
     */

  size = sizeof(IP_ENTRY) + (sizeof(IP_ENTRY) & (sizeof(void*) - 1) );

  block_IP_ENTRIES = (void *)MyMalloc((size * n_left_to_allocate));  

  free_ip_entries = (IP_ENTRY *)block_IP_ENTRIES;
  last_IP_ENTRY = free_ip_entries;

  /* *shudder* pointer arithmetic */
  while(--n_left_to_allocate)
    {
      block_IP_ENTRIES = (void *)((unsigned long)block_IP_ENTRIES + 
			(unsigned long) size);
      new_IP_ENTRY = (IP_ENTRY *)block_IP_ENTRIES;
      last_IP_ENTRY->next = new_IP_ENTRY;
      new_IP_ENTRY->next = (IP_ENTRY *)NULL;
      last_IP_ENTRY = new_IP_ENTRY;
    }
  bzero((char *)ip_hash_table, sizeof(ip_hash_table));
}

/* 
find_or_add_ip()

inputs		- unsigned long IP address value
output		- pointer to an IP_ENTRY element
side effects	-

If the ip # was not found, a new IP_ENTRY is created, and the ip
count set to 0.
*/

static IP_ENTRY *find_or_add_ip(unsigned long ip_in)
{
  int hash_index;
  IP_ENTRY *ptr, *newptr;

  newptr = (IP_ENTRY *)NULL;
  ptr = ip_hash_table[hash_index = hash_ip(ip_in)];
  while(ptr)
    {
      if(ptr->ip == ip_in)
        return(ptr);
      else
	ptr = ptr->next;
    }

  if ( (ptr = ip_hash_table[hash_index]) != (IP_ENTRY *)NULL )
    {
      if( free_ip_entries == (IP_ENTRY *)NULL)	/* it might be recoverable */
	{
	  sendto_ops("s_conf.c free_ip_entries was found NULL in find_or_add");
	  sendto_ops("rehash_ip was done, this is an error.");
	  sendto_ops("Please report to the hybrid team! ircd-hybrid@vol.com");
	  rehash_ip_hash();
	  if(free_ip_entries == (IP_ENTRY *)NULL)
	    outofmemory();
	}

      newptr = ip_hash_table[hash_index] = free_ip_entries;
      free_ip_entries = newptr->next;

      newptr->ip = ip_in;
      newptr->count = 0;
      newptr->next = ptr;
      return(newptr);
    }
  else
    {
      if( free_ip_entries == (IP_ENTRY *)NULL)  /* it might be recoverable */
        {
          sendto_ops("s_conf.c free_ip_entries was found NULL in find_or_add");
          sendto_ops("rehash_ip was done, this is an error.");
          sendto_ops("Please report to the hybrid team! ircd-hybrid@vol.com");
          rehash_ip_hash();
          if(free_ip_entries == (IP_ENTRY *)NULL)
            outofmemory();
        }

      ptr = ip_hash_table[hash_index] = free_ip_entries;
      free_ip_entries = ptr->next;
      ptr->ip = ip_in;
      ptr->count = 0;
      ptr->next = (IP_ENTRY *)NULL;
      return (ptr);
    }
}

/* 
remove_one_ip

inputs		- unsigned long IP address value
output		- NONE
side effects	- ip address listed, is looked up in ip hash table
		  and number of ip#'s for that ip decremented.
		  if ip # count reaches 0, the IP_ENTRY is returned
		  to the free_ip_enties link list.
*/

void remove_one_ip(unsigned long ip_in)
{
  int hash_index;
  IP_ENTRY *last_ptr;
  IP_ENTRY *ptr;
  IP_ENTRY *old_free_ip_entries;

  last_ptr = ptr = ip_hash_table[hash_index = hash_ip(ip_in)];
  while(ptr)
    {
      if(ptr->ip == ip_in)
	{
          if(ptr->count != 0)
            ptr->count--;
	  if(ptr->count == 0)
	    {
              if(ip_hash_table[hash_index] == ptr)
                ip_hash_table[hash_index] = ptr->next;
	      else
		last_ptr->next = ptr->next;
	
              if(free_ip_entries != (IP_ENTRY *)NULL)
                {
                  old_free_ip_entries = free_ip_entries;
                  free_ip_entries = ptr;
                  ptr->next = old_free_ip_entries;
                }
              else
                {
                  free_ip_entries = ptr;
                  ptr->next = (IP_ENTRY *)NULL;
                }
	    }
	  return;
	}
      else
        {
          last_ptr = ptr;
	  ptr = ptr->next;
        }
    }
  sendto_ops("s_conf.c couldn't find ip# in hash table in remove_one_ip()");
  sendto_ops("Please report to the hybrid team! ircd-hybrid@vol.com");
  return;
}

/*
hash_ip()

input		- unsigned long ip address
output		- integer value used as index into hash table
side effects	- hopefully, none
*/

static int hash_ip(unsigned long ip)
{
  int hash;
  ip = ntohl(ip);
  hash = ((ip >>= 12) + ip) & (IP_HASH_SIZE-1);
  return(hash);
}

/* Added so s_debug could check memory usage in here -Dianora */
/*
count_ip_hash

inputs		- pointer to counter of number of ips hashed 
		- pointer to memory used for ip hash
output		- returned via pointers input
side effects	- NONE

number of hashed ip #'s is counted up, plus the amount of memory
used in the hash.
*/

void count_ip_hash(int *number_ips_stored,u_long *mem_ips_stored)
{
  IP_ENTRY *ip_hash_ptr;
  int i;

  *number_ips_stored = 0;
  *mem_ips_stored = 0;

  for(i = 0; i < IP_HASH_SIZE ;i++)
    {
      ip_hash_ptr = ip_hash_table[i];
      while(ip_hash_ptr)
        {
          *number_ips_stored = *number_ips_stored + 1;
          *mem_ips_stored = *mem_ips_stored +
             sizeof(IP_ENTRY);

          ip_hash_ptr = ip_hash_ptr->next;
        }
    }
}

/*
rehash_ip_hash

inputs		- NONE
output		- NONE
side effects	- 

This function clears the ip hash table, then re-enters all the ip's
found in local clients.

N.B. This should never have to be called, and hopefully, we can remove
this function in future versions of hybrid. i.e. if everything is working
right, there should never ever be a case where an IP is still in the ip_hash
table and the corresponding client isn't.

*/

void rehash_ip_hash()
{
  IP_ENTRY *ip_hash_ptr;
  IP_ENTRY *tmp_ip_hash_ptr;
  IP_ENTRY *old_free_ip_entries;
  int i;

  /* first, clear the ip hash */

  for(i = 0; i < IP_HASH_SIZE ;i++)
    {
      ip_hash_ptr = ip_hash_table[i];
      while(ip_hash_ptr)
        {
	  tmp_ip_hash_ptr = ip_hash_ptr->next;
	  if(free_ip_entries)
	    {
	      old_free_ip_entries = free_ip_entries;
	      free_ip_entries = ip_hash_ptr;
	      ip_hash_ptr->next = old_free_ip_entries;
	    }
	  else
	    {
	      free_ip_entries = ip_hash_ptr;
	      ip_hash_ptr->next = (IP_ENTRY *)NULL;
	    }
          ip_hash_ptr = tmp_ip_hash_ptr;
        }
      ip_hash_table[i] = (IP_ENTRY *)NULL;
    }

  for (i = highest_fd; i >= 0; i--)
    {
      if (local[i] && MyClient(local[i]))
	{
          if(local[i]->fd >= 0)
	    {
	      ip_hash_ptr = find_or_add_ip(local[i]->ip.s_addr);
              ip_hash_ptr->count++;
            }
        }
    }
}

/*
 * Find the single N line and return pointer to it (from list).
 * If more than one then return NULL pointer.
 */
aConfItem	*count_cnlines(Link *lp)
{
  Reg	aConfItem	*aconf, *cline = NULL, *nline = NULL;
  
  for (; lp; lp = lp->next)
    {
      aconf = lp->value.aconf;
      if (!(aconf->status & CONF_SERVER_MASK))
	continue;
      if (aconf->status == CONF_CONNECT_SERVER && !cline)
	cline = aconf;
      else if (aconf->status == CONF_NOCONNECT_SERVER && !nline)
	nline = aconf;
    }
  return nline;
}

/*
** detach_conf
**	Disassociate configuration from the client.
**      Also removes a class from the list if marked for deleting.
*/
int	detach_conf(aClient *cptr,aConfItem *aconf)
{
  Reg	Link	**lp, *tmp;

  lp = &(cptr->confs);

  while (*lp)
    {
      if ((*lp)->value.aconf == aconf)
	{
	  if ((aconf) && (Class(aconf)))
	    {
	      if (aconf->status & CONF_CLIENT_MASK)
		if (ConfLinks(aconf) > 0)
		  --ConfLinks(aconf);
	      if (ConfMaxLinks(aconf) == -1 &&
		  ConfLinks(aconf) == 0)
		{
		  free_class(Class(aconf));
		  Class(aconf) = NULL;
		}
	    }
	  if (aconf && !--aconf->clients && IsIllegal(aconf))
	    free_conf(aconf);
	  tmp = *lp;
	  *lp = tmp->next;
	  free_link(tmp);
	  return 0;
	}
      else
	lp = &((*lp)->next);
    }
  return -1;
}

static	int	is_attached(aConfItem *aconf,aClient *cptr)
{
  Reg	Link	*lp;

  for (lp = cptr->confs; lp; lp = lp->next)
    if (lp->value.aconf == aconf)
      break;
  
  return (lp) ? 1 : 0;
}

/*
** attach_conf
**	Associate a specific configuration entry to a *local*
**	client (this is the one which used in accepting the
**	connection). Note, that this automatically changes the
**	attachment if there was an old one...
*/
int	attach_conf(aClient *cptr,aConfItem *aconf)
{
  Reg Link *lp;

  if (is_attached(aconf, cptr))
    {
      return 1;
    }
  if (IsIllegal(aconf))
    {
      return -1;
    }
  /*
    By using "ConfLinks(aconf) >= ConfMaxLinks(aconf)....
    the client limit is set by the Y line, connection class, not
    by the individual client count in each I line conf.

    -Dianora

   */

  /* If the requested change, is to turn them into an OPER, then
     they are already attached to a fd there is no need to check for
     max in a class now is there?

     -Dianora
  */

  /* If OLD_Y_LIMIT is defined the code goes back to the old way
     I lines used to work, i.e. number of clients per I line
     not total in Y
     -Dianora
  */
#ifdef OLD_Y_LIMIT
  if ((aconf->status & (CONF_LOCOP | CONF_OPERATOR | CONF_CLIENT)) &&
    aconf->clients >= ConfMaxLinks(aconf) && ConfMaxLinks(aconf) > 0)
#else
  if ( (aconf->status & (CONF_LOCOP | CONF_OPERATOR ) ) == 0 )
    {
      if ((aconf->status & CONF_CLIENT) &&
	  ConfLinks(aconf) >= ConfMaxLinks(aconf) && ConfMaxLinks(aconf) > 0)
#endif
	{
	  if (!find_fline(cptr))
	    {
	      return -3;	/* Use this for printing error message */
	    }
	  else
	    {
	      send(cptr->fd,
		   "NOTICE FLINE :I: line is full, but you have an F: line!\n",
		   56, 0);
	    }
	}
#ifndef OLD_Y_LIMIT
    }
#endif

  lp = make_link();
  lp->next = cptr->confs;
  lp->value.aconf = aconf;
  cptr->confs = lp;
  aconf->clients++;
  if (aconf->status & CONF_CLIENT_MASK)
    ConfLinks(aconf)++;
  return 0;
}


aConfItem *find_admin()
{
  Reg aConfItem *aconf;

  for (aconf = conf; aconf; aconf = aconf->next)
    if (aconf->status & CONF_ADMIN)
      break;
  
  return (aconf);
}

aConfItem *find_me()
{
  Reg aConfItem *aconf;
  for (aconf = conf; aconf; aconf = aconf->next)
    if (aconf->status & CONF_ME)
      return(aconf);

  return((aConfItem *)NULL);	/* oh oh... is there code to handle
				   this case ? - Dianora */
}

/*
 * attach_confs
 *  Attach a CONF line to a client if the name passed matches that for
 * the conf file (for non-C/N lines) or is an exact match (C/N lines
 * only).  The difference in behaviour is to stop C:*::* and N:*::*.
 */
aConfItem *attach_confs(aClient *cptr,char *name,int statmask)
{
  Reg aConfItem *tmp;
  aConfItem *first = NULL;
  int len = strlen(name);
  
  if (!name || len > HOSTLEN)
    return((aConfItem *)NULL);

  for (tmp = conf; tmp; tmp = tmp->next)
    {
      if ((tmp->status & statmask) && !IsIllegal(tmp) &&
	  ((tmp->status & (CONF_SERVER_MASK|CONF_HUB)) == 0) &&
	  tmp->name && !match(tmp->name, name))
	{
	  if (!attach_conf(cptr, tmp) && !first)
	    first = tmp;
	}
      else if ((tmp->status & statmask) && !IsIllegal(tmp) &&
	       (tmp->status & (CONF_SERVER_MASK|CONF_HUB)) &&
	       tmp->name && !mycmp(tmp->name, name))
	{
	  if (!attach_conf(cptr, tmp) && !first)
	    first = tmp;
	}
    }
  return (first);
}

/*
 * Added for new access check    meLazy
 */
aConfItem *attach_confs_host(aClient *cptr,char *host,int statmask)
{
  Reg	aConfItem *tmp;
  aConfItem *first = NULL;
  int	len = strlen(host);
  
  if (!host || len > HOSTLEN)
    return( (aConfItem *)NULL);

  for (tmp = conf; tmp; tmp = tmp->next)
    {
      if ((tmp->status & statmask) && !IsIllegal(tmp) &&
	  (tmp->status & CONF_SERVER_MASK) == 0 &&
	  (!tmp->host || match(tmp->host, host) == 0))
	{
	  if (!attach_conf(cptr, tmp) && !first)
	    first = tmp;
	}
      else if ((tmp->status & statmask) && !IsIllegal(tmp) &&
	       (tmp->status & CONF_SERVER_MASK) &&
	       (tmp->host && mycmp(tmp->host, host) == 0))
	{
	  if (!attach_conf(cptr, tmp) && !first)
	    first = tmp;
	}
    }
  return (first);
}

/*
 * find a conf entry which matches the hostname and has the same name.
 */
aConfItem *find_conf_exact(char *name,
			   char *user,
			   char *host,
			   int statmask)
{
  Reg	aConfItem *tmp;
  char	userhost[USERLEN+HOSTLEN+3];

  (void)ircsprintf(userhost, "%s@%s", user, host);

  for (tmp = conf; tmp; tmp = tmp->next)
    {
      if (!(tmp->status & statmask) || !tmp->name || !tmp->host ||
	  mycmp(tmp->name, name))
	continue;
      /*
      ** Accept if the *real* hostname (usually sockecthost)
      ** socket host) matches *either* host or name field
      ** of the configuration.
      */
      if (match(tmp->host, userhost))
	continue;
      if (tmp->status & (CONF_OPERATOR|CONF_LOCOP))
	{
	  if (tmp->clients < MaxLinks(Class(tmp)))
	    return tmp;
	  else
	    continue;
	}
      else
	return tmp;
    }
  return((aConfItem *)NULL);
}

/*
find_conf_name()


** Accept if the *real* hostname (usually sockecthost)
** matches *either* host or name field of the configuration.
*/

aConfItem *find_conf_name(char *name,int statmask)
{
  Reg	aConfItem *tmp;
 
  for (tmp = conf; tmp; tmp = tmp->next)
    {
      if ((tmp->status & statmask) &&
	  (!tmp->name || match(tmp->name, name) == 0))
	return tmp;
    }
  return((aConfItem *)NULL);
}

aConfItem *find_conf(Link *lp,char *name,int statmask)
{
  Reg	aConfItem *tmp;
  int	namelen = name ? strlen(name) : 0;
  
  if (namelen > HOSTLEN)
    return ((aConfItem *) NULL);

  for (; lp; lp = lp->next)
    {
      tmp = lp->value.aconf;
      if ((tmp->status & statmask) &&
	  (((tmp->status & (CONF_SERVER_MASK|CONF_HUB)) &&
	    tmp->name && !mycmp(tmp->name, name)) ||
	   ((tmp->status & (CONF_SERVER_MASK|CONF_HUB)) == 0 &&
	    tmp->name && !match(tmp->name, name))))
	return tmp;
    }
  return((aConfItem *) NULL);
}

/*
 * Added for new access check    meLazy
 */
aConfItem *find_conf_host(Link *lp, char *host,int statmask)
{
  Reg	aConfItem *tmp;
  int	hostlen = host ? strlen(host) : 0;
  
  if (hostlen > HOSTLEN || BadPtr(host))
    return ((aConfItem *)NULL);
  for (; lp; lp = lp->next)
    {
      tmp = lp->value.aconf;
      if (tmp->status & statmask &&
	  (!(tmp->status & CONF_SERVER_MASK || tmp->host) ||
	   (tmp->host && !match(tmp->host, host))))
	return tmp;
    }
  return ((aConfItem *)NULL);
}

/*
 * find_conf_ip
 *
 * Find a conf line using the IP# stored in it to search upon.
 * Added 1/8/92 by Avalon.
 */
aConfItem *find_conf_ip(Link *lp,char *ip,char *user, int statmask)
{
  Reg	aConfItem *tmp;
  Reg	char	*s;
  
  for (; lp; lp = lp->next)
    {
      tmp = lp->value.aconf;
      if (!(tmp->status & statmask))
	continue;
      s = index(tmp->host, '@');
      if(s == (char *)NULL)
	continue;
      *s = '\0';
      if (match(tmp->host, user))
	{
	  *s = '@';
	  continue;
	}
      *s = '@';
      if (!bcmp((char *)&tmp->ipnum, ip, sizeof(struct in_addr)))
	return tmp;
    }
  return ((aConfItem *)NULL);
}

/*
 * find_conf_entry
 *
 * - looks for a match on all given fields.
 */
aConfItem *find_conf_entry(aConfItem *aconf, int mask)
{
  Reg	aConfItem *bconf;

  for (bconf = conf, mask &= ~CONF_ILLEGAL; bconf; bconf = bconf->next)
    {
      if (!(bconf->status & mask) || (bconf->port != aconf->port))
	continue;
      
      if ((BadPtr(bconf->host) && !BadPtr(aconf->host)) ||
	  (BadPtr(aconf->host) && !BadPtr(bconf->host)))
	continue;
      if (!BadPtr(bconf->host) && mycmp(bconf->host, aconf->host))
	continue;

      if ((BadPtr(bconf->passwd) && !BadPtr(aconf->passwd)) ||
	  (BadPtr(aconf->passwd) && !BadPtr(bconf->passwd)))
	continue;
      if (!BadPtr(bconf->passwd) &&
	  mycmp(bconf->passwd, aconf->passwd))
      continue;

	  if ((BadPtr(bconf->name) && !BadPtr(aconf->name)) ||
	      (BadPtr(aconf->name) && !BadPtr(bconf->name)))
	  continue;
	  if (!BadPtr(bconf->name) && mycmp(bconf->name, aconf->name))
	  continue;
	  break;
	  }
      return bconf;
}

/*
 * rehash
 *
 * Actual REHASH service routine. Called with sig == 0 if it has been called
 * as a result of an operator issuing this command, else assume it has been
 * called as a result of the server receiving a HUP signal.
 */
int	rehash(aClient *cptr,aClient *sptr,int sig)
{
  Reg	aConfItem **tmp = &conf, *tmp2;
  Reg	aClass	*cltmp;
  Reg	aClient	*acptr;
  Reg	int	i;
  int	ret = 0;

  if (sig == SIGHUP)
    {
      sendto_ops("Got signal SIGHUP, reloading ircd conf. file");
#ifdef	ULTRIX
      if (fork() > 0)
	exit(0);
      write_pidfile();
#endif
    }

  /* Shadowfax's LOCKFILE code */
#ifdef LOCKFILE
  do_pending_klines();
#endif

  for (i = 0; i <= highest_fd; i++)
    if ((acptr = local[i]) && !IsMe(acptr))
      {
	/*
	 * Nullify any references from client structures to
	 * this host structure which is about to be freed.
	 * Could always keep reference counts instead of
	 * this....-avalon
	 */
	acptr->hostp = NULL;
#if defined(R_LINES_REHASH) && !defined(R_LINES_OFTEN)
	if (find_restrict(acptr))
	  {
	    sendto_ops("Restricting %s, closing lp",
		       get_client_name(acptr,FALSE));
	    if (exit_client(cptr,acptr,sptr,"R-lined") ==
		FLUSH_BUFFER)
	      ret = FLUSH_BUFFER;
	  }
#endif
		    }

  while ((tmp2 = *tmp))
    if (tmp2->clients || tmp2->status & CONF_LISTEN_PORT)
      {
	/*
	** Configuration entry is still in use by some
	** local clients, cannot delete it--mark it so
	** that it will be deleted when the last client
	** exits...
	*/
	if (!(tmp2->status & (CONF_LISTEN_PORT|CONF_CLIENT)))
	  {
	    *tmp = tmp2->next;
	    tmp2->next = NULL;
	  }
	else
	  tmp = &tmp2->next;
	tmp2->status |= CONF_ILLEGAL;
      }
    else
      {
	*tmp = tmp2->next;
	free_conf(tmp2);
      }

  /*
   * We don't delete the class table, rather mark all entries
   * for deletion. The table is cleaned up by check_class. - avalon
   */
  for (cltmp = NextClass(FirstClass()); cltmp; cltmp = NextClass(cltmp))
    MaxLinks(cltmp) = -1;

  /* do we really want to flush the DNS entirely on a SIGHUP?
     why not let that be controlled by oper /rehash, and use SIGHUP
     only to change conf file, if one doesn't have a valid O yet? :-)
     -Dianora
     */

  if (sig != SIGINT)
    flush_cache();		/* Flush DNS cache */

  clear_conf_list(&KList1);
  clear_conf_list(&KList2);
  clear_conf_list(&KList3);
  
  clear_conf_list(&DList1);	/* Only d lines of this form allowed */

  clear_conf_list(&BList1);
  clear_conf_list(&BList2);
  clear_conf_list(&BList3);

  clear_conf_list(&EList1);
  clear_conf_list(&EList2);
  clear_conf_list(&EList3);

  clear_conf_list(&FList1);
  clear_conf_list(&FList2);
  clear_conf_list(&FList3);

  (void) initconf(0,configfile);
#ifdef SEPARATE_QUOTE_KLINES_BY_DATE
  {
    char timebuffer[20];
    char filenamebuf[1024];
    struct tm *tmptr;
    tmptr = localtime(&NOW);
    (void)strftime(timebuffer, 20, "%y%m%d", tmptr);
    ircsprintf(filenamebuf, "%s.%s", klinefile, timebuffer);
    if(initconf(0,filenamebuf))
      {
	sendto_ops("There was a problem opening %s",filenamebuf);
      }
    else
      {
	sendto_ops("loaded K-lines from %s", filenamebuf);
      }
  }
#else
#ifdef KLINEFILE
  (void) initconf(0,klinefile);
#endif
#endif
  close_listeners();

  /*
   * flush out deleted I and P lines although still in use.
   */
  for (tmp = &conf; (tmp2 = *tmp); )
    if (!(tmp2->status & CONF_ILLEGAL))
      tmp = &tmp2->next;
    else
      {
	*tmp = tmp2->next;
	tmp2->next = NULL;
	if (!tmp2->clients)
	  free_conf(tmp2);
      }
  rehashed = 1;
  return ret;
}

/*
 * openconf
 *
 * returns -1 on any error or else the fd opened from which to read the
 * configuration file from.  This may either be th4 file direct or one end
 * of a pipe from m4.
 */
int	openconf(char *filename)
{
#ifdef	M4_PREPROC
  int	pi[2], i;

  if (pipe(pi) == -1)
    return -1;
  switch(vfork())	
    {
    case -1 :
      return -1;
    case 0 :
      (void)close(pi[0]);
      if (pi[1] != 1)
	{
	  (void)dup2(pi[1], 1);
	  (void)close(pi[1]);
	}
      (void)dup2(1,2);
      for (i = 3; i < MAXCONNECTIONS; i++)
	if (local[i])
	  (void) close(i);
      /*
       * m4 maybe anywhere, use execvp to find it.  Any error
       * goes out with report_error.  Could be dangerous,
       * two servers running with the same fd's >:-) -avalon
       */
      (void)execlp("m4", "m4", "ircd.m4", configfile, 0);
      report_error("Error executing m4 %s:%s", &me);
      exit(-1);
    default :
      (void)close(pi[1]);
      return pi[0];
    }
#else
  return open(filename, O_RDONLY);
#endif
}
extern char *getfield();

/*
** initconf() 
**    Read configuration file.
**
*
* Inputs 	- opt 
* 		- name of config file to use
*
**    returns -1, if file cannot be opened
**             0, if file opened
*/

#define MAXCONFLINKS 150

int 	initconf(int opt, char *conf_file)
{
  static	char	quotes[9][2] = {{'b', '\b'}, {'f', '\f'}, {'n', '\n'},
					{'r', '\r'}, {'t', '\t'}, {'v', '\v'},
					{'\\', '\\'}, { 0, 0}};
  Reg	char	*tmp, *s;
  int	fd, i, dontadd;
  char	line[512], c[80];
  int	ccount = 0, ncount = 0;
  u_long vaddr;

  aConfItem *aconf = NULL;

  Debug((DEBUG_DEBUG, "initconf():  = %s", conf_file));
  if ((fd = openconf(conf_file)) == -1)
    {
#ifdef	M4_PREPROC
      (void)wait(0);
#endif
      return -1;
    }
  (void)dgets(-1, NULL, 0); /* make sure buffer is at empty pos */
  while ((i = dgets(fd, line, sizeof(line) - 1)) > 0)
    {
      line[i] = '\0';
      if ((tmp = (char *)index(line, '\n')))
	*tmp = '\0';
      else while(dgets(fd, c, sizeof(c) - 1) > 0)
	if ((tmp = (char *)index(c, '\n')))
	  {
	    *tmp = '\0';
	    break;
	  }
      /*
       * Do quoting of characters and # detection.
       */
      for (tmp = line; *tmp; tmp++)
	{
	  if (*tmp == '\\')
	    {
	      for (i = 0; quotes[i][0]; i++)
		if (quotes[i][0] == *(tmp+1))
		  {
		    *tmp = quotes[i][1];
		    break;
		  }
	      if (!quotes[i][0])
		*tmp = *(tmp+1);
	      if (!*(tmp+1))
		break;
	      else
		for (s = tmp; (*s = *(s+1)); s++)
		  ;
	    }
	  else if (*tmp == '#')
	    *tmp = '\0';
	}
      if (!*line || line[0] == '#' || line[0] == '\n' ||
	  line[0] == ' ' || line[0] == '\t')
	continue;
      /* Could we test if it's conf line at all?	-Vesa */
      if (line[1] != ':')
	{
	  Debug((DEBUG_ERROR, "Bad config line: %s", line));
	  continue;
	}
      if (aconf)
	free_conf(aconf);
      aconf = make_conf();

      tmp = getfield(line);
      if (!tmp)
	continue;
      dontadd = 0;
      switch (*tmp)
	{
	case 'A': /* Name, e-mail address of administrator */
	case 'a': /* of this server. */
	  aconf->status = CONF_ADMIN;
	  break;

	case 'B': /* Bots we know are ok */
	case 'b':
	  aconf->status = CONF_BLINE;
	  break;

	case 'C': /* Server where I should try to connect */
	case 'c': /* in case of lp failures             */
	  ccount++;
	  aconf->status = CONF_CONNECT_SERVER;
	  break;

	case 'D': /* Deny lines (immediate refusal) */
	case 'd':
	  aconf->status = CONF_DLINE;
	  break;

	case 'E': /* Addresses that we don't want to check */
	case 'e': /* for bots. */
	  aconf->status = CONF_ELINE;
	  break;

	case 'F': /* Super-Exempt hosts */
	case 'f':
	  aconf->status = CONF_FLINE;
	  break;

	case 'H': /* Hub server line */
	case 'h':
	  aconf->status = CONF_HUB;
	  break;

#ifdef LITTLE_I_LINES
	case 'i': /* to connect me */
	  aconf->status = CONF_CLIENT;
	  aconf->flags |= CONF_FLAGS_LITTLE_I_LINE;
	  break;

	case 'I': /* Just plain normal irc client trying  */
	  aconf->status = CONF_CLIENT;
	  break;
#else
	case 'i': /* to connect me */
	case 'I': /* Just plain normal irc client trying  */
	  aconf->status = CONF_CLIENT;
	  break;
#endif
	case 'K': /* Kill user line on irc.conf           */
	case 'k':
	  aconf->status = CONF_KILL;
	  break;

	case 'L': /* guaranteed leaf server */
	case 'l':
	  aconf->status = CONF_LEAF;
	  break;

	  /* Me. Host field is name used for this host */
	  /* and port number is the number of the port */
	case 'M':
	case 'm':
	  aconf->status = CONF_ME;
	  break;

	case 'N': /* Server where I should NOT try to     */
	case 'n': /* connect in case of lp failures     */
	  /* but which tries to connect ME        */
	  ++ncount;
	  aconf->status = CONF_NOCONNECT_SERVER;
	  break;

          /* Operator. Line should contain at least */
          /* password and host where connection is  */

	case 'O':
	  aconf->status = CONF_OPERATOR;
	  break;
	  /* Local Operator, (limited privs --SRB) */

	case 'o':
	  aconf->status = CONF_LOCOP;
	  break;

	case 'P': /* listen port line */
	case 'p':
	  aconf->status = CONF_LISTEN_PORT;
	  break;

#ifdef R_LINES
	case 'R': /* extended K line */
	case 'r': /* Offers more options of how to restrict */
	  aconf->status = CONF_RESTRICT;
	  break;
#endif
	case 'U': /* Uphost, ie. host where client reading */
	case 'u': /* this should connect.                  */
	  /* This is for client only, I must ignore this */
	  /* ...U-line should be removed... --msa */
	  break;

	case 'Y':
	case 'y':
	  aconf->status = CONF_CLASS;
	  break;

	default:
	  Debug((DEBUG_ERROR, "Error in config file: %s", line));
	  break;
	}
      if (IsIllegal(aconf))
	continue;

      for (;;) /* Fake loop, that I can use break here --msa */
	{
	  if ((tmp = getfield(NULL)) == NULL)
	    break;
	  DupString(aconf->host, tmp);
	  if ((tmp = getfield(NULL)) == NULL)
	    break;
	  DupString(aconf->passwd, tmp);
	  if ((tmp = getfield(NULL)) == NULL)
	    break;
	  DupString(aconf->name, tmp);
	  if ((tmp = getfield(NULL)) == NULL)
	    break;
	  aconf->port = atoi(tmp);
	  if ((tmp = getfield(NULL)) == NULL)
	    break;
	  Class(aconf) = find_class(atoi(tmp));
	  break;
          /* NOTREACHED */
	}
      /*
      ** If conf line is a class definition, create a class entry
      ** for it and make the conf_line illegal and delete it.
      */
      if (aconf->status & CONF_CLASS)
	{
	  add_class(atoi(aconf->host), atoi(aconf->passwd),
		    atoi(aconf->name), aconf->port,
		    tmp ? atoi(tmp) : 0);
	  continue;
	}
      /*
      ** associate each conf line with a class by using a pointer
      ** to the correct class record. -avalon
      */
      if (aconf->status & (CONF_CLIENT_MASK|CONF_LISTEN_PORT))
	{
	  if (Class(aconf) == 0)
	    Class(aconf) = find_class(0);
	  if (MaxLinks(Class(aconf)) < 0)
	    Class(aconf) = find_class(0);
	}
      if (aconf->status & (CONF_LISTEN_PORT|CONF_CLIENT))
	{
	  aConfItem *bconf;
	  
	  if ( (bconf = find_conf_entry(aconf, aconf->status)) )
	    {
	      delist_conf(bconf);
	      bconf->status &= ~CONF_ILLEGAL;
	      if (aconf->status == CONF_CLIENT)
		{
		  bconf->class->links -= bconf->clients;
		  bconf->class = aconf->class;
		  bconf->class->links += bconf->clients;
		}
	      free_conf(aconf);
	      aconf = bconf;
	    }
	  else if (aconf->host &&
		   aconf->status == CONF_LISTEN_PORT)
	    (void)add_listener(aconf);
	}
      if (aconf->status & CONF_SERVER_MASK)
	if (ncount > MAXCONFLINKS || ccount > MAXCONFLINKS ||
	    !aconf->host || index(aconf->host, '*') ||
	    index(aconf->host,'?') || !aconf->name)
	  continue;
      
      if (aconf->status &
	  (CONF_SERVER_MASK|CONF_LOCOP|CONF_OPERATOR))
	if (!index(aconf->host, '@') && *aconf->host != '/')
	  {
	    char	*newhost;
	    int	len = 3;	/* *@\0 = 3 */
	    
	    len += strlen(aconf->host);
	    newhost = (char *)MyMalloc(len);
	    (void)ircsprintf(newhost, "*@%s", aconf->host);
	    MyFree(aconf->host);
	    aconf->host = newhost;
	  }
      if (aconf->status & CONF_SERVER_MASK)
	{
	  if (BadPtr(aconf->passwd))
	    continue;
	  else if (!(opt & BOOT_QUICK))
	    (void)lookup_confhost(aconf);
	}
      /*
      ** Own port and name cannot be changed after the startup.
      ** (or could be allowed, but only if all links are closed
      ** first).
      ** Configuration info does not override the name and port
      ** if previously defined. Note, that "info"-field can be
      ** changed by "/rehash".
      ** Can't change vhost mode/address either 
      */
      if (aconf->status == CONF_ME)
	{
	  strncpyzt(me.info, aconf->name, sizeof(me.info));

	  if (me.name[0] == '\0' && aconf->host[0])
          {
	    strncpyzt(me.name, aconf->host,
		      sizeof(me.name));
	    if ((aconf->passwd[0] != '\0') && (aconf->passwd[0] != '*'))
            {
		bzero((char *) &vserv, sizeof(vserv));
		vserv.sin_family = AF_INET;
		vaddr = inet_addr(aconf->passwd);
		bcopy((char *) &vaddr, (char *)&vserv.sin_addr, sizeof(struct in_addr));
		specific_virtual_host = 1;
	    }
	  }

	  if (portnum < 0 && aconf->port >= 0)
	    portnum = aconf->port;

	}
      
      if ((aconf->status & CONF_KILL) && aconf->host)
	{
	  char	*host = host_field(aconf);
	  
	  dontadd = 1;
	  switch (sortable(host))
	    {
	    case 0 :
	      l_addto_conf_list(&KList3, aconf, host_field);
	      break;
	    case 1 :
	      addto_conf_list(&KList1, aconf, host_field);
	      break;
	    case -1 :
	      addto_conf_list(&KList2, aconf, rev_host_field);
	      break;
	    }

	  MyFree(host);
	}

      if (aconf->host && (aconf->status & CONF_BLINE))
        {
	  char    *host = host_field(aconf);

	  dontadd = 1;
	  switch (sortable(host))
	    {
	    case 0 :
	      l_addto_conf_list(&BList3, aconf, host_field);
	      break;
	    case 1 :
	      addto_conf_list(&BList1, aconf, host_field);
	      break;
	    case -1 :
	      addto_conf_list(&BList2, aconf, rev_host_field);
	      break;
  	  }
	
	MyFree(host);
      }

      if (aconf->host && (aconf->status & CONF_DLINE)) {
	char    *host = host_field(aconf);

	dontadd = 1;
	switch (sortable(host))
	  {
	  case 0 :
	    break;
	  case 1 :
	    addto_conf_list(&DList1, aconf, host_field);
	    break;
	  case -1 :
	    break;
	  }
	
	MyFree(host);
      }

      if (aconf->host && (aconf->status & CONF_ELINE)) {
	char	*host = host_field(aconf);

	dontadd = 1;
	switch (sortable(host))
	  {
	  case 0 :
	    l_addto_conf_list(&EList3, aconf, host_field);
	    break;
	  case 1 :
	    addto_conf_list(&EList1, aconf, host_field);
	    break;
	  case -1 :
	    addto_conf_list(&EList2, aconf, rev_host_field);
	    break;
	  }
	MyFree(host);
      }

      if (aconf->host && (aconf->status & CONF_FLINE)) {
	char	*host = host_field(aconf);

	dontadd = 1;
	switch (sortable(host))
	  {
	  case 0 :
	    l_addto_conf_list(&FList3, aconf, host_field);
	    break;
	  case 1 :
	    addto_conf_list(&FList1, aconf, host_field);
	    break;
	  case -1 :
	    addto_conf_list(&FList2, aconf, rev_host_field);
	    break;
	  }
	MyFree(host);
      }

      (void)collapse(aconf->host);
      (void)collapse(aconf->name);
      Debug((DEBUG_NOTICE,
	     "Read Init: (%d) (%s) (%s) (%s) (%d) (%d)",
	     aconf->status, aconf->host, aconf->passwd,
	     aconf->name, aconf->port, Class(aconf)));
      if (!dontadd)
	{
	  aconf->next = conf;
	  conf = aconf;
	}
      aconf = NULL;
    }
  if (aconf)
    free_conf(aconf);
  (void)dgets(-1, NULL, 0); /* make sure buffer is at empty pos */
  (void)close(fd);
#ifdef	M4_PREPROC
  (void)wait(0);
#endif
  check_class();
  nextping = nextconnect = time(NULL);
  return 0;
}

/*
 * lookup_confhost
 *   Do (start) DNS lookups of all hostnames in the conf line and convert
 * an IP addresses in a.b.c.d number for to IP#s.

  cleaned up Aug 3'97 - Dianora
 */
static	int	lookup_confhost(aConfItem *aconf)
{
  Reg	char	*s;
  Reg	struct	hostent *hp;
  Link	ln;

  if (BadPtr(aconf->host) || BadPtr(aconf->name))
    {
      if (aconf->ipnum.s_addr == -1)
	bzero((char *)&aconf->ipnum, sizeof(struct in_addr));
      Debug((DEBUG_ERROR,"Host/server name error: (%s) (%s)",
	     aconf->host, aconf->name));
      return -1;
    }
  if ((s = index(aconf->host, '@')))
    s++;
  else
    s = aconf->host;
  /*
  ** Do name lookup now on hostnames given and store the
  ** ip numbers in conf structure.
  */
  if (!isalpha(*s) && !isdigit(*s))
    {
      if (aconf->ipnum.s_addr == -1)
	bzero((char *)&aconf->ipnum, sizeof(struct in_addr));
      Debug((DEBUG_ERROR,"Host/server name error: (%s) (%s)",
	     aconf->host, aconf->name));
      return -1;
    }

  /*
  ** Prepare structure in case we have to wait for a
  ** reply which we get later and store away.
  */
  ln.value.aconf = aconf;
  ln.flags = ASYNC_CONF;
  
  if (isdigit(*s))
    aconf->ipnum.s_addr = inet_addr(s);
  else if ((hp = gethost_byname(s, &ln)))
    bcopy(hp->h_addr, (char *)&(aconf->ipnum),
	  sizeof(struct in_addr));
  
  if (aconf->ipnum.s_addr == -1)
    bzero((char *)&aconf->ipnum, sizeof(struct in_addr));
  {
    Debug((DEBUG_ERROR,"Host/server name error: (%s) (%s)",
	   aconf->host, aconf->name));
    return -1;
  }
  /* NOTREACHED */
  return 0;
}

/* Why not just a simple macro? maybe next version... -Dianora */
/* This is also used in ircd.c now */

#ifndef K_COMMENT_ONLY
int is_comment(char *comment)
{
  return (*comment == '');
}
#endif

int     find_bline(aClient *cptr)
{
  return find_conf_match(cptr, &BList1, &BList2, &BList3);
}

int     find_dline(char *host)
{
  if (strlen(host)  > (size_t) HOSTLEN)
    return (0);

  /* Try hostnames of the form "word*" -Sol */
  /*
   * For D lines most of them are going to be nnn.nnn.nnn.*
   * to  deny a /24 - Dianora
   * I won't check for any other kind of Dline now...
  */
  
  if (find_matching_conf(&DList1, host)) 
    return ( -1 );
  else
    return (0);
}

int     find_eline(aClient *cptr)
{
  return find_conf_match(cptr, &EList1, &EList2, &EList3);
}

int     find_fline(aClient *cptr)
{
  return find_conf_match(cptr, &FList1, &FList2, &FList3);
}

/*
  find_kill

  See if this user is klined already, and if so, return aConfItem pointer
  to the entry for this kline. This wildly changes the way find_kill works
  -Dianora

*/

aConfItem *find_kill(aClient *cptr)
{
  char *host, *name;
  
  if (!cptr->user)
    return 0;

  host = cptr->sockhost;
  name = cptr->user->username;

  if (strlen(host)  > (size_t) HOSTLEN ||
      (name ? strlen(name) : 0) > (size_t) HOSTLEN)
    return (0);
  
  if (find_eline(cptr))
    return 0;

  return(find_is_klined(host,name));
}

/*
  WARNING, no sanity checking on length of name,host etc.
  thats expected to be done by caller.... *sigh* -Dianora
 */

aConfItem *find_is_klined(char *host,char *name)
{
  aConfList *list;
  char rev[HOSTLEN+1];	/* why waste 2 function calls for this ? - Dianora */

  aConfItem *kill_list_ptr;	/* used for the link list only */
  aConfItem *last_list_ptr;
  aConfItem *tmp_list_ptr;
  aConfItem *tmp;

  /* Temporary kline handling...
     I expect this list to be very tiny. (crosses fingers) so CPU
     time in this, should be minimum.
     -Dianora
  */

  if(temporary_klines)
    {
      kill_list_ptr = last_list_ptr = temporary_klines;

      while(kill_list_ptr)
	{
	  if(kill_list_ptr->hold <= NOW)	/* a kline has expired */
	    {
	      if(temporary_klines == kill_list_ptr)
		{
		  /* Its pointing to first one in link list*/
		  /* so, bypass this one, remember bad things can happen
		     if you try to use an already freed pointer.. */

		  temporary_klines = last_list_ptr = tmp_list_ptr =
		    kill_list_ptr->next;
		}
	      else
		{
		  /* its in the middle of the list, so link around it */
		  tmp_list_ptr = last_list_ptr->next = kill_list_ptr->next;
		}

	      MyFree(kill_list_ptr->host);
	      MyFree(kill_list_ptr->name);
	      MyFree(kill_list_ptr->passwd);
	      MyFree(kill_list_ptr);
	      kill_list_ptr = tmp_list_ptr;
	    }
	  else
	    {
	      if( (kill_list_ptr->name && (!name || !match(kill_list_ptr->name,
                 name))) && (kill_list_ptr->host &&
                   (!host || !match(kill_list_ptr->host,host))))
		return(kill_list_ptr);
              last_list_ptr = kill_list_ptr;
              kill_list_ptr = kill_list_ptr->next;
	    }
	}
    }


  reverse(rev, host);

/* I have NEVER seen a kline with a port field, have you?
   I have removed the testing of the port number from here
   -Dianora
*/

  /* Start with hostnames of the form "*word" (most frequent) -Sol */
  list = &KList2;
  while ((tmp = find_matching_conf(list, rev)) != NULL)
    {
#ifndef TIMED_KLINES
      if (tmp->name && (!name || !match(tmp->name, name)))
	  return(tmp);
#else
#ifdef K_COMMENT_ONLY
      if (tmp->name && (!name || !match(tmp->name, name)))
	if(tmp->passwd)
	  {
	    if(check_time_interval(tmp->passwd))
	      return(tmp);
	    else
	      return((aConfItem *)NULL);
	  }
#else
      if (tmp->name && (!name || !match(tmp->name, name)))
	if(!BadPtr(tmp->passwd) && is_comment(tmp->passwd))
	  {
	    if(check_time_interval(tmp->passwd))
	      return(tmp);
	    else
	      return((aConfItem *)NULL);
	  }
#endif
#endif
      list = NULL;
    }

  /* Try hostnames of the form "word*" -Sol */
  list = &KList1;
  while ((tmp = find_matching_conf(list, host)) != NULL)
    {
#ifndef TIMED_KLINES
      if (tmp->name && (!name || !match(tmp->name, name)))
	  return(tmp);
#else
#ifdef K_COMMENT_ONLY
      if (tmp->name && (!name || !match(tmp->name, name)))
	if(tmp->passwd)
	  {
	    if(check_time_interval(tmp->passwd))
	      return(tmp);
	    else
	      return((aConfItem *)NULL);
	  }
#else
      if (tmp->name && (!name || !match(tmp->name, name)))
	if(!BadPtr(tmp->passwd) && is_comment(tmp->passwd))
	  {
	    if(check_time_interval(tmp->passwd))
	      return(tmp);
	    else
	      return((aConfItem *)NULL);
	  }
#endif
#endif
      list = NULL;
    }

  /* If none of the above worked, try non-sorted entries -Sol */
  list = &KList3;
  while ((tmp = l_find_matching_conf(list, host)) != NULL)
    {
#ifndef TIMED_KLINES
      if (tmp->host && tmp->name && (!name || !match(tmp->name, name)))
	  return(tmp);
#else
#ifdef K_COMMENT_ONLY
      if (tmp->host && tmp->name && (!name || !match(tmp->name, name)))
	if(tmp->passwd)
	  {
	    if(check_time_interval(tmp->passwd))
	      return(tmp);
	    else
	      return((aConfItem *)NULL);
	  }
#else
      if (tmp->host && tmp->name && (!name || !match(tmp->name, name)))
	if(!BadPtr(tmp->passwd) && is_comment(tmp->passwd))
	  {
	    if(check_time_interval(tmp->passwd))
	       return(tmp);
	    else
	      return((aConfItem *)NULL);
	  }
#endif
#endif
      list = NULL;
    }
  return((aConfItem *)NULL);
}

/*
report_matching_host_klines

inputs		- aClient pointer pointing to user who requested stats k
output		- NONE
side effects	-

report klines that are in the same "close" domain as user

-Dianora
*/

void report_matching_host_klines(aClient *cptr,char *host)
{
  char *pass;
  char *name = (char *)NULL;
  char *found_host = (char *)NULL;
  int  port;
  aConfItem *tmp;
  aConfList *list;
  static char null[] = "<NULL>";
  char rev[HOSTLEN+1];	/* why waste 2 function calls for this ? - Dianora */

  if (strlen(host) > (size_t) HOSTLEN ||
      (name ? strlen(name) : 0) > (size_t) HOSTLEN)
    return;

  reverse(rev, host);

  /* Start with hostnames of the form "*word" (most frequent) -Sol */
  list = &KList2;
  while ((tmp = find_matching_conf(list, rev)) != NULL)
    {
      pass = BadPtr(tmp->passwd) ? null : tmp->passwd;
      name = BadPtr(tmp->name) ? null : tmp->name;
      found_host = BadPtr(tmp->host) ? null : tmp->host;
#ifdef K_COMMENT_ONLY
      if (tmp->status == CONF_KILL)
	sendto_one(cptr, rpl_str(RPL_STATSKLINE), me.name,
		   cptr->name, 'K', found_host,
		   name, pass);
#else
      port = BadPtr(tmp->port) ? 0 : tmp->port;
      if (tmp->status == CONF_KILL)
	sendto_one(cptr, rpl_str(RPL_STATSKLINE), me.name,
		   cptr->name, 'K', found_host,
		   pass, name, port, get_conf_class(tmp));
#endif
      list = NULL;
    }

  /* Try hostnames of the form "word*" -Sol */
  list = &KList1;
  while ((tmp = find_matching_conf(list, host)) != NULL)
    {
      pass = BadPtr(tmp->passwd) ? null : tmp->passwd;
      name = BadPtr(tmp->name) ? null : tmp->name;
      found_host = BadPtr(tmp->host) ? null : tmp->host;
#ifdef K_COMMENT_ONLY
      if (tmp->status == CONF_KILL)
	sendto_one(cptr, rpl_str(RPL_STATSKLINE), me.name,
		   cptr->name, 'K', found_host,
		   name, pass);
#else
      port = BadPtr(tmp->port) ? 0 : tmp->port;
      if (tmp->status == CONF_KILL)
	sendto_one(cptr, rpl_str(RPL_STATSKLINE), me.name,
		   cptr->name, 'K', found_host,
		   pass, name, port, get_conf_class(tmp));
#endif
      list = NULL;
    }

  /* If none of the above worked, try non-sorted entries -Sol */
  list = &KList3;
  while ((tmp = l_find_matching_conf(list, host)) != NULL)
    {
      pass = BadPtr(tmp->passwd) ? null : tmp->passwd;
      name = BadPtr(tmp->name) ? null : tmp->name;
      found_host = BadPtr(tmp->host) ? null : tmp->host;
#ifdef K_COMMENT_ONLY
      if (tmp->status == CONF_KILL)
	sendto_one(cptr, rpl_str(RPL_STATSKLINE), me.name,
		   cptr->name, 'K', found_host,
		   name, pass);
#else
      port = BadPtr(tmp->port) ? 0 : tmp->port;
      if (tmp->status == CONF_KILL)
	sendto_one(cptr, rpl_str(RPL_STATSKLINE), me.name,
		   cptr->name, 'K', found_host,
		   pass, name, port, get_conf_class(tmp));
#endif
      list = NULL;
    }
}


/*
find_dkill is only called when a /quote dline is applied
all quote dlines are sortable, of the form nnn.nnn.nnn.* or nnn.nnn.nnn.nnn
There is no reason to check for all three possible formats as in klines

- Dianora

inputs		- client pointer
output		- return 1 if match is found, and loser should die
		  return 0 if no match is found and loser shouldn't die
side effects	- only sortable dline tree is searched
*/

aConfItem *find_dkill(aClient *cptr)
{
  char *host;

  if (!cptr->user)
    return 0;

  host = cptr->hostip;	/* guaranteed to always be less than
			   HOSTIPLEN - Dianora */

  if (find_eline(cptr))
    return 0;

  return(find_is_dlined(host));
}

/*
find_is_dlined

inputs		- pointer to host ip#
output		- pointer to aConfItem if found, NULL pointer if not
side effects	-
*/

aConfItem *find_is_dlined(char *host)
{
  aConfItem *tmp;

  /* This will be the only form possible in a quote dline */
  /* Try hostnames of the form "word*" -Sol */

  /* This code is almost identical to find_dline
     but in the case of an active quote dline, the loser deserves
     to know why they are being dlined don't they? - Dianora
  */

  if ((tmp = find_matching_conf(&DList1, host)) == NULL)
    {
      return ((aConfItem *)NULL);
    }

  return (tmp);
}

int	find_conf_match(aClient *cptr,
			aConfList *List1,
			aConfList *List2,
			aConfList *List3)
{
  char	*host, *name;
  aConfItem *tmp;
  char rev[HOSTLEN+1];	/* why waste 2 function calls for this ? - Dianora */
  aConfList *list;

  if (!cptr->user)
    return 0;

  host = cptr->sockhost;
  name = cptr->user->username;
  
  if (strlen(host)  > (size_t) HOSTLEN ||
      (name ? strlen(name) : 0) > (size_t) HOSTLEN)
    return (0);

  reverse(rev, host);

  /* Start with hostnames of the form "*word" (most frequent) -Sol */
  list = List2;
  while ((tmp = find_matching_conf(list, rev)) != NULL)
    {
      if (tmp->name && (!name || !match(tmp->name, name)))
	{
	  return (tmp ? -1 : 0);
	}
      list = NULL;
    }
  
  /* Try hostnames of the form "word*" -Sol */
  list = List1;
  while ((tmp = find_matching_conf(list, host)) != NULL)
    {
      if (tmp->name && (!name || !match(tmp->name, name)))
	{
	  return (tmp ? -1 : 0);
	}
      list = NULL;
    }

  /* If none of the above worked, try non-sorted entries -Sol */
  list = List3;
  while ((tmp = l_find_matching_conf(list, host)) != NULL)
    {
      if (tmp->host && tmp->name && (!name || !match(tmp->name, name)))
	{
	  return (tmp ? -1 : 0);
	}
      list = NULL;
    }

  return (tmp ? -1 : 0);
}

/* add_temp_kline

inputs		- pointer to aConfItem
output		- none
Side effects	- links in given aConfItem into temporary kline link list

-Dianora
*/

void add_temp_kline(aConfItem *aconf)
{
  aconf->next = temporary_klines;
  temporary_klines = aconf;
}

/* flush_temp_klines

inputs		- NONE
output		- NONE
side effects	- All temporary klines are flushed out. 
		  really should be used only for cases of extreme
		  goof up for now.
*/
void flush_temp_klines()
{
  aConfItem *kill_list_ptr;

  if( (kill_list_ptr = temporary_klines) )
    {
      while(kill_list_ptr)
        {
          temporary_klines = kill_list_ptr->next;
          MyFree(kill_list_ptr->host);
          MyFree(kill_list_ptr->name);
	  MyFree(kill_list_ptr->passwd);
          MyFree(kill_list_ptr);
	  kill_list_ptr = temporary_klines;
        }
    }
}

#ifdef GLINES
aConfItem *find_gkill(aClient *cptr)
{
  char *host, *name;
  
  if (!cptr->user)
    return 0;

  host = cptr->sockhost;
  name = cptr->user->username;

  if (strlen(host)  > (size_t) HOSTLEN ||
      (name ? strlen(name) : 0) > (size_t) HOSTLEN)
    return (0);
  
  if (find_eline(cptr))
    return 0;

  return(find_is_glined(host,name));
}

/*
  WARNING, no sanity checking on length of name,host etc.
  thats expected to be done by caller.... *sigh* -Dianora
 */

aConfItem *find_is_glined(char *host,char *name)
{
  aConfItem *kill_list_ptr;	/* used for the link list only */
  aConfItem *last_list_ptr;
  aConfItem *tmp_list_ptr;

  /* gline handling... exactly like temporary klines 
     I expect this list to be very tiny. (crosses fingers) so CPU
     time in this, should be minimum.
     -Dianora
  */

  if(glines)
    {
      kill_list_ptr = last_list_ptr = glines;

      while(kill_list_ptr)
	{
	  if(kill_list_ptr->hold <= NOW)	/* a gline has expired */
	    {
	      if(glines == kill_list_ptr)
		{
		  /* Its pointing to first one in link list*/
		  /* so, bypass this one, remember bad things can happen
		     if you try to use an already freed pointer.. */

		  glines = last_list_ptr = tmp_list_ptr =
		    kill_list_ptr->next;
		}
	      else
		{
		  /* its in the middle of the list, so link around it */
		  tmp_list_ptr = last_list_ptr->next = kill_list_ptr->next;
		}

	      MyFree(kill_list_ptr->host);
	      MyFree(kill_list_ptr->name);
	      MyFree(kill_list_ptr->passwd);
	      MyFree(kill_list_ptr);
	      kill_list_ptr = tmp_list_ptr;
	    }
	  else
	    {
	      if( (kill_list_ptr->name && (!name || !match(kill_list_ptr->name,
                 name))) && (kill_list_ptr->host &&
                   (!host || !match(kill_list_ptr->host,host))))
		return(kill_list_ptr);
              last_list_ptr = kill_list_ptr;
              kill_list_ptr = kill_list_ptr->next;
	    }
	}
    }

  return((aConfItem *)NULL);
}

/* report_glines

inputs		- aClient pointer
output		- NONE
side effects	- 

Just like temp_klines, except these are kept separately...
		  
*/
void report_glines(aClient *sptr)
{
  aConfItem *kill_list_ptr;
  aConfItem *last_list_ptr;
  aConfItem *tmp_list_ptr;
  char *host;
  char *name;
  char *reason;

  expire_pending_glines();	/* This is not the g line list, but
				   the pending possible g line list */

  if(glines)
    {
      kill_list_ptr = last_list_ptr = glines;

      while(kill_list_ptr)
        {
	  if(kill_list_ptr->hold <= NOW)	/* gline has expired */
	    {
	      if(temporary_klines == kill_list_ptr)
		{
		  /* Its pointing to first one in link list*/
		  /* so, bypass this one, remember bad things can happen
		     if you try to use an already freed pointer.. */

		  glines = last_list_ptr = tmp_list_ptr =
		    kill_list_ptr->next;
		}
	      else
		{
		  /* its in the middle of the list, so link around it */
		  tmp_list_ptr = last_list_ptr->next = kill_list_ptr->next;
		}

	      MyFree(kill_list_ptr->host);
	      MyFree(kill_list_ptr->name);
	      MyFree(kill_list_ptr->passwd);
	      MyFree(kill_list_ptr);
	      kill_list_ptr = tmp_list_ptr;
	    }
	  else
	    {
	      if(kill_list_ptr->host)
		host = kill_list_ptr->host;
	      else
		host = "*";

	      if(kill_list_ptr->name)
		name = kill_list_ptr->name;
	      else
		name = "*";

	      if(kill_list_ptr->passwd)
		reason = kill_list_ptr->passwd;
	      else
		reason = "No Reason";

	      sendto_one(sptr,rpl_str(RPL_STATSKLINE), me.name,
			 sptr->name, 'G' , host, name, reason);

	      last_list_ptr = kill_list_ptr;
	      kill_list_ptr = kill_list_ptr->next;
	    }
        }
    }
}

/* add_gline

inputs		- pointer to aConfItem
output		- none
Side effects	- links in given aConfItem into gline link list

Identical to add_temp_kline code really.

-Dianora
*/

void add_gline(aConfItem *aconf)
{
  aconf->next = glines;
  glines = aconf;
}
#endif

/* report_temp_klines

inputs		- aClient pointer
output		- NONE
side effects	- 
		  
*/
void report_temp_klines(aClient *sptr)
{
  aConfItem *kill_list_ptr;
  aConfItem *last_list_ptr;
  aConfItem *tmp_list_ptr;
  char *host;
  char *name;
  char *reason;

  if(temporary_klines)
    {
      kill_list_ptr = last_list_ptr = temporary_klines;

      while(kill_list_ptr)
        {
	  if(kill_list_ptr->hold <= NOW)	/* kline has expired */
	    {
	      if(temporary_klines == kill_list_ptr)
		{
		  /* Its pointing to first one in link list*/
		  /* so, bypass this one, remember bad things can happen
		     if you try to use an already freed pointer.. */

		  temporary_klines = last_list_ptr = tmp_list_ptr =
		    kill_list_ptr->next;
		}
	      else
		{
		  /* its in the middle of the list, so link around it */
		  tmp_list_ptr = last_list_ptr->next = kill_list_ptr->next;
		}

	      MyFree(kill_list_ptr->host);
	      MyFree(kill_list_ptr->name);
	      MyFree(kill_list_ptr->passwd);
	      MyFree(kill_list_ptr);
	      kill_list_ptr = tmp_list_ptr;
	    }
	  else
	    {
	      if(kill_list_ptr->host)
		host = kill_list_ptr->host;
	      else
		host = "*";

	      if(kill_list_ptr->name)
		name = kill_list_ptr->name;
	      else
		name = "*";

	      if(kill_list_ptr->passwd)
		reason = kill_list_ptr->passwd;
	      else
		reason = "No Reason";

	      sendto_one(sptr,rpl_str(RPL_STATSKLINE), me.name,
			 sptr->name, 'k' , host, name, reason);

	      last_list_ptr = kill_list_ptr;
	      kill_list_ptr = kill_list_ptr->next;
	    }
        }
    }
}

/* urgh. consider this a place holder for a new version of R-lining
   to be done in next version of hybrid. don't use this. it will
   likely break... :-) -Dianora
*/ 

#ifdef R_LINES
/* find_restrict works against host/name and calls an outside program 
 * to determine whether a client is allowed to connect.  This allows 
 * more freedom to determine who is legal and who isn't, for example
 * machine load considerations.  The outside program is expected to 
 * return a reply line where the first word is either 'Y' or 'N' meaning 
 * "Yes Let them in" or "No don't let them in."  If the first word 
 * begins with neither 'Y' or 'N' the default is to let the person on.
 * It returns a value of 0 if the user is to be let through -Hoppie
 */
int	find_restrict(aClient *cptr)
{
  aConfItem *tmp;
  char	reply[80], temprpl[80];
  char	*rplhold = reply, *host, *name, *s;
  char	rplchar = 'Y';
  int	pi[2], rc = 0, n;
  
  if (!cptr->user)
    return 0;
  name = cptr->user->username;
  host = cptr->sockhost;
  Debug((DEBUG_INFO, "R-line check for %s[%s]", name, host));

  for (tmp = conf; tmp; tmp = tmp->next)
    {
      if (tmp->status != CONF_RESTRICT ||
	  (tmp->host && host && match(tmp->host, host)) ||
	  (tmp->name && name && match(tmp->name, name)))
	continue;

      if (BadPtr(tmp->passwd))
	{
	  sendto_ops("Program missing on R-line %s/%s, ignoring",
		     name, host);
	  continue;
	}

      if (pipe(pi) == -1)
	{
	  report_error("Error creating pipe for R-line %s:%s",
		       &me);
	  return 0;
	}
      switch (rc = vfork())
	{
	case -1 :
	  report_error("Error forking for R-line %s:%s", &me);
	  return 0;
	case 0 :
	  {
	    Reg	int	i;

	    (void)close(pi[0]);
	    for (i = 2; i < MAXCONNECTIONS; i++)
	      if (i != pi[1])
		(void)close(i);
	    if (pi[1] != 2)
	      (void)dup2(pi[1], 2);
	    (void)dup2(2, 1);
	    if (pi[1] != 2 && pi[1] != 1)
	      (void)close(pi[1]);
	    (void)execlp(tmp->passwd, tmp->passwd, name, host, 0);
	    exit(-1);
	  }
	default :
	  (void)close(pi[1]);
	  break;
	}
      *reply = '\0';
      (void)dgets(-1, NULL, 0); /* make sure buffer marked empty */
      while ((n = dgets(pi[0], temprpl, sizeof(temprpl)-1)) > 0)
	{
	  temprpl[n] = '\0';
	  if ((s = (char *)index(temprpl, '\n')))
	    *s = '\0';
	  if (strlen(temprpl) + strlen(reply) < sizeof(reply)-2)
	    (void)ircsprintf(rplhold, "%s %s", rplhold,
			     temprpl);
	  else
	    {
	      sendto_ops("R-line %s/%s: reply too long!",
			 name, host);
	      break;
	    }
	}
      (void)dgets(-1, NULL, 0); /* make sure buffer marked empty */
      (void)close(pi[0]);
      (void)kill(rc, SIGKILL); /* cleanup time */
      (void)wait(0);

      rc = 0;
      while (*rplhold == ' ')
	rplhold++;
      rplchar = *rplhold; /* Pull out the yes or no */
      while (*rplhold != ' ')
	rplhold++;
      while (*rplhold == ' ')
	rplhold++;
      (void)strcpy(reply,rplhold);
      rplhold = reply;
      
      if ((rc = (rplchar == 'n' || rplchar == 'N')))
	break;
    }
  if (rc)
    {
      sendto_one(cptr, ":%s %d %s :Restriction: %s",
		 me.name, ERR_YOUREBANNEDCREEP, cptr->name,
		 reply);
      return -1;
    }
  return 0;
}
#endif


/* check_time_interval
inputs		- comment field of k-line
output		- 1 if its NOT a time_interval or it is :-)
		  0 if its a time interval, but its not in the interval
side effects	-

There doesn't seem to be much need for this kind of kline anymore,
and it is a bit of a CPU hog. The previous version of hybrid only supported
it if you didn't have KLINE_COMMENT_ONLY....
It still needs some rethinking/recoding.. but not today...
In most cases, its not needed, esp. on a busy server. don't
#define TIMED_KLINES, if you ban them on one server, they'll just
use another server. Just doesn't make sense to me any more.

-Dianora
*/

#ifdef TIMED_KLINES
/*
** check against a set of time intervals
*/

static int check_time_interval(char *interval)
{
  struct tm *tptr;
  time_t tick;
  char	 *p;
  int	 perm_min_hours, perm_min_minutes;
  int    perm_max_hours, perm_max_minutes;
  int    now, perm_min, perm_max;
  /*   char   reply[512];	*//* BLAH ! */

  tick = timeofday;
  tptr = localtime(&tick);
  now = tptr->tm_hour * 60 + tptr->tm_min;

  while (interval)
    {
      p = (char *)index(interval, ',');
      if (p)
	*p = '\0';
      if (sscanf(interval, "%2d%2d-%2d%2d",
		 &perm_min_hours, &perm_min_minutes,
		 &perm_max_hours, &perm_max_minutes) != 4)
	{
	  if (p)
	    *p = ',';
	  return(1);	/* Not an interval, let the caller deal with it */
	}
      if (p)
	*(p++) = ',';
      perm_min = 60 * perm_min_hours + perm_min_minutes;
      perm_max = 60 * perm_max_hours + perm_max_minutes;
      /*
      ** The following check allows intervals over midnight ...
      */
      if ((perm_min < perm_max)
	  ? (perm_min <= now && now <= perm_max)
	  : (perm_min <= now || now <= perm_max))
	{
	  /*
	  (void)ircsprintf(reply,
			   ":%%s %%d %%s :%s %d:%02d to %d:%02d.",
			   "You are not allowed to connect from",
			   perm_min_hours, perm_min_minutes,
			   perm_max_hours, perm_max_minutes);
			   */
	  return 1;

	  /*	  return(ERR_YOUREBANNEDCREEP); */
	}
      if ((perm_min < perm_max)
	  ? (perm_min <= now + 5 && now + 5 <= perm_max)
	  : (perm_min <= now + 5 || now + 5 <= perm_max))
	{
	  /*
	  (void)ircsprintf(reply, ":%%s %%d %%s :%d minute%s%s",
			   perm_min-now,(perm_min-now)>1?"s ":" ",
			   "and you will be denied for further access");
			   */
	  return 1;
	  /* return(ERR_YOUWILLBEBANNED); */
	}
      interval = p;
    }
  return 0;
}
#endif
