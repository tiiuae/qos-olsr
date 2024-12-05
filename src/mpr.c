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
#include "mpr.h"
#include "two_hop_neighbor_table.h"
#include "olsr.h"
#include "neighbor_table.h"
#include "scheduler.h"
#include "net_olsr.h"


/* Begin:
 * Prototypes for internal functions
 */

static uint16_t add_will_always_nodes(void);

static void olsr_optimize_mpr_set(void);

static void olsr_clear_mprs(void);

static void olsr_clear_two_hop_processed(void);

static struct neighbor_entry *olsr_find_maximum_covered(int);

static uint16_t olsr_calculate_two_hop_neighbors(void);

static int olsr_check_mpr_changes(void);

static int olsr_chosen_mpr(struct neighbor_entry *, uint16_t *);

static struct neighbor_2_list_entry *olsr_find_2_hop_neighbors_with_1_link(int);

/* End:
 * Prototypes for internal functions
 */

/**
 *Find all 2 hop neighbors with 1 link
 *connecting them to us trough neighbors
 *with a given willingness.
 *
 *@param willingness the willigness of the neighbors
 *
 *@return a linked list of allocated neighbor_2_list_entry structures
 */
static struct neighbor_2_list_entry *
olsr_find_2_hop_neighbors_with_1_link(int willingness)
{

  uint8_t idx;
  struct neighbor_2_list_entry *two_hop_list_tmp = NULL;
  struct neighbor_2_list_entry *two_hop_list = NULL;
  struct neighbor_entry *dup_neighbor;
  struct neighbor_2_entry *two_hop_neighbor = NULL;

  for (idx = 0; idx < HASHSIZE; idx++) {

    for (two_hop_neighbor = two_hop_neighbortable[idx].next; two_hop_neighbor != &two_hop_neighbortable[idx];
         two_hop_neighbor = two_hop_neighbor->next) {

      //two_hop_neighbor->neighbor_2_state=0;
      //two_hop_neighbor->mpr_covered_count = 0;

      dup_neighbor = olsr_lookup_neighbor_table(&two_hop_neighbor->neighbor_2_addr);

      if ((dup_neighbor != NULL) && (dup_neighbor->status != NOT_SYM)) {

        //OLSR_PRINTF(1, "(1)Skipping 2h neighbor %s - already 1hop\n", olsr_ip_to_string(&buf, &two_hop_neighbor->neighbor_2_addr));

        continue;
      }

      if (two_hop_neighbor->neighbor_2_pointer == 1) {
        if ((two_hop_neighbor->neighbor_2_nblist.next->neighbor->willingness == willingness)
            && (two_hop_neighbor->neighbor_2_nblist.next->neighbor->status == SYM)) {
          two_hop_list_tmp = olsr_malloc(sizeof(struct neighbor_2_list_entry), "MPR two hop list");

          //OLSR_PRINTF(1, "ONE LINK ADDING %s\n", olsr_ip_to_string(&buf, &two_hop_neighbor->neighbor_2_addr));

          /* Only queue one way here */
          two_hop_list_tmp->neighbor_2 = two_hop_neighbor;

          two_hop_list_tmp->next = two_hop_list;

          two_hop_list = two_hop_list_tmp;
        }
      }

    }

  }

  return (two_hop_list_tmp);
}

/**
 *This function processes the chosen MPRs and updates the counters
 *used in calculations
 */
