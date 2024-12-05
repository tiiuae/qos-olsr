/*
 * The olsr.org Optimized Link-State Routing daemon (olsrd)
 *
 * (c) by the OLSR project
 *
 * See our Git repository to find out who worked on this file
 * and thus is a copyright holder on it.
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * * Redistributions of source code must retain the above copyright
 *   notice, this list of conditions and the following disclaimer.
 * * Redistributions in binary form must reproduce the above copyright
 *   notice, this list of conditions and the following disclaimer in
 *   the documentation and/or other materials provided with the
 *   distribution.
 * * Neither the name of olsr.org, olsrd nor the names of its
 *   contributors may be used to endorse or promote products derived
 *   from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 * FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 * BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 * ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Visit http://www.olsr.org for more information.
 *
 * If you find this software useful feel free to make a donation
 * to the project. For more information see the website or contact
 * the copyright holders.
 *
 */

#include "ipcalc.h"
#include "defs.h"
#include "mpr_selector_set.h"
#include "olsr.h"
#include "scheduler.h"
#include "net_olsr.h"

static uint16_t ansn;

/* MPR selector list */
static struct mpr_selector mprs_list;
static struct mpr_selector heads_list;

/**
 *Initialize MPR selector set
 */

void
olsr_init_mprs_set(void)
{
  OLSR_PRINTF(5, "MPRS: Init\n");

  /* Initial values */
  ansn = 0;

  mprs_list.next = &mprs_list;
  mprs_list.prev = &mprs_list;
  //update
  heads_list.next = &heads_list;
  heads_list.prev = &heads_list;
}

uint16_t
get_local_ansn(void)
{
  return ansn;
}

void
increase_local_ansn(void)
{
  ansn++;
}

#if 0

/**
 * Check if we(this node) is selected as a MPR by any
 * neighbors. If the list is empty we are not MPR.
 */
bool
olsr_is_mpr(void)
{
  return ((mprs_list.next == &mprs_list) ? false : true);
}
#endif /* 0 */

/**
 * Wrapper for the timer callback.
 */
static void
olsr_expire_mpr_sel_entry(void *context)
{
#ifdef DEBUG
  struct ipaddr_str buf;
#endif /* DEBUG */
  struct mpr_selector *mpr_sel;

  mpr_sel = (struct mpr_selector *)context;
  mpr_sel->MS_timer = NULL;

#ifdef DEBUG
  OLSR_PRINTF(1, "MPRS: Timing out %st\n", olsr_ip_to_string(&buf, &mpr_sel->MS_main_addr));
#endif /* DEBUG */

  DEQUEUE_ELEM(mpr_sel);

  /* Delete entry */
  free(mpr_sel);
  signal_link_changes(true);
  
  //update
  // is the node still a head?
  if(olsr_cnf->is_head == 2 ||olsr_cnf->is_head == 3)
  {
    if(mprs_list.next == &mprs_list)
    {
      OLSR_PRINTF(1,"my MPR Selectors Set is empty and this I am no more a head");
      olsr_cnf->is_head = olsr_cnf->is_head-2;
    }
  }
  
}

static void
olsr_expire_head_sel_entry(void *context)
{
#ifdef DEBUG
  struct ipaddr_str buf;
#endif /* DEBUG */
  struct mpr_selector *mpr_sel;

  mpr_sel = (struct mpr_selector *)context;
  mpr_sel->MS_timer = NULL;

#ifdef DEBUG
  OLSR_PRINTF(1, "Head: Timing out %st\n", olsr_ip_to_string(&buf, &mpr_sel->MS_main_addr));
#endif /* DEBUG */

  DEQUEUE_ELEM(mpr_sel);

  /* Delete entry */
  free(mpr_sel);
  signal_link_changes(true);
  
  //update
  // is the node still a head?
  if(olsr_cnf->is_head == 2 ||olsr_cnf->is_head == 3)
  {
	if(heads_list.next == &heads_list)
	{
		OLSR_PRINTF(1,"My head Selectors Set is empty and this I am no more a head");
		olsr_cnf->is_head = olsr_cnf->is_head-2;
	}
  }
  
}
/**
 * Set the mpr selector expiration timer.
 *
 * all timer setting shall be done using this function.
 * The timer param is a relative timer expressed in milliseconds.
 */
