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

#include "process_package.h"
#include "ipcalc.h"
#include "defs.h"
#include "lq_packet.h"
#include "hysteresis.h"
#include "two_hop_neighbor_table.h"
#include "tc_set.h"
#include "mpr_selector_set.h"
#include "mid_set.h"
#include "olsr.h"
#include "parser.h"
#include "duplicate_set.h"
#include "scheduler.h"
#include "net_olsr.h"
#include "lq_plugin.h"
#include "log.h"

#include <assert.h>
#include <stddef.h>

static void process_message_neighbors(struct neighbor_entry *, const struct hello_message *);

static void linking_this_2_entries(struct neighbor_entry *, struct neighbor_2_entry *, olsr_reltime);

static bool lookup_mpr_status(const struct hello_message *, const struct interface_olsr *);

static bool lookup_head_status(const struct hello_message *, const struct interface_olsr *);

/**
 *Processes an list of neighbors from an incoming HELLO message.
 *@param neighbor the neighbor who sent the message.
 *@param message the HELLO message
 *@return nada
 */
static void
process_message_neighbors(struct neighbor_entry *neighbor, const struct hello_message *message)
{

  // delete all two-hop nodes connected to this neighbor
  // loop through all the elements
  int i;
  // for each hash size
  struct neighbor_2_entry *neigh2;
  struct ipaddr_str buf;

  // if the entry exists delete it to re-add it
 // OLSR_PRINTF(1, "Deleting entry %s\n", olsr_ip_to_string(&buf, &neighbor->neighbor_main_addr));
 
 /* if ((neigh2 = olsr_lookup_two_hop_neighbor_table(&neighbor->neighbor_main_addr)) != NULL)
  {
    olsr_delete_two_hop_neighbor_table(neigh2);
  }
*/
 /// olsr_print_two_hop_neighbor_table();

  struct hello_neighbor *message_neighbors;
  int count_neighbors = 0;
  for (message_neighbors = message->neighbors; message_neighbors != NULL; message_neighbors = message_neighbors->next)
  {
    union olsr_ip_addr *neigh_addr;
    struct neighbor_2_entry *two_hop_neighbor;

    count_neighbors = count_neighbors + 1;

    /*
     *check all interfaces
     *so that we don't add ourselves to the
     *2 hop list
     *IMPORTANT!
     */
    if (if_ifwithaddr(&message_neighbors->address) != NULL)
      continue;

    /* Get the main address */
    neigh_addr = mid_lookup_main_addr(&message_neighbors->address);

    if (neigh_addr != NULL)
    {
      message_neighbors->address = *neigh_addr;
    }

    struct ipaddr_str bufff;

    // process if the neighbor in the message is a head
    if (((message_neighbors->status == SYM_NEIGH) || (message_neighbors->status == MPR_NEIGH)) || (message_neighbors->status == MPR_HEAD) || (message_neighbors->status == NHEAD) || (message_neighbors->status == TWO_HEAD))
    {

      // look if the two-hop, one-hop tuple exist
      struct neighbor_2_list_entry *two_hop_neighbor_yet = olsr_lookup_my_neighbors(neighbor, &message_neighbors->address);

      if (two_hop_neighbor_yet != NULL)
      {
       /* Updating the holding time for this neighbor */
        olsr_set_timer(&two_hop_neighbor_yet->nbr2_list_timer, message->vtime, OLSR_NBR2_LIST_JITTER, OLSR_TIMER_ONESHOT,
                       &olsr_expire_nbr2_list, two_hop_neighbor_yet, 0);
        two_hop_neighbor = two_hop_neighbor_yet->neighbor_2;

        /*
         * For link quality OLSR, reset the path link quality here.
         * The path link quality will be calculated in the second pass, below.
         * Keep the saved_path_link_quality for reference.
         */

        if (olsr_cnf->lq_level > 0)
        {
          /*
           * loop through the one-hop neighbors that see this
           * 'two_hop_neighbor'
           */

          struct neighbor_list_entry *walker;

          for (walker = two_hop_neighbor->neighbor_2_nblist.next; walker != &two_hop_neighbor->neighbor_2_nblist;
               walker = walker->next)
          {
            /*
             * have we found the one-hop neighbor that sent the
             * HELLO message that we're current processing?
             */

            if (walker->neighbor == neighbor)
            {
              walker->path_linkcost = LINK_COST_BROKEN;
            }
          }
        }
        two_hop_neighbor = olsr_lookup_two_hop_neighbor_table(&message_neighbors->address);
        //if the node was a two-hop head and its status changed to not head remove it
        if (two_hop_neighbor != NULL)
        {
          if (message_neighbors->status < 3)
          { // status is not 1
            olsr_delete_two_hop_neighbor_table(two_hop_neighbor);
          }
        }
        
      }
      else
      {
        two_hop_neighbor = olsr_lookup_two_hop_neighbor_table(&message_neighbors->address);
        struct neighbor_entry *is_one_neigh;
        is_one_neigh = olsr_lookup_neighbor_table(&message_neighbors->address);
        if (two_hop_neighbor == NULL && is_one_neigh == NULL)
        {
         // OLSR_PRINTF(1, "not two and not one\n");
          changes_neighborhood = true;
          changes_topology = true;

          two_hop_neighbor = olsr_malloc(sizeof(struct neighbor_2_entry), "Process HELLO");

          two_hop_neighbor->neighbor_2_nblist.next = &two_hop_neighbor->neighbor_2_nblist;

          two_hop_neighbor->neighbor_2_nblist.prev = &two_hop_neighbor->neighbor_2_nblist;

          two_hop_neighbor->neighbor_2_pointer = 0;

          two_hop_neighbor->neighbor_2_addr = message_neighbors->address;

          if ((message_neighbors->status == MPR_HEAD) || (message_neighbors->status == NHEAD) || (message_neighbors->status == TWO_HEAD))
          {
            if (message_neighbors->status == TWO_HEAD)
            {
              two_hop_neighbor->is_2hop = false;
            }
            else
            {
              two_hop_neighbor->is_2hop = true;
            }

            olsr_insert_two_hop_neighbor_table(two_hop_neighbor);

            linking_this_2_entries(neighbor, two_hop_neighbor, message->vtime);

            // we call the calculation of mprs for heads only when the node become head or when a new 2-hop head is added to the list.
            
          }

          // else
          //{
          //	two_hop_neighbor->is_head =0;
          //}
        }
        else if (is_one_neigh == NULL && two_hop_neighbor != NULL)
        {
          /*
            linking to this two_hop_neighbor entry
          */
         // OLSR_PRINTF(1, "is two and not one\n");
          changes_neighborhood = true;
          changes_topology = true;
        //  OLSR_PRINTF(1, "Existing 2-hop neighbor IP address %s with status %d\n", olsr_ip_to_string(&bufff, &message_neighbors->address), message_neighbors->status);
          if (message_neighbors->status >= 3)
          { 
            linking_this_2_entries(neighbor, two_hop_neighbor, message->vtime);
          }
          else
          {
           // OLSR_PRINTF(1, "Deleting neighbor \n");
            olsr_delete_two_hop_neighbor_table(two_hop_neighbor);
          }
        }
      }
    }
  }

  neighbor->neighbor_size = count_neighbors;
  struct ipaddr_str bufff;
  // OLSR_PRINTF(1,"Neighbor %s has %d neighbors\n",olsr_ip_to_string(&bufff,& neighbor->neighbor_main_addr), neighbor->neighbor_size);

  /* Separate, second pass for link quality OLSR */
  /* Separate, second and third pass for link quality OLSR */

  if (olsr_cnf->lq_level > 0)
  {
    olsr_linkcost first_hop_pathcost;
    struct link_entry *lnk = get_best_link_to_neighbor(&neighbor->neighbor_main_addr);

    if (!lnk)
      return;

    /* calculate first hop path quality */
    first_hop_pathcost = lnk->linkcost;
    /*
     *  Second pass for link quality OLSR: calculate the best 2-hop
     * path costs to all the 2-hop neighbors indicated in the
     * HELLO message. Since the same 2-hop neighbor may be listed
     * more than once in the same HELLO message (each at a possibly
     * different quality) we want to select only the best one, not just
     * the last one listed in the HELLO message.
     */

    for (message_neighbors = message->neighbors; message_neighbors != NULL; message_neighbors = message_neighbors->next)
    {
      if (if_ifwithaddr(&message_neighbors->address) != NULL)
        continue;

      if (((message_neighbors->status == SYM_NEIGH) || (message_neighbors->status == MPR_NEIGH) || (message_neighbors->status == MPR_HEAD) || (message_neighbors->status == NHEAD) || (message_neighbors->status == TWO_HEAD)))
      {
        struct neighbor_list_entry *walker;
        struct neighbor_2_entry *two_hop_neighbor;
        struct neighbor_2_list_entry *two_hop_neighbor_yet = olsr_lookup_my_neighbors(neighbor,
                                                                                      &message_neighbors->address);

        if (!two_hop_neighbor_yet)
          continue;

        two_hop_neighbor = two_hop_neighbor_yet->neighbor_2;
       // if (!two_hop_neighbor->is_2hop)
         // continue;
        /*
         *  loop through the one-hop neighbors that see this
         * 'two_hop_neighbor'
         */

        for (walker = two_hop_neighbor->neighbor_2_nblist.next; walker != &two_hop_neighbor->neighbor_2_nblist;
             walker = walker->next)
        {
          /*
           * have we found the one-hop neighbor that sent the
           * HELLO message that we're current processing?
           */

          if (walker->neighbor == neighbor)
          {
            olsr_linkcost new_second_hop_linkcost, new_path_linkcost;

            // the link cost between the 1-hop neighbour and the
            // 2-hop neighbour

            new_second_hop_linkcost = message_neighbors->cost;

            // the total cost for the route
            // "us --- 1-hop --- 2-hop"

            new_path_linkcost = first_hop_pathcost + new_second_hop_linkcost;
            //OLSR_PRINTF(1,"new path link cost: %d\n",new_path_linkcost);
            // Only copy the link quality if it is better than what we have
            // for this 2-hop neighbor
            if (new_path_linkcost < walker->path_linkcost)
            {
              walker->second_hop_linkcost = new_second_hop_linkcost;
              walker->path_linkcost = new_path_linkcost;

              walker->saved_path_linkcost = new_path_linkcost;

              changes_neighborhood = true;
              changes_topology = true;
            }
          }
        }
      }
    }
  } 
  
  /*if (olsr_cnf->is_head >= 1)
  {
    olsr_calculate_lq_mpr();
  }*/
}