static int
olsr_chosen_mpr(struct neighbor_entry *one_hop_neighbor, uint16_t * two_hop_covered_count)
{
  struct neighbor_list_entry *the_one_hop_list;
  struct neighbor_2_list_entry *second_hop_entries;
  struct neighbor_entry *dup_neighbor;
  uint16_t count;
  struct ipaddr_str buf;
  count = *two_hop_covered_count;

  OLSR_PRINTF(1, "Setting %s as MPR\n", olsr_ip_to_string(&buf, &one_hop_neighbor->neighbor_main_addr));

  //printf("PRE COUNT: %d\n\n", count);

  // this 
  one_hop_neighbor->is_mpr = true;      //NBS_MPR;

  for (second_hop_entries = one_hop_neighbor->neighbor_2_list.next; second_hop_entries != &one_hop_neighbor->neighbor_2_list;
       second_hop_entries = second_hop_entries->next) {
       // update skip any 2 hop neighbor that is not a
      /* if(!second_hop_entries->neighbor_2->is_head){
       	continue;
       }*/
    dup_neighbor = olsr_lookup_neighbor_table(&second_hop_entries->neighbor_2->neighbor_2_addr);

    if ((dup_neighbor != NULL) && (dup_neighbor->status == SYM)) {
      //OLSR_PRINTF(7, "(2)Skipping 2h neighbor %s - already 1hop\n", olsr_ip_to_string(&buf, &second_hop_entries->neighbor_2->neighbor_2_addr));
      continue;
    }
    //      if(!second_hop_entries->neighbor_2->neighbor_2_state)
    //if(second_hop_entries->neighbor_2->mpr_covered_count < olsr_cnf->mpr_coverage)
    //{
    /*
       Now the neighbor is covered by this mpr
     */
     // update how many mprs are covering this two hop neighbor
    second_hop_entries->neighbor_2->mpr_covered_count++;
    the_one_hop_list = second_hop_entries->neighbor_2->neighbor_2_nblist.next;

    //OLSR_PRINTF(1, "[%s](%x) has coverage %d\n", olsr_ip_to_string(&buf, &second_hop_entries->neighbor_2->neighbor_2_addr), second_hop_entries->neighbor_2, second_hop_entries->neighbor_2->mpr_covered_count);

    // count is updated if the covering mprs is above the redundency threshold
    if (second_hop_entries->neighbor_2->mpr_covered_count >= olsr_cnf->mpr_coverage)
      count++;

    while (the_one_hop_list != &second_hop_entries->neighbor_2->neighbor_2_nblist) {

      if (the_one_hop_list->neighbor->status == SYM) {
        if (second_hop_entries->neighbor_2->mpr_covered_count >= olsr_cnf->mpr_coverage) {
          the_one_hop_list->neighbor->neighbor_2_nocov--;
        }
      }
      the_one_hop_list = the_one_hop_list->next;
    }

    //}
  }

  //printf("POST COUNT %d\n\n", count);

  *two_hop_covered_count = count;
  return count;

}

/**
 *Find the neighbor that covers the most 2 hop neighbors
 *with a given willingness
 *
 *@param willingness the willingness of the neighbor
 *
 *@return a pointer to the neighbor_entry struct
 */
static struct neighbor_entry *
olsr_find_maximum_covered(int willingness)
{
  uint16_t maximum;
  struct neighbor_entry *a_neighbor;
  struct neighbor_entry *mpr_candidate = NULL;

  maximum = 0;

  OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {

#if 0
    printf("[%s] nocov: %d mpr: %d will: %d max: %d\n\n", olsr_ip_to_string(&buf, &a_neighbor->neighbor_main_addr),
           a_neighbor->neighbor_2_nocov, a_neighbor->is_mpr, a_neighbor->willingness, maximum);
#endif /* 0 */

    if ((!a_neighbor->is_mpr) && (a_neighbor->willingness == willingness) && (maximum < a_neighbor->neighbor_2_nocov)) {

      maximum = a_neighbor->neighbor_2_nocov;
      mpr_candidate = a_neighbor;
    }
  }
  OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);

  return mpr_candidate;
}

/**
 *Remove all MPR registrations
 */