static void
olsr_set_mpr_sel_timer(struct mpr_selector *mpr_sel, olsr_reltime rel_timer)
{

  olsr_set_timer(&mpr_sel->MS_timer, rel_timer, OLSR_MPR_SEL_JITTER, OLSR_TIMER_ONESHOT, &olsr_expire_mpr_sel_entry, mpr_sel, 0);
}

static void
olsr_set_head_sel_timer(struct mpr_selector *mpr_sel, olsr_reltime rel_timer)
{

  olsr_set_timer(&mpr_sel->MS_timer, rel_timer, OLSR_MPR_SEL_JITTER, OLSR_TIMER_ONESHOT, &olsr_expire_head_sel_entry, mpr_sel, 0);
}

/**
 *Add a MPR selector to the MPR selector set
 *
 *@param addr address of the MPR selector
 *@param vtime validity time for the new entry
 *
 *@return a pointer to the new entry
 */
struct mpr_selector *
olsr_add_mpr_selector(const union olsr_ip_addr *addr, olsr_reltime vtime)
{
  struct ipaddr_str buf;
  struct mpr_selector *new_entry;

  OLSR_PRINTF(1, "MPRS: adding %s\n", olsr_ip_to_string(&buf, addr));

  new_entry = olsr_malloc(sizeof(struct mpr_selector), "Add MPR selector");
  /* Fill struct */
  new_entry->MS_main_addr = *addr;
  olsr_set_mpr_sel_timer(new_entry, vtime);
  /* Queue */
  QUEUE_ELEM(mprs_list, new_entry);
  

  // update 
  // change the status of is_head if a normal node is an mpr selector.
  
  /*
     new_entry->prev = &mprs_list;
     new_entry->next = mprs_list.next;
     mprs_list.next->prev = new_entry;
     mprs_list.next = new_entry;
   */
  return new_entry;
}



struct mpr_selector *
olsr_add_head_selector(const union olsr_ip_addr *addr, olsr_reltime vtime)
{
  struct ipaddr_str buf;
  struct mpr_selector *new_entry;

  OLSR_PRINTF(1, "Head: adding %s\n", olsr_ip_to_string(&buf, addr));

  new_entry = olsr_malloc(sizeof(struct mpr_selector), "Add MPR selector");
  /* Fill struct */
  new_entry->MS_main_addr = *addr;
  olsr_set_head_sel_timer(new_entry, vtime);
  /* Queue */
  QUEUE_ELEM(heads_list, new_entry);
  

  // update 
  // change the status of is_head if a normal node is an mpr selector.
  
  /*
     new_entry->prev = &mprs_list;
     new_entry->next = mprs_list.next;
     mprs_list.next->prev = new_entry;
     mprs_list.next = new_entry;
   */
  return new_entry;
}
/**
 *Lookup an entry in the MPR selector table
 *based on address
 *
 *@param addr the addres to check for
 *
 *@return a pointer to the entry or NULL
 */
struct mpr_selector *
olsr_lookup_mprs_set(const union olsr_ip_addr *addr)
{
  struct mpr_selector *mprs;

  if (addr == NULL)
    return NULL;
  //OLSR_PRINTF(1, "MPRS: Lookup....");

  for (mprs = mprs_list.next; mprs != &mprs_list; mprs = mprs->next) {
    if (ipequal(&mprs->MS_main_addr, addr)) {
      //OLSR_PRINTF(1, "MATCH\n");
      return mprs;
    }
  }
  //OLSR_PRINTF(1, "NO MACH\n");
  return NULL;
}


