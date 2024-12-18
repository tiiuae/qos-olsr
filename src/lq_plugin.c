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

#include "tc_set.h"
#include "link_set.h"
#include "olsr_spf.h"
#include "lq_packet.h"
#include "packet.h"
#include "olsr.h"
#include "two_hop_neighbor_table.h"
#include "common/avl.h"

#include "lq_plugin_default_float.h"
#include "lq_plugin_default_fpm.h"
#include "lq_plugin_default_ff.h"
#include "lq_plugin_default_ffeth.h"

#ifdef LINUX_NL80211
#include "linux/lq_plugin_ffeth_nl80211.h"
#endif

#include <assert.h>
#include <math.h>

struct avl_tree lq_handler_tree;
struct lq_handler *active_lq_handler = NULL;

/**
 * case-insensitive string comparator for avl-trees
 * @param str1
 * @param str2
 * @return
 */
int avl_strcasecmp(const void *str1, const void *str2)
{
  return strcasecmp(str1, str2);
}

/**
 * Activate a LQ handler
 * @param name
 */
static void
activate_lq_handler(const char *name)
{
  struct lq_handler_node *node;

  node = (struct lq_handler_node *)avl_find(&lq_handler_tree, name);
  if (node == NULL)
  {
    char buf[1024];
    snprintf(buf, sizeof(buf), "Error, unknown lq_handler '%s'", name);
    olsr_exit(buf, EXIT_FAILURE);
  }

  OLSR_PRINTF(1, "Using '%s' algorithm for lq calculation.\n", name);
  active_lq_handler = node->handler;
  active_lq_handler->initialize();
}

/**
 * Initialize LQ handler
 */
void init_lq_handler_tree(void)
{
  avl_init(&lq_handler_tree, &avl_strcasecmp);
  register_lq_handler(&lq_etx_float_handler, LQ_ALGORITHM_ETX_FLOAT_NAME);
  register_lq_handler(&lq_etx_fpm_handler, LQ_ALGORITHM_ETX_FPM_NAME);
  register_lq_handler(&lq_etx_ff_handler, LQ_ALGORITHM_ETX_FF_NAME);
  register_lq_handler(&lq_etx_ffeth_handler, LQ_ALGORITHM_ETX_FFETH_NAME);
#ifdef LINUX_NL80211
  register_lq_handler(&lq_etx_ffeth_nl80211_handler, LQ_ALGORITHM_ETX_FFETH_NL80211_NAME);
#endif

  if (olsr_cnf->lq_algorithm == NULL)
  {
    activate_lq_handler(DEF_LQ_ALGORITHM);
  }
  else
  {
    activate_lq_handler(olsr_cnf->lq_algorithm);
  }
}

/**
 * set_lq_handler
 *
 * this function is used by routing metric plugins to activate their link
 * quality handler
 *
 * The name parameter is marked as "unused" to squelch a compiler warning if debug
 * output is not active
 *
 * @param handler pointer to lq_handler structure
 * @param name name of the link quality handler for debug output
 */
void register_lq_handler(struct lq_handler *handler, const char *name)
{
  struct lq_handler_node *node;
  size_t name_size = strlen(name) + 1;

  node = olsr_malloc(sizeof(*node) + name_size, "olsr lq handler");

  strscpy(node->name, name, name_size);
  node->node.key = node->name;
  node->handler = handler;

  avl_insert(&lq_handler_tree, &node->node, false);
}

/**
 * olsr_calc_tc_cost
 *
 * this function calculates the linkcost of a tc_edge_entry
 *
 * @param tc_edge pointer to the tc_edge_entry
 * @return linkcost
 */
olsr_linkcost
olsr_calc_tc_cost(const struct tc_edge_entry *tc_edge)
{
  assert((const char *)tc_edge + sizeof(*tc_edge) >= (const char *)tc_edge->linkquality);
  return active_lq_handler->calc_tc_cost(tc_edge->linkquality);
}

/**
 * olsr_serialize_hello_lq_pair
 *
 * this function converts the lq information of a lq_hello_neighbor into binary package
 * format
 *
 * @param buff pointer to binary buffer to write into
 * @param neigh pointer to lq_hello_neighbor
 * @return number of bytes that have been written
 */
int olsr_serialize_hello_lq_pair(unsigned char *buff, struct lq_hello_neighbor *neigh)
{
  assert((const char *)neigh + sizeof(*neigh) >= (const char *)neigh->linkquality);
  return active_lq_handler->serialize_hello_lq(buff, neigh->linkquality);
}