static void
olsr_clear_mprs(void)
{
  struct neighbor_entry *a_neighbor;
  struct neighbor_2_list_entry *two_hop_list;

  OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {

    /* Clear MPR selection. */
    if (a_neighbor->is_mpr) {
      a_neighbor->was_mpr = true;
      a_neighbor->is_mpr = false;
	    // update
      //a_neighbor->my_head = false;
    }

    /* Clear two hop neighbors coverage count/ */
    for (two_hop_list = a_neighbor->neighbor_2_list.next; two_hop_list != &a_neighbor->neighbor_2_list;
         two_hop_list = two_hop_list->next) {
      two_hop_list->neighbor_2->mpr_covered_count = 0;
    }

  }
  OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);
}

/**
 *Check for changes in the MPR set
 *
 *@return 1 if changes occured 0 if not
 */
static int
olsr_check_mpr_changes(void)
{
  struct neighbor_entry *a_neighbor;
  int retval;

  retval = 0;

  OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {

    if (a_neighbor->was_mpr) {
      a_neighbor->was_mpr = false;

      if (!a_neighbor->is_mpr) {
        retval = 1;
      }
    }
  }
  OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);

  return retval;
}

/**
 *Clears out proccess registration
 *on two hop neighbors
 */
static void
olsr_clear_two_hop_processed(void)
{
  int idx;

  for (idx = 0; idx < HASHSIZE; idx++) {
    struct neighbor_2_entry *neighbor_2;
    for (neighbor_2 = two_hop_neighbortable[idx].next; neighbor_2 != &two_hop_neighbortable[idx]; neighbor_2 = neighbor_2->next) {
      /* Clear */
      neighbor_2->processed = 0;
    }
  }

}

/**
 *This function calculates the number of two hop neighbors
 */
static uint16_t
olsr_calculate_two_hop_neighbors(void)
{
  struct neighbor_entry *a_neighbor, *dup_neighbor;
  struct neighbor_2_list_entry *twohop_neighbors;
  uint16_t count = 0;
  uint16_t n_count = 0;
  uint16_t sum = 0;

  /* Clear 2 hop neighs */
  olsr_clear_two_hop_processed();

  OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {

    if (a_neighbor->status == NOT_SYM) {
      a_neighbor->neighbor_2_nocov = count;
      continue;
    }

    for (twohop_neighbors = a_neighbor->neighbor_2_list.next; twohop_neighbors != &a_neighbor->neighbor_2_list;
         twohop_neighbors = twohop_neighbors->next) {

      dup_neighbor = olsr_lookup_neighbor_table(&twohop_neighbors->neighbor_2->neighbor_2_addr);

      if ((dup_neighbor == NULL) || (dup_neighbor->status != SYM)) {
        n_count++;
        // update && (twohop_neighbors->neighbor_2->is_head)
        if (!twohop_neighbors->neighbor_2->processed ) {
          count++;
          twohop_neighbors->neighbor_2->processed = 1;
        }
      }
    }
    a_neighbor->neighbor_2_nocov = n_count;

    /* Add the two hop count */
    // update to debug this value and n_count
    sum += count;

  }
  OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);

  OLSR_PRINTF(3, "Two hop neighbors: %d\n", sum);
  return sum;
}

/**
 * Adds all nodes with willingness set to WILL_ALWAYS
 */
static uint16_t
add_will_always_nodes(void)
{
  struct neighbor_entry *a_neighbor;
  uint16_t count = 0;

#if 0
  printf("\nAdding WILL ALWAYS nodes....\n");
#endif /* 0 */

  OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {
    struct ipaddr_str buf;
    if ((a_neighbor->status == NOT_SYM) || (a_neighbor->willingness != WILL_ALWAYS)) {
      continue;
    }
    olsr_chosen_mpr(a_neighbor, &count);

    OLSR_PRINTF(3, "Adding WILL_ALWAYS: %s\n", olsr_ip_to_string(&buf, &a_neighbor->neighbor_main_addr));

  }
  OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);

#if 0
  OLSR_PRINTF(1, "Count: %d\n", count);
#endif /* 0 */
  return count;
}

/**
 *This function calculates the mpr neighbors
 *@return nada
 */