struct mpr_selector *
olsr_lookup_heads_set(const union olsr_ip_addr *addr)
{
  struct mpr_selector *mprs;

  if (addr == NULL)
    return NULL;
  //OLSR_PRINTF(1, "MPRS: Lookup....");

  for (mprs = heads_list.next; mprs != &heads_list; mprs = mprs->next) {
    if (ipequal(&mprs->MS_main_addr, addr)) {
      //OLSR_PRINTF(1, "MATCH\n");
      return mprs;
    }
  }
  //OLSR_PRINTF(1, "NO MACH\n");
  return NULL;
}
bool olsr_get_mpr_sel_set_size(){

if(mprs_list.next == &mprs_list)
	{
		// OLSR_PRINTF(1,"my MPR Selectors Set is empty");
		return false;
	}
else{
		OLSR_PRINTF(1,"MPR Selectors Set is not empty");
    return true;
  }
}


bool olsr_get_head_sel_set_size(){
if(heads_list.next == &heads_list)
	{
		OLSR_PRINTF(1,"My head Selectors Set is empty and this I am no more a head");
		return false;
    //olsr_cnf->is_head = 0;
	}
else{
		OLSR_PRINTF(1,"MPR Selectors Set is not empty");
    return true;
  }
}
/**
 *Update a MPR selector entry or create an new
 *one if it does not exist
 *
 *@param addr the address of the MPR selector
 *@param vtime tha validity time of the entry
 *
 *@return 1 if a new entry was added 0 if not
 */
int
olsr_update_mprs_set(const union olsr_ip_addr *addr, olsr_reltime vtime)
{
  struct ipaddr_str buf;
  struct mpr_selector *mprs = olsr_lookup_mprs_set(addr);

  OLSR_PRINTF(1, "MPRS: Update %s\n", olsr_ip_to_string(&buf, addr));

  if (mprs == NULL) {
    olsr_add_mpr_selector(addr, vtime);
    signal_link_changes(true);
    return 1;
  }
  olsr_set_mpr_sel_timer(mprs, vtime);
  return 0;
}


int
olsr_update_heads_set(const union olsr_ip_addr *addr, olsr_reltime vtime)
{
  struct ipaddr_str buf;
  struct mpr_selector *mprs = olsr_lookup_heads_set(addr);

  OLSR_PRINTF(1, "Heads: Update %s\n", olsr_ip_to_string(&buf, addr));

  if (mprs == NULL) {
    olsr_add_head_selector(addr, vtime);
    signal_link_changes(true);
    return 1;
  }
  // update
  olsr_set_head_sel_timer(mprs, vtime);
  return 0;
}

void
olsr_print_mprs_set(void)
{
  struct mpr_selector *mprs;
  OLSR_PRINTF(1, "MPR SELECTORS: ");
  for (mprs = mprs_list.next; mprs != &mprs_list; mprs = mprs->next) {
    struct ipaddr_str buf;
    OLSR_PRINTF(1, "%s ", olsr_ip_to_string(&buf, &mprs->MS_main_addr));
  }
  OLSR_PRINTF(1, "\n");
}


void
olsr_print_heads_set(void)
{
  struct mpr_selector *mprs;
  OLSR_PRINTF(1, "HEAD SELECTORS: ");
  for (mprs = heads_list.next; mprs != &heads_list; mprs = mprs->next) {
    struct ipaddr_str buf;
    OLSR_PRINTF(1, "%s ", olsr_ip_to_string(&buf, &mprs->MS_main_addr));
  }
  OLSR_PRINTF(1, "\n");
}

bool 
is_mprs_empty(void)
{
struct mpr_selector *mprs;

for (mprs = mprs_list.next; mprs != &mprs_list; mprs = mprs->next) {

return false;
}

return true;
}

#if 0

/**
 *Print the current MPR selector set to STDOUT
 */
void
olsr_print_mprs_set(void)
{
  struct mpr_selector *mprs;
  OLSR_PRINTF(1, "MPR SELECTORS: ");
  for (mprs = mprs_list.next; mprs != &mprs_list; mprs = mprs->next) {
    struct ipaddr_str buf;
    OLSR_PRINTF(1, "%s ", olsr_ip_to_string(&buf, &mprs->MS_main_addr));
  }
  OLSR_PRINTF(1, "\n");
}
#endif /* 0 */

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