/**
 * olsr_deserialize_hello_lq_pair
 *
 * this function reads the lq information of a binary package into a hello_neighbor
 * It also initialize the cost variable of the hello_neighbor
 *
 * @param curr pointer to the current buffer pointer
 * @param neigh pointer to hello_neighbor
 */
void olsr_deserialize_hello_lq_pair(const uint8_t **curr, struct hello_neighbor *neigh)
{
  assert((const char *)neigh + sizeof(*neigh) >= (const char *)neigh->linkquality);
  active_lq_handler->deserialize_hello_lq(curr, neigh->linkquality);
  olsr_linkcost sampledLC = active_lq_handler->calc_hello_cost(neigh->linkquality);

  const struct lq_ffeth *lq = neigh->linkquality;

  OLSR_PRINTF(1,"In Plugin- Trend: %d, previous cost %d, sampled cost %d, ",lq->valueBandwidth, neigh->cost,sampledLC);

  // if the previous cost is 0, set it as the current cost
  if (neigh->cost==0){
    neigh->cost=sampledLC;
  }

  // if there's a trend, set the neighbor cost as the current cost 
  // sampledLC is usually in the range up to 10 or 11 based on the max penalty value of 7 and the LQ cost (converges to 1)
  if(lq->valueBandwidth!=0){
      neigh->cost = sampledLC;
  }	
  // else if we don't have a trend, degrade the cost gradually 
  else{

      // if the current cost is higher than 10.x, set the neighbor cost and the sampledLC
      if(sampledLC > 41000)
      {
        neigh->cost= sampledLC;
      }
      // if the previous neighbor cost is high, set it to the current value
      // this is important for when we suddenly drop the sampledLC cost 
      if(neigh->cost > 41000)
      {
        neigh->cost= sampledLC;
      }
      // in this case, if the sampled is higher than 10, the neigh cost will be the same value
      neigh->cost = (0.99*neigh->cost) +(0.01*sampledLC);
  }

     OLSR_PRINTF(1,"weighted cost: %f \n",neigh->cost*1.0/1024.0);

  //     neigh->cost = (0.7*neigh->cost) +(0.3*sampledLC);
      // neigh->cost=sampledLC;
}

/**
 * olsr_serialize_tc_lq_pair
 *
 * this function converts the lq information of a olsr_serialize_tc_lq_pair
 * into binary package format
 *
 * @param buff pointer to binary buffer to write into
 * @param neigh pointer to olsr_serialize_tc_lq_pair
 * @return number of bytes that have been written
 */
int olsr_serialize_tc_lq_pair(unsigned char *buff, struct tc_mpr_addr *neigh)
{
  assert((const char *)neigh + sizeof(*neigh) >= (const char *)neigh->linkquality);
  return active_lq_handler->serialize_tc_lq(buff, neigh->linkquality);
}

/**
 * olsr_deserialize_tc_lq_pair
 *
 * this function reads the lq information of a binary package into a tc_edge_entry
 *
 * @param curr pointer to the current buffer pointer
 * @param edge pointer to tc_edge_entry
 */
void olsr_deserialize_tc_lq_pair(const uint8_t **curr, struct tc_edge_entry *edge)
{
  assert((const char *)edge + sizeof(*edge) >= (const char *)edge->linkquality);
  active_lq_handler->deserialize_tc_lq(curr, edge->linkquality);
}

/**
 * olsr_update_packet_loss_worker
 *
 * this function is called every times a hello package for a certain link_entry
 * is lost (timeout) or received. This way the lq-plugin can update the links link
 * quality value.
 *
 * @param entry pointer to link_entry
 * @param lost true if hello package was lost
 */
void olsr_update_packet_loss_worker(struct link_entry *entry, bool lost)
{
  assert((const char *)entry + sizeof(*entry) >= (const char *)entry->linkquality);
  active_lq_handler->packet_loss_handler(entry, entry->linkquality, lost);
}

/**
 * olsr_memorize_foreign_hello_lq
 *
 * this function is called to copy the link quality information from a received
 * hello package into a link_entry.
 *
 * @param local pointer to link_entry
 * @param foreign pointer to hello_neighbor, if NULL the neighbor link quality information
 * of the link entry has to be reset to "zero"
 */