void
olsr_calculate_mpr(void)
{
  if(olsr_cnf->is_head==0 ){
  	return;
  }
  
  uint16_t two_hop_covered_count;
  uint16_t two_hop_count;
  int i;

  OLSR_PRINTF(1, "\n**RECALCULATING MPR**\n\n");

  olsr_clear_mprs();
  two_hop_count = olsr_calculate_two_hop_neighbors(); //count the number of 2-hop heads
  two_hop_covered_count = add_will_always_nodes(); 

  /*
   *Calculate MPRs based on WILLINGNESS
   */

  for (i = WILL_ALWAYS - 1; i > WILL_NEVER; i--) {

    struct neighbor_entry *mprs;
 
    struct neighbor_2_list_entry *two_hop_list = olsr_find_2_hop_neighbors_with_1_link(i);

    while (two_hop_list != NULL) {
      struct neighbor_2_list_entry *tmp;
      //printf("CHOSEN FROM 1 LINK\n");
      if (!two_hop_list->neighbor_2->neighbor_2_nblist.next->neighbor->is_mpr)
        olsr_chosen_mpr(two_hop_list->neighbor_2->neighbor_2_nblist.next->neighbor, &two_hop_covered_count);
      tmp = two_hop_list;
      two_hop_list = two_hop_list->next;;
      free(tmp);
    }

    if (two_hop_covered_count >= two_hop_count) {
      i = WILL_NEVER;
      break;
    }
    //printf("two hop covered count: %d\n", two_hop_covered_count);

    while ((mprs = olsr_find_maximum_covered(i)) != NULL) {
      //printf("CHOSEN FROM MAXCOV\n");
      olsr_chosen_mpr(mprs, &two_hop_covered_count);

      if (two_hop_covered_count >= two_hop_count) {
        i = WILL_NEVER;
        break;
      }

    }
  }

  /*
     increment the mpr sequence number
   */
  //neighbortable.neighbor_mpr_seq++;

  /* Optimize selection */
  olsr_optimize_mpr_set();

  if (olsr_check_mpr_changes()) {
    OLSR_PRINTF(3, "CHANGES IN MPR SET\n");
    if (olsr_cnf->tc_redundancy > 0)
      signal_link_changes(true);
  }
  
  struct neighbor_entry *a_neighbor;
   OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {

    if(a_neighbor->my_head)
    {
    	a_neighbor->is_mpr = true;
    }

  }
  OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);

}


// update 
//find head when the node joins in the middle of the calculation period or when it loses its head
void olsr_find_head(void){
	OLSR_PRINTF(1,"Finding Existing Cluster head\n");
  //problem: the previous disconnected head needs its is_mpr and is_head status set to false here

	struct neighbor_entry *a_neighbor, *temp_neighbor;
	//uint16_t max_qos = olsr_cnf->qos;
	//union olsr_ip_addr max_ip = olsr_cnf->main_addr;
	uint16_t max_qos = 0;
	union olsr_ip_addr max_ip ;
  struct ipaddr_str buf;
	bool head_found=false;
  // loop through all neighbor entries 
	 OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {
    // if the neighbor is not a head continue
		 if(a_neighbor->is_head==0 || a_neighbor->status == NOT_SYM){
      //update: changing the is_mpr and is_head status of the previous disconnected head
      if(a_neighbor->is_head){
      a_neighbor->is_mpr = false;
      a_neighbor->my_head = false;
      }
      //update end
      continue;
		 }

    head_found=true;
		 OLSR_PRINTF(1,"Head Neighbor: %s with qos: %d \n", olsr_ip_to_string(&buf, &a_neighbor->neighbor_main_addr),a_neighbor->qos);
		// if the neighbor is a head then check their qos 
		if (a_neighbor->qos >= max_qos) {
		  max_qos = a_neighbor->qos;
		  max_ip = a_neighbor->neighbor_main_addr;
		}

	}
	OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);
	// if(max_qos==0)
	// return;
	 OLSR_PRINTF(1,"For node: %s with qos: %d ", olsr_ip_to_string(&buf, &olsr_cnf->main_addr),olsr_cnf->qos);

  if(head_found){
      // if selected neighbor is in the neighbor list, set their mpr and head status 
      temp_neighbor = olsr_lookup_neighbor_table(&max_ip);
      //cout<<olsr_cnf->main_addr<<" my qos is "<<olsr_cnf->qos<<", ";
      if(temp_neighbor != NULL)
      {
        struct ipaddr_str buf1;
        OLSR_PRINTF(1," new head is %s with qos: %d ", olsr_ip_to_string(&buf1, &temp_neighbor->neighbor_main_addr),temp_neighbor->qos);
        temp_neighbor->is_mpr = true;
        temp_neighbor->my_head = true;
      }

  }
  else{
    olsr_calculate_head();
  }
  /*if(olsr_cnf->qos==max_qos && olsr_cnf->is_head !=1 && olsr_cnf->is_head <3){
			olsr_cnf->is_head =olsr_cnf->is_head+1;
  }
    if(olsr_cnf->is_head ==1){
  olsr_calculate_lq_mpr();
  }
*/  
}