/**
 *Links a one-hop neighbor with a 2-hop neighbor.
 *
 *@param neighbor the 1-hop neighbor
 *@param two_hop_neighbor the 2-hop neighbor
 *@param vtime the validity time
 */
static void
linking_this_2_entries(struct neighbor_entry *neighbor, struct neighbor_2_entry *two_hop_neighbor, olsr_reltime vtime)
{
  struct neighbor_list_entry *list_of_1_neighbors = olsr_malloc(sizeof(struct neighbor_list_entry), "Link entries 1");
  struct neighbor_2_list_entry *list_of_2_neighbors = olsr_malloc(sizeof(struct neighbor_2_list_entry), "Link entries 2");

  list_of_1_neighbors->neighbor = neighbor;

  list_of_1_neighbors->path_linkcost = LINK_COST_BROKEN;
  list_of_1_neighbors->saved_path_linkcost = LINK_COST_BROKEN;
  list_of_1_neighbors->second_hop_linkcost = LINK_COST_BROKEN;

  /* Queue */
  two_hop_neighbor->neighbor_2_nblist.next->prev = list_of_1_neighbors;
  list_of_1_neighbors->next = two_hop_neighbor->neighbor_2_nblist.next;

  two_hop_neighbor->neighbor_2_nblist.next = list_of_1_neighbors;
  list_of_1_neighbors->prev = &two_hop_neighbor->neighbor_2_nblist;
  list_of_2_neighbors->neighbor_2 = two_hop_neighbor;
  list_of_2_neighbors->nbr2_nbr = neighbor; /* XXX refcount */

  olsr_change_timer(list_of_2_neighbors->nbr2_list_timer, vtime, OLSR_NBR2_LIST_JITTER, OLSR_TIMER_ONESHOT);

  /* Queue */
  neighbor->neighbor_2_list.next->prev = list_of_2_neighbors;
  list_of_2_neighbors->next = neighbor->neighbor_2_list.next;
  neighbor->neighbor_2_list.next = list_of_2_neighbors;
  list_of_2_neighbors->prev = &neighbor->neighbor_2_list;

  /*increment the pointer counter */
  two_hop_neighbor->neighbor_2_pointer++;
}