void olsr_memorize_foreign_hello_lq(struct link_entry *local, struct hello_neighbor *foreign)
{
  assert((const char *)local + sizeof(*local) >= (const char *)local->linkquality);
  if (foreign)
  {
    assert((const char *)foreign + sizeof(*foreign) >= (const char *)foreign->linkquality);
    active_lq_handler->memorize_foreign_hello(local->linkquality, foreign->linkquality);
  }
  else
  {
    active_lq_handler->memorize_foreign_hello(local->linkquality, NULL);
  }
}

/**
 * get_link_entry_text
 *
 * this function returns the text representation of a link_entry cost value.
 * It's not thread save and should not be called twice with the same println
 * value in the same context (a single printf command for example).
 *
 * @param entry to link_entry
 * @param separator separator between LQ and NLQ
 * @param buffer for output
 * @return pointer to a buffer with the text representation
 */
const char *
get_link_entry_text(struct link_entry *entry, char separator, struct lqtextbuffer *buffer)
{
  assert((const char *)entry + sizeof(*entry) >= (const char *)entry->linkquality);
  return active_lq_handler->print_hello_lq(entry->linkquality, separator, buffer);
}

/**
 * get_tc_edge_entry_text
 *
 * this function returns the text representation of a tc_edge_entry cost value.
 * It's not thread save and should not be called twice with the same println
 * value in the same context (a single printf command for example).
 *
 * @param entry pointer to tc_edge_entry
 * @param separator separator between LQ and NLQ
 * @param buffer pointer to buffer
 * @return pointer to the buffer with the text representation
 */
const char *
get_tc_edge_entry_text(struct tc_edge_entry *entry, char separator, struct lqtextbuffer *buffer)
{
  assert((const char *)entry + sizeof(*entry) >= (const char *)entry->linkquality);
  return active_lq_handler->print_tc_lq(entry->linkquality, separator, buffer);
}

/**
 * get_linkcost_text
 *
 * This function transforms an olsr_linkcost value into it's text representation and copies
 * the result into a buffer.
 *
 * @param cost link cost value
 * @param route true to transform the cost of a route, false for a link
 * @param buffer pointer to buffer
 * @return pointer to buffer filled with text
 */
const char *
get_linkcost_text(olsr_linkcost cost, bool route, struct lqtextbuffer *buffer)
{
  olsr_linkcost limit = route ? ROUTE_COST_BROKEN : LINK_COST_BROKEN;
  if (cost >= limit)
  {
    return "INFINITE";
  }

  snprintf(buffer->buf, sizeof(buffer->buf), "%.3f", active_lq_handler->get_cost_scaled(cost));
  buffer->buf[sizeof(buffer->buf) - 1] = '\0';
  return buffer->buf;
}

double
get_linkcost_scaled(olsr_linkcost cost, bool route)
{
  olsr_linkcost limit = route ? ROUTE_COST_BROKEN : LINK_COST_BROKEN;
  if (cost >= limit)
  {
    return (double)limit;
  }

  return active_lq_handler->get_cost_scaled(cost);
}

/**
 * olsr_copy_hello_lq
 *
 * this function copies the link quality information from a link_entry to a
 * lq_hello_neighbor.
 *
 * @param target pointer to target lq_hello_neighbor
 * @param source pointer to source link_entry
 */
void olsr_copy_hello_lq(struct lq_hello_neighbor *target, struct link_entry *source)
{
  assert((const char *)target + sizeof(*target) >= (const char *)target->linkquality);
  assert((const char *)source + sizeof(*source) >= (const char *)source->linkquality);

  active_lq_handler->copy_link_lq_into_neigh(target->linkquality, source->linkquality);
}

void olsr_copy_hello_lq3(struct lq_hello_neighbor *target, struct neighbor_list_entry *source)
{
  
  assert((const char *)target + sizeof(*target) >= (const char *)target->linkquality);
 
  assert((const char *)source + sizeof(*source) >= (const char *)source->path_linkcost);
  
  active_lq_handler->copy_link_lq_into_neigh(target->linkquality, (uint32_t)source->path_linkcost);

}

/**
 * olsr_copylq_link_entry_2_tc_mpr_addr
 *
 * this function copies the link quality information from a link_entry to a
 * tc_mpr_addr.
 *
 * @param target pointer to tc_mpr_addr
 * @param source pointer to link_entry
 */
void olsr_copylq_link_entry_2_tc_mpr_addr(struct tc_mpr_addr *target, struct link_entry *source)
{
  assert((const char *)target + sizeof(*target) >= (const char *)target->linkquality);
  assert((const char *)source + sizeof(*source) >= (const char *)source->linkquality);
  active_lq_handler->copy_link_lq_into_tc(target->linkquality, source->linkquality);
}