//head calculation
void
olsr_calculate_head(void){
	// find the neighbor with max qos
	// set the max qos neighbor as mpr.
	OLSR_PRINTF(1,"Calculating Cluster head\n my qos is %d\n",olsr_cnf->qos);
	
	struct neighbor_entry *a_neighbor, *temp_neighbor,*temp_neighbor2;
	uint16_t max_qos = olsr_cnf->qos;
	uint16_t max_prev_head_qos = 0;
	union olsr_ip_addr max_ip = olsr_cnf->main_addr;
	 struct ipaddr_str buf;
	 int max_num_neighbor = olsr_cnf->neighnum;


	 union olsr_ip_addr prev_head_ip = olsr_cnf->main_addr;
	 // update
	 if( olsr_cnf->is_head == 1 || olsr_cnf->is_head == 3 ){
		 prev_head_ip=olsr_cnf->main_addr;
		 max_prev_head_qos =  olsr_cnf->qos;
	 }	 

	 OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {
      struct link_entry *lnk = get_best_link_to_neighbor(&a_neighbor->neighbor_main_addr);
      if (lnk) {
        /*if((lnk->linkcost/1024.0) > 13)
        {
          continue;
        }*/

        //update: this is creating a problem
        //update: The is_mpr and is_head of the not symetric node should be also changed
        if(a_neighbor->status == NOT_SYM){
          //changes start:
          a_neighbor->is_mpr = false;
          a_neighbor->my_head = false;
          //changes end:
          continue;
        }

        
        if(a_neighbor->my_head){
          prev_head_ip=a_neighbor->neighbor_main_addr;
          max_prev_head_qos=a_neighbor->qos;	 
        }
        struct ipaddr_str buf3;
        OLSR_PRINTF(1,"neighbor %s has QoS %d \n",olsr_ip_to_string(&buf3,&a_neighbor->neighbor_main_addr),a_neighbor->qos);
        a_neighbor->is_mpr = false;
        a_neighbor->my_head = false;
        
        if (a_neighbor->qos > max_qos) {
          max_qos = a_neighbor->qos;
          max_ip = a_neighbor->neighbor_main_addr;
        }
      }
	}
	OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);

	if(max_qos==0)
	  return;
  struct ipaddr_str buf2;
  OLSR_PRINTF(1,"max qos neighbor is: %s with qos: %d\n",olsr_ip_to_string(&buf2,&max_ip),max_qos);
  // check if previous head still exists 
	 temp_neighbor2 = olsr_lookup_neighbor_table(&prev_head_ip);

	 if(temp_neighbor2 != NULL || olsr_cnf->is_head == 1 || olsr_cnf->is_head == 3)
	 {
	 	
 		if((max_qos)*0.9 <= max_prev_head_qos){
 					OLSR_PRINTF(1,"No head change");
 			 if(temp_neighbor2 != NULL){
 			  temp_neighbor2->is_mpr = true;
	 			temp_neighbor2->my_head = true;
 			 }

			return;
		}
	}
	 
	
  temp_neighbor = olsr_lookup_neighbor_table(&max_ip);
	
  if(temp_neighbor != NULL)
  {
  struct ipaddr_str buf1;
    OLSR_PRINTF(1," new head is %s with qos: %d ", olsr_ip_to_string(&buf1, &temp_neighbor->neighbor_main_addr),temp_neighbor->qos);
    temp_neighbor->is_mpr = true;
    temp_neighbor->my_head = true;
  }

  if(olsr_cnf->qos==max_qos && olsr_cnf->is_head !=1 && olsr_cnf->is_head <3){
			olsr_cnf->is_head =olsr_cnf->is_head+1;
  }

  // if i am not min qos and i previously set myself as head,  
  //
  if((olsr_cnf->qos!=max_qos && olsr_cnf->qos==max_prev_head_qos) && (olsr_cnf->is_head == 1||olsr_cnf->is_head == 3)){
    olsr_cnf->is_head =olsr_cnf->is_head-1;
  }

  // if(olsr_cnf->is_head ==1){
  // olsr_calculate_lq_mpr();
  // }

}