/**
 * Check if a hello message states this node as a MPR.
 *
 * @param message the message to check
 * @param in_if the incoming interface
 *
 *@return 1 if we are selected as MPR 0 if not
 */
static bool
lookup_mpr_status(const struct hello_message *message, const struct interface_olsr *in_if)
{
  struct hello_neighbor *neighbors;

  for (neighbors = message->neighbors; neighbors; neighbors = neighbors->next)
  {
    if (neighbors->link != UNSPEC_LINK && (olsr_cnf->ip_version == AF_INET
                                               ? ip4equal(&neighbors->address.v4, &in_if->ip_addr.v4)
                                               : ip6equal(&neighbors->address.v6, &in_if->int6_addr.sin6_addr)))
    {
      // OLSR_PRINTF(1,"\n\nthe neighbor status sym: %s, is MPR_NEIGH? %s,  is MPR_HEAD? %s\n\n", neighbors->status == SYM ? "YES " : "NO  ",neighbors->status == MPR_NEIGH ? "YES " : "NO  ",neighbors->status == MPR_HEAD ? "YES " : "NO  ");
      //  if (neighbors->link == SYM_LINK &&( (neighbors->status == MPR_NEIGH)||(neighbors->status == MPR_HEAD)||(neighbors->status == NHEAD))) {
      if (neighbors->link == SYM_LINK && ((neighbors->status == MPR_NEIGH) || (neighbors->status == MPR_HEAD)))
      {

        return true;
      }
      break;
    }
  }
  /* Not found */
  return false;
}