/**
 * olsr_copylq_link_entry_2_tc_edge_entry
 *
 * this function copies the link quality information from a link_entry to a
 * tc_edge_entry.
 *
 * @param target pointer to tc_edge_entry
 * @param source pointer to link_entry
 */
void olsr_copylq_link_entry_2_tc_edge_entry(struct tc_edge_entry *target, struct link_entry *source)
{
  assert((const char *)target + sizeof(*target) >= (const char *)target->linkquality);
  assert((const char *)source + sizeof(*source) >= (const char *)source->linkquality);
  active_lq_handler->copy_link_lq_into_tc(target->linkquality, source->linkquality);
}

/* clear the lq of a link set entry */
void olsr_clear_hello_lq(struct link_entry *link)
{
  active_lq_handler->clear_hello(link->linkquality);
}

/**
 * olsr_clear_tc_lq
 *
 * this function resets the linkquality value of a tc_mpr_addr
 *
 * @param target pointer to tc_mpr_addr
 */
void olsr_clear_tc_lq(struct tc_mpr_addr *target)
{
  assert((const char *)target + sizeof(*target) >= (const char *)target->linkquality);
  active_lq_handler->clear_tc(target->linkquality);
}

/**
 * olsr_malloc_hello_neighbor
 *
 * this function allocates memory for an hello_neighbor inclusive
 * linkquality data.
 *
 * @param id string for memory debugging
 *
 * @return pointer to hello_neighbor
 */
struct hello_neighbor *
olsr_malloc_hello_neighbor(const char *id)
{
  struct hello_neighbor *h;

  h = olsr_malloc(sizeof(struct hello_neighbor) + active_lq_handler->hello_lq_size, id);

  assert((const char *)h + sizeof(*h) >= (const char *)h->linkquality);
  active_lq_handler->clear_hello(h->linkquality);
  return h;
}

/**
 * olsr_malloc_tc_mpr_addr
 *
 * this function allocates memory for an tc_mpr_addr inclusive
 * linkquality data.
 *
 * @param id string for memory debugging
 *
 * @return pointer to tc_mpr_addr
 */
struct tc_mpr_addr *
olsr_malloc_tc_mpr_addr(const char *id)
{
  struct tc_mpr_addr *t;

  t = olsr_malloc(sizeof(struct tc_mpr_addr) + active_lq_handler->tc_lq_size, id);

  assert((const char *)t + sizeof(*t) >= (const char *)t->linkquality);
  active_lq_handler->clear_tc(t->linkquality);
  return t;
}

/**
 * olsr_malloc_lq_hello_neighbor
 *
 * this function allocates memory for an lq_hello_neighbor inclusive
 * linkquality data.
 *
 * @param id string for memory debugging
 *
 * @return pointer to lq_hello_neighbor
 */
struct lq_hello_neighbor *
olsr_malloc_lq_hello_neighbor(const char *id)
{
  struct lq_hello_neighbor *h;

  h = olsr_malloc(sizeof(struct lq_hello_neighbor) + active_lq_handler->hello_lq_size, id);

  assert((const char *)h + sizeof(*h) >= (const char *)h->linkquality);
  active_lq_handler->clear_hello(h->linkquality);
  return h;
}

/**
 * olsr_malloc_link_entry
 *
 * this function allocates memory for an link_entry inclusive
 * linkquality data.
 *
 * @param id string for memory debugging
 *
 * @return pointer to link_entry
 */
struct link_entry *
olsr_malloc_link_entry(const char *id)
{
  struct link_entry *h;

  h = olsr_malloc(sizeof(struct link_entry) + active_lq_handler->hello_lq_size, id);

  assert((const char *)h + sizeof(*h) >= (const char *)h->linkquality);
  active_lq_handler->clear_hello(h->linkquality);
  return h;
}

size_t olsr_sizeof_hello_lqdata(void)
{
  return active_lq_handler->hello_lqdata_size;
}

size_t olsr_sizeof_tc_lqdata(void)
{
  return active_lq_handler->tc_lqdata_size;
}

/**
 * This function should be called whenever the current linkcost
 * value changed in a relevant way.
 */
void olsr_relevant_linkcost_change(void)
{
  changes_neighborhood = true;
  changes_topology = true;

  /* XXX - we should check whether we actually announce this neighbour */
  signal_link_changes(true);
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