/**
 *Optimize MPR set by removing all entries
 *where all 2 hop neighbors actually is
 *covered by enough MPRs already
 *Described in RFC3626 section 8.3.1
 *point 5
 *
 *@return nada
 */
static void
olsr_optimize_mpr_set(void)
{
  struct neighbor_entry *a_neighbor, *dup_neighbor;
  struct neighbor_2_list_entry *two_hop_list;
  int i, removeit;

#if 0
  printf("\n**MPR OPTIMIZING**\n\n");
#endif /* 0 */

  for (i = WILL_NEVER + 1; i < WILL_ALWAYS; i++) {

    OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {

      if (a_neighbor->willingness != i) {
        continue;
      }

      if (a_neighbor->is_mpr) {
        removeit = 1;

        for (two_hop_list = a_neighbor->neighbor_2_list.next; two_hop_list != &a_neighbor->neighbor_2_list;
             two_hop_list = two_hop_list->next) {

          dup_neighbor = olsr_lookup_neighbor_table(&two_hop_list->neighbor_2->neighbor_2_addr);

          if ((dup_neighbor != NULL) && (dup_neighbor->status != NOT_SYM)) {
            continue;
          }
          //printf("\t[%s] coverage %d\n", olsr_ip_to_string(&buf, &two_hop_list->neighbor_2->neighbor_2_addr), two_hop_list->neighbor_2->mpr_covered_count);
          /* Do not remove if we find a entry which need this MPR */
          if (two_hop_list->neighbor_2->mpr_covered_count <= olsr_cnf->mpr_coverage) {
            removeit = 0;
          }
        }

        if (removeit) {
          struct ipaddr_str buf;
          OLSR_PRINTF(3, "MPR OPTIMIZE: removiong mpr %s\n\n", olsr_ip_to_string(&buf, &a_neighbor->neighbor_main_addr));
          a_neighbor->is_mpr = false;
        }
      }
    } OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);
  }
}

#ifndef NODEBUG
void
olsr_print_mpr_set(void)
{
  /* The whole function makes no sense without it. */
  struct neighbor_entry *a_neighbor;

  OLSR_PRINTF(1, "MPR SET: ");

  OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {

    /*
     * Remove MPR settings
     */
    if (a_neighbor->is_mpr) {
      struct ipaddr_str buf;
      OLSR_PRINTF(1, "[%s] ", olsr_ip_to_string(&buf, &a_neighbor->neighbor_main_addr));
    }
  } OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);

  OLSR_PRINTF(1, "\n");
}
#endif /* NODEBUG */

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