// update
// to know if I am the head of the neighbor
static bool
lookup_head_status(const struct hello_message *message, const struct interface_olsr *in_if)
{
  struct hello_neighbor *neighbors;

  for (neighbors = message->neighbors; neighbors; neighbors = neighbors->next)
  {
    if (neighbors->link != UNSPEC_LINK && (olsr_cnf->ip_version == AF_INET
                                               ? ip4equal(&neighbors->address.v4, &in_if->ip_addr.v4)
                                               : ip6equal(&neighbors->address.v6, &in_if->int6_addr.sin6_addr)))
    {

      // if (neighbors->link == SYM_LINK &&( (neighbors->status == MPR_NEIGH)||(neighbors->status == MPR_HEAD)||(neighbors->status == NHEAD))) {
      if (neighbors->link == SYM_LINK && neighbors->status == MPR_HEAD)
      {
        return true;
      }
      break;
    }
  }
  /* Not found */
  return false;
}

static int
deserialize_hello(struct hello_message *hello, const void *ser)
{
  static const int LINK_ORDER[] = HELLO_LINK_ORDER_ARRAY;
  const unsigned char *curr, *limit;
  uint8_t type;
  uint16_t size;
  struct ipaddr_str buf;
  const unsigned char *curr_saved;
  unsigned int idx;
  struct hello_neighbor *neigh_unspec_first_prev = NULL;
  struct hello_neighbor *neigh_unspec_first = NULL;

  assert(LINK_ORDER[0] == UNSPEC_LINK);

  memset(hello, 0, sizeof(*hello));

  curr = ser;
  pkt_get_u8(&curr, &type);
  if (type != HELLO_MESSAGE && type != LQ_HELLO_MESSAGE)
  {
    /* No need to do anything more */
    return 1;
  }
  pkt_get_reltime(&curr, &hello->vtime);
  pkt_get_u16(&curr, &size);
  pkt_get_ipaddress(&curr, &hello->source_addr);
  pkt_get_u8(&curr, &hello->ttl);
  pkt_get_u8(&curr, &hello->hop_count);
  pkt_get_u16(&curr, &hello->packet_seq_number);
  pkt_ignore_u16(&curr);

  pkt_get_reltime(&curr, &hello->htime);
  pkt_get_u8(&curr, &hello->willingness);
  // OLSR_PRINTF(1,"the willingness is: %d\n",hello->willingness);

  // update
  pkt_get_u8(&curr, &hello->qos);
  uint8_t qos2;
  pkt_get_u8(&curr, &qos2);
  uint16_t qos_complete = (qos2 << 8) | (hello->qos & 0xff);
  hello->qos = qos_complete;
  // pkt_ignore_u8(&curr);

  pkt_get_u8(&curr, &hello->is_head);

  pkt_ignore_u8(&curr);

  ///OLSR_PRINTF(1, "READING FROM PACKAGE HELLO %d\n", hello->is_head);
  // why this has a non make sense value
  // OLSR_PRINTF(1,"the neighbor qos 2 is: %d\n",hello->qos);

  hello->neighbors = NULL;

  limit = ((const unsigned char *)ser) + size;

  curr_saved = curr;

  for (idx = 0; idx < (sizeof(LINK_ORDER) / sizeof(LINK_ORDER[0])); idx++)
  {
    curr = curr_saved;

    while (curr < limit)
    {
      const unsigned char *limit2 = curr;
      uint8_t link_code;
      uint16_t size2;

      pkt_get_u8(&curr, &link_code);
      pkt_ignore_u8(&curr);
      pkt_get_u16(&curr, &size2);

      limit2 += size2;

      if (EXTRACT_LINK(link_code) != LINK_ORDER[idx])
      {
        curr = limit2;
        continue;
      }

      while (curr < limit2)
      {
        struct hello_neighbor *neigh = olsr_malloc_hello_neighbor("HELLO deserialization");
        pkt_get_ipaddress(&curr, &neigh->address);
        struct ipaddr_str bufip;

        if (type == LQ_HELLO_MESSAGE)
        {

          olsr_deserialize_hello_lq_pair(&curr, neigh);
        }
        neigh->link = EXTRACT_LINK(link_code);
        neigh->status = EXTRACT_STATUS(link_code);

        neigh->next = hello->neighbors;
        hello->neighbors = neigh;

        if (neigh->link == UNSPEC_LINK)
        {
          neigh_unspec_first = neigh;
        }
        else if (!neigh_unspec_first_prev)
        {
          neigh_unspec_first_prev = neigh;
        }
      }
    }
  }

  if (neigh_unspec_first_prev && neigh_unspec_first)
  {
    struct hello_neighbor *neigh;
    for (neigh = hello->neighbors; neigh && (neigh != neigh_unspec_first); neigh = neigh->next)
    {
      struct hello_neighbor *neigh_cull;
      struct hello_neighbor *neigh_cull_prev;
      struct hello_neighbor *neigh_cull_next;

      for (neigh_cull_prev = neigh_unspec_first_prev, neigh_cull = neigh_unspec_first;
           neigh_cull;
           neigh_cull = neigh_cull_next)
      {
        neigh_cull_next = neigh_cull->next;

        if (!ipequal(&neigh_cull->address, &neigh->address))
        {
          neigh_cull_prev = neigh_cull;
          continue;
        }

        if (neigh_cull == neigh_unspec_first)
        {
          neigh_unspec_first = neigh_cull_next;
        }

        neigh_cull_prev->next = neigh_cull_next;
        free(neigh_cull);
      }
    }
  }

  return 0;
}

bool olsr_input_hello(union olsr_message *ser, struct interface_olsr *inif, union olsr_ip_addr *from)
{
  struct hello_message hello;

  if (ser == NULL)
  {
    return false;
  }
  if (deserialize_hello(&hello, ser) != 0)
  {
    return false;
  }
  olsr_hello_tap(&hello, inif, from);

  /* Do not forward hello messages */
  return false;
}

/**
 *Initializing the parser functions we are using
 */
void olsr_init_package_process(void)
{
  if (olsr_cnf->lq_level == 0)
  {
    olsr_parser_add_function(&olsr_input_hello, HELLO_MESSAGE);
    olsr_parser_add_function(&olsr_input_tc, TC_MESSAGE);
  }
  else
  {

    olsr_parser_add_function(&olsr_input_hello, LQ_HELLO_MESSAGE);
    olsr_parser_add_function(&olsr_input_tc, LQ_TC_MESSAGE);
  }

  olsr_parser_add_function(&olsr_input_mid, MID_MESSAGE);
  olsr_parser_add_function(&olsr_input_hna, HNA_MESSAGE);
}

void olsr_hello_tap(struct hello_message *message, struct interface_olsr *in_if, const union olsr_ip_addr *from_addr)
{
  struct neighbor_entry *neighbor;
  // OLSR_PRINTF(1,"In Hello_tap\n");
  /*
   * Update link status
   */
  struct link_entry *lnk = update_link_entry(&in_if->ip_addr, from_addr, message, in_if);


  //update: Commented the following to check whats making all neighbors not symetric
  // if(lnk->neighbor->my_head && lnk->neighbor->status==NOT_SYM){
  //   changes_in_head_status=true;
  // }


  /*check alias message->source_addr*/
  if (!ipequal(&message->source_addr, from_addr))
  {
    /*new alias of new neighbour are thrown in the mid table to speed up routing*/
    if (olsr_validate_address(from_addr))
    {
      union olsr_ip_addr *main_addr = mid_lookup_main_addr(from_addr);
      if ((main_addr == NULL) || (ipequal(&message->source_addr, main_addr)))
      {
        /*struct ipaddr_str srcbuf, origbuf;
        olsr_syslog(OLSR_LOG_INFO, "got hello from unknown alias ip of direct neighbour: ip: %s main-ip: %s",
                    olsr_ip_to_string(&origbuf,&message->source_addr),
                    olsr_ip_to_string(&srcbuf,from_addr));*/
        insert_mid_alias(&message->source_addr, from_addr, message->vtime);
      }
      else
      {
        struct ipaddr_str srcbuf, origbuf;
        olsr_syslog(OLSR_LOG_INFO, "got hello with invalid from and originator address pair (%s, %s) Duplicate Ips?\n",
                    olsr_ip_to_string(&origbuf, &message->source_addr),
                    olsr_ip_to_string(&srcbuf, from_addr));
      }
    }
  }

  if (olsr_cnf->lq_level > 0)
  {
    struct hello_neighbor *walker;
    /* just in case our neighbor has changed its HELLO interval */
    olsr_update_packet_loss_hello_int(lnk, message->htime);

    /* find the input interface in the list of neighbor interfaces */
    for (walker = message->neighbors; walker != NULL; walker = walker->next)
    {
      if (ipequal(&walker->address, &in_if->ip_addr))
      {
        /*
         * memorize our neighbour's idea of the link quality, so that we
         * know the link quality in both directions
         */
        olsr_memorize_foreign_hello_lq(lnk, walker->link != UNSPEC_LINK ? walker : NULL);
        break;
      }
    }
    /* update packet loss for link quality calculation */
    olsr_received_hello_handler(lnk);
  }

  neighbor = lnk->neighbor;

  /*
   * Hysteresis
   */
  if (olsr_cnf->use_hysteresis)
  {
    /* Update HELLO timeout */
    /* printf("MESSAGE HTIME: %f\n", message->htime); */
    olsr_update_hysteresis_hello(lnk, message->htime);
  }
  struct ipaddr_str buf11;
  //OLSR_PRINTF(1, "Checking if MPR Selector for node %s \n ", olsr_ip_to_string(&buf11, &message->source_addr));
  /* Check if we are chosen as MPR */
  if (lookup_mpr_status(message, in_if))
  {

    struct ipaddr_str buf;
    /* source_addr is always the main addr of a node! */
    //  OLSR_PRINTF(1,"Should be adding an MPR Selector\n");
  //  OLSR_PRINTF(1, "Adding an MPR Selector for node %s \n ", olsr_ip_to_string(&buf, &message->source_addr));

    olsr_update_mprs_set(&message->source_addr, message->vtime);
    struct neighbor_entry *a_neighbor;

    /*	 OLSR_FOR_ALL_NBR_ENTRIES(a_neighbor) {
       OLSR_PRINTF(1,"checking  neighbor %s and message sender %s \n",olsr_ip_to_string(&buf, &a_neighbor->neighbor_main_addr),olsr_ip_to_string(&buf, &message->source_addr));
           if (ipequal(&a_neighbor->neighbor_main_addr, &message->source_addr)){
           a_neighbor->is_mpr=true;
           // OLSR_PRINTF(1,"found it! \n");
           }
       }
  OLSR_FOR_ALL_NBR_ENTRIES_END(a_neighbor);*/
    /*    uint32_t hash;
        struct neighbor_entry *new_neigh;
        hash = olsr_ip_hashing(&olsr_cnf->main_addr);
         for (new_neigh = neighbortable[hash].next; new_neigh != &neighbortable[hash]; new_neigh = new_neigh->next) {
           OLSR_PRINTF(1,"checking for neighbor %s \n",olsr_ip_to_string(&buf, &new_neigh->neighbor_main_addr));
        if (ipequal(&new_neigh->neighbor_main_addr, &olsr_cnf->main_addr)){
          new_neigh->is_mpr=true;
             OLSR_PRINTF(1,"making an mpr \n");
        }
         // return new_neigh;
      }*/
  }

  // it is not checking if the other is head
  //  update
  if (lookup_head_status(message, in_if))
  {
    olsr_update_heads_set(&message->source_addr, message->vtime);

    //     OLSR_PRINTF(1,"Should be adding head Selector\n");
    if (olsr_cnf->is_head < 3 && olsr_cnf->is_head != 2)
      olsr_cnf->is_head = olsr_cnf->is_head + 2;
  }
  /*else
  {
  }*/

  // if (neighbor->status != message->status){
  //     OLSR_PRINTF(1,"Status of the neighbor changed");
  //     neighbor->status = message->status;      
  // }
  /* Check willingness */
  if (neighbor->willingness != message->willingness)
  {
    struct ipaddr_str buf;
    OLSR_PRINTF(2, "Willingness for %s changed from %d to %d - UPDATING\n", olsr_ip_to_string(&buf, &neighbor->neighbor_main_addr),
                neighbor->willingness, message->willingness);
    /*
     *If willingness changed - recalculate
     */
    neighbor->willingness = message->willingness;
    changes_neighborhood = true;
    changes_topology = true;
  }
  struct ipaddr_str buf;
  OLSR_PRINTF(2, "qos for %s changed from %d to %d - UPDATING\n", olsr_ip_to_string(&buf, &neighbor->neighbor_main_addr),
              neighbor->qos, message->qos);
  // update
  if (neighbor->qos != message->qos)
  {
    changes_neighborhood = true;
    changes_topology = true;
    neighbor->qos = message->qos;
  }
  //OLSR_PRINTF(1, " CHecking head status NEIGHBOR AS HEAD %s %d \n", olsr_ip_to_string(&buf, &neighbor->neighbor_main_addr), message->is_head);
  if (neighbor->is_head != message->is_head)
  {
    struct ipaddr_str buf;
    changes_neighborhood = true;
    changes_topology = true;
    neighbor->is_head = message->is_head;
    //OLSR_PRINTF(1, " changing NEIGHBOR HEAD status %s \n", olsr_ip_to_string(&buf, &neighbor->neighbor_main_addr));
  }

  /* Don't register neighbors of neighbors that announces WILL_NEVER */
  if (neighbor->willingness != WILL_NEVER)
    process_message_neighbors(neighbor, message);

  /* Process changes immediately in case of MPR updates */
  olsr_process_changes();

  // olsr_print_neighbor_table();
  olsr_free_hello_packet(message);

  return;
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
