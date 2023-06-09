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

#include "build_msg.h"
#include "ipcalc.h"
#include "olsr.h"
#include "log.h"
#include "mantissa.h"
#include "net_olsr.h"
#include "gateway.h"

#define BMSG_DBGLVL 5

#define OLSR_IPV4_HDRSIZE          12
#define OLSR_IPV6_HDRSIZE          24

// update
//#define OLSR_HELLO_IPV4_HDRSIZE    (OLSR_IPV4_HDRSIZE + 4)
// 2 for qos 2 for reserved
#define OLSR_HELLO_IPV4_HDRSIZE    (OLSR_IPV4_HDRSIZE + 8)

#define OLSR_HELLO_IPV6_HDRSIZE    (OLSR_IPV6_HDRSIZE + 4)
#define OLSR_TC_IPV4_HDRSIZE       (OLSR_IPV4_HDRSIZE + 4)
#define OLSR_TC_IPV6_HDRSIZE       (OLSR_IPV6_HDRSIZE + 4)
#define OLSR_MID_IPV4_HDRSIZE      OLSR_IPV4_HDRSIZE
#define OLSR_MID_IPV6_HDRSIZE      OLSR_IPV6_HDRSIZE
#define OLSR_HNA_IPV4_HDRSIZE      OLSR_IPV4_HDRSIZE
#define OLSR_HNA_IPV6_HDRSIZE      OLSR_IPV6_HDRSIZE

static void check_buffspace(int msgsize, int buffsize, const char *type);

/* All these functions share this buffer */

static uint32_t msg_buffer_align[(MAXMESSAGESIZE - OLSR_HEADERSIZE)/sizeof(uint32_t) + 1];
static uint8_t *msg_buffer = (uint8_t *)msg_buffer_align;

static uint32_t send_empty_tc;          /* TC empty message sending */

/* Prototypes for internal functions */

/* IPv4 */

static bool serialize_hello4(struct hello_message *, struct interface_olsr *);

static bool serialize_tc4(struct tc_message *, struct interface_olsr *);

static bool serialize_mid4(struct interface_olsr *);

static bool serialize_hna4(struct ip_prefix_list *h, struct interface_olsr *, bool is_zero_bw);

/* IPv6 */

static bool serialize_hello6(struct hello_message *, struct interface_olsr *);

static bool serialize_tc6(struct tc_message *, struct interface_olsr *);

static bool serialize_mid6(struct interface_olsr *);

static bool serialize_hna6(struct ip_prefix_list *h, struct interface_olsr *, bool is_zero_bw);

/**
 * Set the timer that controls the generation of
 * empty TC messages
 */
void
set_empty_tc_timer(uint32_t empty_tc_new)
{
  send_empty_tc = empty_tc_new;
}

/**
 * Get the timer that controls the generation of
 * empty TC messages
 */
uint32_t
get_empty_tc_timer(void)
{
  return send_empty_tc;
}

/**
 * Generate HELLO packet with the contents of the parameter "message".
 * If this won't fit in one packet, chop it up into several.
 * Send the packet if the size of the data contained in the output buffer
 * reach maxmessagesize. Can generate an empty HELLO packet if the
 * neighbor table is empty.
 *
 *
 *@param message the hello_message struct containing the info
 *to build the hello message from.
 *@param ifp the interface to send the message on
 *
 *@return nada
 */

bool
queue_hello(struct hello_message * message, struct interface_olsr * ifp)
{
#ifdef DEBUG
  OLSR_PRINTF(BMSG_DBGLVL, "Building HELLO on %s\n-------------------\n", ifp->int_name);
#endif /* DEBUG */

  switch (olsr_cnf->ip_version) {
  case (AF_INET6):
    return serialize_hello6(message, ifp);
  case (AF_INET):
  default:
    return serialize_hello4(message, ifp);
  }
  return false;
}

/*
 * Generate TC packet with the contents of the parameter "message".
 * If this won't fit in one packet, chop it up into several.
 * Send the packet if the size of the data contained in the output buffer
 * reach maxmessagesize.
 *
 *@param message the tc_message struct containing the info
 *to send
 *@param ifp the interface to send the message on
 *
 *@return nada
 */

bool
queue_tc(struct tc_message * message, struct interface_olsr * ifp)
{
#ifdef DEBUG
  OLSR_PRINTF(BMSG_DBGLVL, "Building TC on %s\n-------------------\n", ifp->int_name);
#endif /* DEBUG */

  switch (olsr_cnf->ip_version) {
  case (AF_INET6):
    return serialize_tc6(message, ifp);
  case (AF_INET):
  default:
    return serialize_tc4(message, ifp);
  }
  return false;
}

/**
 *Build a MID message to the outputbuffer
 *
 *<b>NO INTERNAL BUFFER</b>
 *@param ifp a pointer to the interface with the main address
 *@return 1 on success
 */

bool
queue_mid(struct interface_olsr * ifp)
{
#ifdef DEBUG
  OLSR_PRINTF(BMSG_DBGLVL, "Building MID on %s\n-------------------\n", ifp->int_name);
#endif /* DEBUG */

  switch (olsr_cnf->ip_version) {
  case (AF_INET6):
    return serialize_mid6(ifp);
  case (AF_INET):
  default:
    return serialize_mid4(ifp);
  }
  return false;
}

/**
 *Builds a HNA message in the outputbuffer
 *<b>NB! Not internal packetformat!</b>
 *
 *@param ifp the interface to send on
 *@return nada
 */
bool
queue_hna(struct interface_olsr * ifp)
{
#ifdef DEBUG
  OLSR_PRINTF(BMSG_DBGLVL, "Building HNA on %s\n-------------------\n", ifp->int_name);
#endif /* DEBUG */

  switch (olsr_cnf->ip_version) {
  case (AF_INET6):
    return serialize_hna6(olsr_cnf->hna_entries, ifp, false);
  case (AF_INET):
  default:
    return serialize_hna4(olsr_cnf->hna_entries, ifp, false);
  }
  return false;
}

/*
 * Protocol specific versions
 */

static void
check_buffspace(int msgsize, int buffsize, const char *type)
{
  if (msgsize > buffsize) {
    char buf[1024];
    snprintf(buf, sizeof(buf), "%s: %s build, output buffer too small (%d/%u)", __func__, type, msgsize, buffsize);
    olsr_exit(buf, EXIT_FAILURE);
  }
}

/**
 * IP version 4
 *
 *@param message the hello_message struct containing the info
 *to build the hello message from.
 *@param ifp the interface to send the message on
 *
 *@return nada
 */

static bool
serialize_hello4(struct hello_message *message, struct interface_olsr *ifp)
{
	// update 
  uint16_t remainsize, curr_size;
  struct hello_neighbor *nb;
  union olsr_message *m;
  struct hellomsg *h;
  struct hellinfo *hinfo;
  char *haddr;
  int i, j;
  bool first_entry;

  if ((!message) || (!ifp) || (olsr_cnf->ip_version != AF_INET))
    return false;

  remainsize = net_outbuffer_bytes_left(ifp);

  m = (union olsr_message *)msg_buffer;

  curr_size = OLSR_HELLO_IPV4_HDRSIZE;

  /* Send pending packet if not room in buffer */
  if (curr_size > remainsize) {
    net_output(ifp);
    remainsize = net_outbuffer_bytes_left(ifp);
  }
  /* Sanity check */
  check_buffspace(curr_size, remainsize, "HELLO");

  h = &m->v4.message.hello;
  hinfo = h->hell_info;
  haddr = (char *)hinfo->neigh_addr;

  /* Fill message header */
  m->v4.ttl = message->ttl;
  m->v4.hopcnt = 0;
  m->v4.olsr_msgtype = HELLO_MESSAGE;
  /* Set source(main) addr */
  m->v4.originator = olsr_cnf->main_addr.v4.s_addr;

  m->v4.olsr_vtime = ifp->valtimes.hello;

  /* Fill HELLO header */
  h->willingness = message->willingness;
  h->htime = reltime_to_me(ifp->hello_etime);
  memset(&h->reserved, 0, sizeof(uint16_t));
  // update
  h->qos = message->qos;
//OLSR_PRINTF(1,"\n ********** setting qos in hello %d %d\n", h->qos,message->qos);
  h->is_head = message->is_head;
  
  
  //memset(&h->reserved2, 0, sizeof(uint16_t));
  /*
   *Loops trough all possible neighbor statuses
   *The neighbor list is grouped by status
   *
   */
  /* Neighbor statuses */
  
  for (i = 0; i <= MAX_NEIGH; i++) {
    /* Link statuses */
    for (j = 0; j <= MAX_LINK; j++) {
#ifdef DEBUG
      struct ipaddr_str buf;
#endif /* DEBUG */

      /* HYSTERESIS - Not adding neighbors with link type HIDE */

      if (j == HIDE_LINK)
        continue;

      first_entry = true;

      /* Looping trough neighbors */
      for (nb = message->neighbors; nb != NULL; nb = nb->next) {
      
        if ((nb->status != i) || (nb->link != j))
          continue;


#ifdef DEBUG
        OLSR_PRINTF(BMSG_DBGLVL, "\t%s - ", olsr_ip_to_string(&buf, &nb->address));
        OLSR_PRINTF(BMSG_DBGLVL, "L:%d N:%d\n", j, i);
#endif /* DEBUG */
        /*
         * If there is not enough room left
         * for the data in the outputbuffer
         * we must send a partial HELLO and
         * continue building the rest of the
         * data in a new HELLO message
         *
         * If this is the first neighbor in
         * a group, we must check for an extra
         * 4 bytes
         */
        if ((curr_size + olsr_cnf->ipsize + (first_entry ? 4 : 0)) > remainsize) {
          /* Only send partial HELLO if it contains data */
          if (curr_size > OLSR_HELLO_IPV4_HDRSIZE) {
#ifdef DEBUG
            OLSR_PRINTF(BMSG_DBGLVL, "Sending partial(size: %d, buff left:%d)\n", curr_size, remainsize);
#endif /* DEBUG */
            /* Complete the headers */
            m->v4.seqno = htons(get_msg_seqno());
            m->v4.olsr_msgsize = htons(curr_size);

            hinfo->size = htons((char *)haddr - (char *)hinfo);

            /* Send partial packet */
            net_outbuffer_push(ifp, msg_buffer, curr_size);

            curr_size = OLSR_HELLO_IPV4_HDRSIZE;

            h = &m->v4.message.hello;
            hinfo = h->hell_info;
            haddr = (char *)hinfo->neigh_addr;
            /* Make sure typeheader is added */
            first_entry = true;
          }

          net_output(ifp);
          /* Reset size and pointers */
          remainsize = net_outbuffer_bytes_left(ifp);

          /* Sanity check */
          check_buffspace(curr_size + olsr_cnf->ipsize + 4, remainsize, "HELLO2");
        }

        if (first_entry) {
          memset(&hinfo->reserved, 0, sizeof(uint8_t));
          /* Set link and status for this group of neighbors (this is the first) */
          hinfo->link_code = CREATE_LINK_CODE(i, j);
          
          curr_size += 4;       /* HELLO type section header */
        }

        memcpy(haddr, &nb->address, sizeof(union olsr_ip_addr));

        /* Point to next address */
        haddr += olsr_cnf->ipsize;
        curr_size += olsr_cnf->ipsize;  /* IP address added */

        first_entry = false;
      }

      if (!first_entry) {
        hinfo->size = htons((char *)haddr - (char *)hinfo);

        hinfo = (struct hellinfo *)((char *)haddr);
        haddr = (char *)hinfo->neigh_addr;

      }
    }                           /* for j */
  }                             /* for i */

  m->v4.seqno = htons(get_msg_seqno());
  m->v4.olsr_msgsize = htons(curr_size);

  net_outbuffer_push(ifp, msg_buffer, curr_size);

  /* HELLO will always be generated */
  return true;
}

/**
 * IP version 6
 *
 *@param message the hello_message struct containing the info
 *to build the hello message from.
 *@param ifp the interface to send the message on
 *
 *@return nada
 */

static bool
serialize_hello6(struct hello_message *message, struct interface_olsr *ifp)
{
  uint16_t remainsize, curr_size;
  struct hello_neighbor *nb;
  union olsr_message *m;
  struct hellomsg6 *h6;
  struct hellinfo6 *hinfo6;
  union olsr_ip_addr *haddr;
  int i, j;
  bool first_entry;

  if ((!message) || (!ifp) || (olsr_cnf->ip_version != AF_INET6))
    return false;

  remainsize = net_outbuffer_bytes_left(ifp);
  m = (union olsr_message *)msg_buffer;

  curr_size = OLSR_HELLO_IPV6_HDRSIZE;  /* OLSR message header */

  /* Send pending packet if not room in buffer */
  if (curr_size > remainsize) {
    net_output(ifp);
    remainsize = net_outbuffer_bytes_left(ifp);
  }
  check_buffspace(curr_size + olsr_cnf->ipsize + 4, remainsize, "HELLO");

  h6 = &m->v6.message.hello;
  hinfo6 = h6->hell_info;
  haddr = (union olsr_ip_addr *)hinfo6->neigh_addr;

  /* Fill message header */
  m->v6.ttl = message->ttl;
  m->v6.hopcnt = 0;
  /* Set source(main) addr */
  m->v6.originator = olsr_cnf->main_addr.v6;
  m->v6.olsr_msgtype = HELLO_MESSAGE;

  m->v6.olsr_vtime = ifp->valtimes.hello;

  /* Fill packet header */
  h6->willingness = message->willingness;
  h6->htime = reltime_to_me(ifp->hello_etime);
  memset(&h6->reserved, 0, sizeof(uint16_t));

  /*
   *Loops trough all possible neighbor statuses
   *The negbor list is grouped by status
   */

  for (i = 0; i <= MAX_NEIGH; i++) {
    for (j = 0; j <= MAX_LINK; j++) {
#ifdef DEBUG
      struct ipaddr_str buf;
#endif /* DEBUG */
      first_entry = true;

      /*
       *Looping trough neighbors
       */
      for (nb = message->neighbors; nb != NULL; nb = nb->next) {
        if ((nb->status != i) || (nb->link != j))
          continue;

#ifdef DEBUG
        OLSR_PRINTF(BMSG_DBGLVL, "\t%s - ", olsr_ip_to_string(&buf, &nb->address));
        OLSR_PRINTF(BMSG_DBGLVL, "L:%d N:%d\n", j, i);
#endif /* DEBUG */

        /*
         * If there is not enough room left
         * for the data in the outputbuffer
         * we must send a partial HELLO and
         * continue building the rest of the
         * data in a new HELLO message
         *
         * If this is the first neighbor in
         * a group, we must check for an extra
         * 4 bytes
         */
        if ((curr_size + olsr_cnf->ipsize + (first_entry ? 4 : 0)) > remainsize) {
          /* Only send partial HELLO if it contains data */
          if (curr_size > OLSR_HELLO_IPV6_HDRSIZE) {
#ifdef DEBUG
            OLSR_PRINTF(BMSG_DBGLVL, "Sending partial(size: %d, buff left:%d)\n", curr_size, remainsize);
#endif /* DEBUG */
            /* Complete the headers */
            m->v6.seqno = htons(get_msg_seqno());
            m->v6.olsr_msgsize = htons(curr_size);

            hinfo6->size = (char *)haddr - (char *)hinfo6;
            hinfo6->size = htons(hinfo6->size);

            /* Send partial packet */
            net_outbuffer_push(ifp, msg_buffer, curr_size);
            curr_size = OLSR_HELLO_IPV6_HDRSIZE;

            h6 = &m->v6.message.hello;
            hinfo6 = h6->hell_info;
            haddr = (union olsr_ip_addr *)hinfo6->neigh_addr;
            /* Make sure typeheader is added */
            first_entry = true;
          }
          net_output(ifp);
          /* Reset size and pointers */
          remainsize = net_outbuffer_bytes_left(ifp);

          check_buffspace(curr_size + olsr_cnf->ipsize + 4, remainsize, "HELLO2");

        }

        if (first_entry) {
          memset(&hinfo6->reserved, 0, sizeof(uint8_t));
          /* Set link and status for this group of neighbors (this is the first) */
          hinfo6->link_code = CREATE_LINK_CODE(i, j);
          curr_size += 4;       /* HELLO type section header */
        }

        *haddr = nb->address;

        /* Point to next address */
        haddr++;
        curr_size += olsr_cnf->ipsize;  /* IP address added */

        first_entry = false;
      }                         /* looping trough neighbors */

      if (!first_entry) {
        hinfo6->size = htons((char *)haddr - (char *)hinfo6);
        hinfo6 = (struct hellinfo6 *)((char *)haddr);
        haddr = (union olsr_ip_addr *)&hinfo6->neigh_addr;
      }

    }                           /* for j */
  }                             /* for i */

  m->v6.seqno = htons(get_msg_seqno());
  m->v6.olsr_msgsize = htons(curr_size);

  net_outbuffer_push(ifp, msg_buffer, curr_size);

  /* HELLO is always buildt */
  return true;
}

/**
 *IP version 4
 *
 *@param message the tc_message struct containing the info
 *to send
 *@param ifp the interface to send the message on
 *
 *@return nada
 */

static bool
serialize_tc4(struct tc_message *message, struct interface_olsr *ifp)
{
#ifdef DEBUG
  struct ipaddr_str buf;
#endif /* DEBUG */
  uint16_t remainsize, curr_size;
  struct tc_mpr_addr *mprs;
  union olsr_message *m;
  struct olsr_tcmsg *tc;
  struct neigh_info *mprsaddr;
  bool found = false, partial_sent = false;

  if ((!message) || (!ifp) || (olsr_cnf->ip_version != AF_INET))
    return false;

  remainsize = net_outbuffer_bytes_left(ifp);

  m = (union olsr_message *)msg_buffer;

  tc = &m->v4.message.tc;

  mprsaddr = tc->neigh;
  curr_size = OLSR_TC_IPV4_HDRSIZE;

  /* Send pending packet if not room in buffer */
  if (curr_size > remainsize) {
    net_output(ifp);
    remainsize = net_outbuffer_bytes_left(ifp);
  }
  check_buffspace(curr_size, remainsize, "TC");

  /* Fill header */
  m->v4.olsr_vtime = ifp->valtimes.tc;
  m->v4.olsr_msgtype = TC_MESSAGE;
  m->v4.hopcnt = message->hop_count;
  m->v4.ttl = message->ttl;
  m->v4.originator = message->originator.v4.s_addr;

  /* Fill TC header */
  tc->ansn = htons(message->ansn);
  tc->reserved = 0;

  /*Looping trough MPR selectors */
  for (mprs = message->multipoint_relay_selector_address; mprs != NULL; mprs = mprs->next) {
    /*If packet is to be chomped */
    if ((curr_size + olsr_cnf->ipsize) > remainsize) {

      /* Only add TC message if it contains data */
      if (curr_size > OLSR_TC_IPV4_HDRSIZE) {
#ifdef DEBUG
        OLSR_PRINTF(BMSG_DBGLVL, "Sending partial(size: %d, buff left:%d)\n", curr_size, remainsize);
#endif /* DEBUG */

        m->v4.olsr_msgsize = htons(curr_size);
        m->v4.seqno = htons(get_msg_seqno());

        net_outbuffer_push(ifp, msg_buffer, curr_size);

        /* Reset stuff */
        mprsaddr = tc->neigh;
        curr_size = OLSR_TC_IPV4_HDRSIZE;
        found = false;
        partial_sent = true;
      }

      net_output(ifp);
      remainsize = net_outbuffer_bytes_left(ifp);
      check_buffspace(curr_size + olsr_cnf->ipsize, remainsize, "TC2");

    }
    found = true;
#ifdef DEBUG
    OLSR_PRINTF(BMSG_DBGLVL, "\t%s\n", olsr_ip_to_string(&buf, &mprs->address));
#endif /* DEBUG */
    mprsaddr->addr = mprs->address.v4.s_addr;
    curr_size += olsr_cnf->ipsize;
    mprsaddr++;
  }

  if (found) {

    m->v4.olsr_msgsize = htons(curr_size);
    m->v4.seqno = htons(get_msg_seqno());

    net_outbuffer_push(ifp, msg_buffer, curr_size);

  } else {
    if ((!partial_sent) && (!TIMED_OUT(send_empty_tc))) {
      if (!TIMED_OUT(send_empty_tc))
        OLSR_PRINTF(1, "TC: Sending empty package - (%d/%d/%d/%d)\n", partial_sent, (int)send_empty_tc, (int)now_times,
                    (int)((send_empty_tc) - now_times));

      m->v4.olsr_msgsize = htons(curr_size);
      m->v4.seqno = htons(get_msg_seqno());

      net_outbuffer_push(ifp, msg_buffer, curr_size);

      found = true;
    }
  }

  return found;
}

/**
 *IP version 6
 *
 *@param message the tc_message struct containing the info
 *to send
 *@param ifp the interface to send the message on
 *
 *@return nada
 */

static bool
serialize_tc6(struct tc_message *message, struct interface_olsr *ifp)
{
#ifdef DEBUG
  struct ipaddr_str buf;
#endif /* DEBUG */
  uint16_t remainsize, curr_size;
  struct tc_mpr_addr *mprs;
  union olsr_message *m;
  struct olsr_tcmsg6 *tc6;
  struct neigh_info6 *mprsaddr6;
  bool found = false, partial_sent = false;

  if ((!message) || (!ifp) || (olsr_cnf->ip_version != AF_INET6))
    return false;

  remainsize = net_outbuffer_bytes_left(ifp);

  m = (union olsr_message *)msg_buffer;

  tc6 = &m->v6.message.tc;

  mprsaddr6 = tc6->neigh;
  curr_size = OLSR_TC_IPV6_HDRSIZE;

  /* Send pending packet if not room in buffer */
  if (curr_size > remainsize) {
    net_output(ifp);
    remainsize = net_outbuffer_bytes_left(ifp);
  }
  check_buffspace(curr_size, remainsize, "TC");

  /* Fill header */
  m->v6.olsr_vtime = ifp->valtimes.tc;
  m->v6.olsr_msgtype = TC_MESSAGE;
  m->v6.hopcnt = message->hop_count;
  m->v6.ttl = message->ttl;
  m->v6.originator = message->originator.v6;

  /* Fill TC header */
  tc6->ansn = htons(message->ansn);
  tc6->reserved = 0;

  /*Looping trough MPR selectors */
  for (mprs = message->multipoint_relay_selector_address; mprs != NULL; mprs = mprs->next) {

    /*If packet is to be chomped */
    if ((curr_size + olsr_cnf->ipsize) > remainsize) {
      /* Only add TC message if it contains data */
      if (curr_size > OLSR_TC_IPV6_HDRSIZE) {
#ifdef DEBUG
        OLSR_PRINTF(BMSG_DBGLVL, "Sending partial(size: %d, buff left:%d)\n", curr_size, remainsize);
#endif /* DEBUG */
        m->v6.olsr_msgsize = htons(curr_size);
        m->v6.seqno = htons(get_msg_seqno());

        net_outbuffer_push(ifp, msg_buffer, curr_size);
        mprsaddr6 = tc6->neigh;
        curr_size = OLSR_TC_IPV6_HDRSIZE;
        found = false;
        partial_sent = true;
      }
      net_output(ifp);
      remainsize = net_outbuffer_bytes_left(ifp);
      check_buffspace(curr_size + olsr_cnf->ipsize, remainsize, "TC2");

    }
    found = true;
#ifdef DEBUG
    OLSR_PRINTF(BMSG_DBGLVL, "\t%s\n", olsr_ip_to_string(&buf, &mprs->address));
#endif /* DEBUG */
    mprsaddr6->addr = mprs->address.v6;
    curr_size += olsr_cnf->ipsize;

    mprsaddr6++;
  }

  if (found) {
    m->v6.olsr_msgsize = htons(curr_size);
    m->v6.seqno = htons(get_msg_seqno());

    net_outbuffer_push(ifp, msg_buffer, curr_size);

  } else {
    if ((!partial_sent) && (!TIMED_OUT(send_empty_tc))) {
      OLSR_PRINTF(1, "TC: Sending empty package\n");

      m->v6.olsr_msgsize = htons(curr_size);
      m->v6.seqno = htons(get_msg_seqno());

      net_outbuffer_push(ifp, msg_buffer, curr_size);

      found = true;
    }
  }

  return found;
}

/**
 *IP version 4
 *
 *<b>NO INTERNAL BUFFER</b>
 *@param ifp use this interfaces address as main address
 *@return 1 on success
 */

static bool
serialize_mid4(struct interface_olsr *ifp)
{
  uint16_t remainsize, curr_size, needsize;
  /* preserve existing data in output buffer */
  union olsr_message *m;
  struct midaddr *addrs;
  struct interface_olsr *ifs;

  if ((olsr_cnf->ip_version != AF_INET) || (!ifp) || (ifnet == NULL) || ((ifnet->int_next == NULL) && (ipequal(&olsr_cnf->main_addr, &ifnet->ip_addr))))
    return false;

  remainsize = net_outbuffer_bytes_left(ifp);

  m = (union olsr_message *)msg_buffer;

  curr_size = OLSR_MID_IPV4_HDRSIZE;

  /* calculate size needed for HNA */
  needsize = curr_size;
  for (ifs = ifnet; ifs != NULL; ifs = ifs->int_next) {
    needsize += olsr_cnf->ipsize*2;
  }

  /* Send pending packet if not room in buffer */
  if (needsize > remainsize) {
    net_output(ifp);
    remainsize = net_outbuffer_bytes_left(ifp);
  }
  check_buffspace(curr_size, remainsize, "MID");

  /* Fill header */
  m->v4.hopcnt = 0;
  m->v4.ttl = MAX_TTL;
  /* Set main(first) address */
  m->v4.originator = olsr_cnf->main_addr.v4.s_addr;
  m->v4.olsr_msgtype = MID_MESSAGE;
  m->v4.olsr_vtime = ifp->valtimes.mid;

  addrs = m->v4.message.mid.mid_addr;

  /* Don't add the main address... it's already there */
  for (ifs = ifnet; ifs != NULL; ifs = ifs->int_next) {
    if (!ipequal(&olsr_cnf->main_addr, &ifs->ip_addr)) {
#ifdef DEBUG
      struct ipaddr_str buf;
#endif /* DEBUG */

      if ((curr_size + olsr_cnf->ipsize) > remainsize) {
        /* Only add MID message if it contains data */
        if (curr_size > OLSR_MID_IPV4_HDRSIZE) {
#ifdef DEBUG
          OLSR_PRINTF(BMSG_DBGLVL, "Sending partial(size: %d, buff left:%d)\n", curr_size, remainsize);
#endif /* DEBUG */
          /* set size */
          m->v4.olsr_msgsize = htons(curr_size);
          m->v4.seqno = htons(get_msg_seqno()); /* seqnumber */

          net_outbuffer_push(ifp, msg_buffer, curr_size);
          curr_size = OLSR_MID_IPV4_HDRSIZE;
          addrs = m->v4.message.mid.mid_addr;
        }
        net_output(ifp);
        remainsize = net_outbuffer_bytes_left(ifp);
        check_buffspace(curr_size, remainsize, "MID2");
      }
#ifdef DEBUG
      OLSR_PRINTF(BMSG_DBGLVL, "\t%s(%s)\n", olsr_ip_to_string(&buf, &ifs->ip_addr), ifs->int_name);
#endif /* DEBUG */

      addrs->addr = ifs->ip_addr.v4.s_addr;
      addrs++;
      curr_size += olsr_cnf->ipsize;
    }
  }

  m->v4.seqno = htons(get_msg_seqno()); /* seqnumber */
  m->v4.olsr_msgsize = htons(curr_size);

  //printf("Sending MID (%d bytes)...\n", outputsize);
  if (curr_size > OLSR_MID_IPV4_HDRSIZE)
    net_outbuffer_push(ifp, msg_buffer, curr_size);

  return true;
}

/**
 *IP version 6
 *
 *<b>NO INTERNAL BUFFER</b>
 *@param ifp use this interfaces address as main address
 *@return 1 on success
 */

static bool
serialize_mid6(struct interface_olsr *ifp)
{
  uint16_t remainsize, curr_size, needsize;
  /* preserve existing data in output buffer */
  union olsr_message *m;
  struct midaddr6 *addrs6;
  struct interface_olsr *ifs;

  //printf("\t\tGenerating mid on %s\n", ifn->int_name);

  if ((olsr_cnf->ip_version != AF_INET6) || (!ifp) || (ifnet == NULL) || ((ifnet->int_next == NULL) && (ipequal(&olsr_cnf->main_addr, &ifnet->ip_addr))))
    return false;

  remainsize = net_outbuffer_bytes_left(ifp);

  curr_size = OLSR_MID_IPV6_HDRSIZE;

  /* calculate size needed for HNA */
  needsize = curr_size;
  for (ifs = ifnet; ifs != NULL; ifs = ifs->int_next) {
    needsize += olsr_cnf->ipsize*2;
  }

  /* Send pending packet if not room in buffer */
  if (needsize > remainsize) {
    net_output(ifp);
    remainsize = net_outbuffer_bytes_left(ifp);
  }
  check_buffspace(curr_size, remainsize, "MID");

  m = (union olsr_message *)msg_buffer;

  /* Build header */
  m->v6.hopcnt = 0;
  m->v6.ttl = MAX_TTL;
  m->v6.olsr_msgtype = MID_MESSAGE;
  m->v6.olsr_vtime = ifp->valtimes.mid;
  /* Set main(first) address */
  m->v6.originator = olsr_cnf->main_addr.v6;

  addrs6 = m->v6.message.mid.mid_addr;

  /* Don't add the main address... it's already there */
  for (ifs = ifnet; ifs != NULL; ifs = ifs->int_next) {
    if (!ipequal(&olsr_cnf->main_addr, &ifs->ip_addr)) {
#ifdef DEBUG
      struct ipaddr_str buf;
#endif /* DEBUG */
      if ((curr_size + olsr_cnf->ipsize) > remainsize) {
        /* Only add MID message if it contains data */
        if (curr_size > OLSR_MID_IPV6_HDRSIZE) {
#ifdef DEBUG
          OLSR_PRINTF(BMSG_DBGLVL, "Sending partial(size: %d, buff left:%d)\n", curr_size, remainsize);
#endif /* DEBUG */
          /* set size */
          m->v6.olsr_msgsize = htons(curr_size);
          m->v6.seqno = htons(get_msg_seqno()); /* seqnumber */

          net_outbuffer_push(ifp, msg_buffer, curr_size);
          curr_size = OLSR_MID_IPV6_HDRSIZE;
          addrs6 = m->v6.message.mid.mid_addr;
        }
        net_output(ifp);
        remainsize = net_outbuffer_bytes_left(ifp);
        check_buffspace(curr_size + olsr_cnf->ipsize, remainsize, "MID2");
      }
#ifdef DEBUG
      OLSR_PRINTF(BMSG_DBGLVL, "\t%s(%s)\n", olsr_ip_to_string(&buf, &ifs->ip_addr), ifs->int_name);
#endif /* DEBUG */

      addrs6->addr = ifs->ip_addr.v6;
      addrs6++;
      curr_size += olsr_cnf->ipsize;
    }
  }

  m->v6.olsr_msgsize = htons(curr_size);
  m->v6.seqno = htons(get_msg_seqno()); /* seqnumber */

  //printf("Sending MID (%d bytes)...\n", outputsize);
  if (curr_size > OLSR_MID_IPV6_HDRSIZE)
    net_outbuffer_push(ifp, msg_buffer, curr_size);

  return true;
}

static void appendHNAEntry(struct interface_olsr *ifp, struct ip_prefix_list *h, uint16_t * remainsize, uint16_t * curr_size,
    union olsr_message *m, struct hnapair **pair, bool zero
#ifndef __linux__
__attribute__((unused))
#endif
  , bool * sgw_set
#ifndef __linux__
__attribute__((unused))
#endif
  ) {
  union olsr_ip_addr ip_addr;
#ifdef __linux__
  bool is_def_route = is_prefix_inetgw(&h->net);
#endif

#ifdef __linux__
  if (!zero && olsr_cnf->smart_gw_active && is_def_route && smartgw_is_zero_bandwidth(olsr_cnf)) {
    /* this is the default route, with zero bandwidth, do not append it */
    return;
  }
#endif /* __linux__ */

  if ((*curr_size + (2 * olsr_cnf->ipsize)) > *remainsize) {
    /* Only add HNA message if it contains data */
    if (*curr_size > OLSR_HNA_IPV4_HDRSIZE) {
#ifdef DEBUG
      OLSR_PRINTF(BMSG_DBGLVL, "Sending partial(size: %d, buff left:%d)\n", *curr_size, *remainsize);
#endif /* DEBUG */
      m->v4.olsr_msgsize = htons(*curr_size);
      m->v4.seqno = htons(get_msg_seqno());
      net_outbuffer_push(ifp, msg_buffer, *curr_size);
      *curr_size = OLSR_HNA_IPV4_HDRSIZE;
      *pair = m->v4.message.hna.hna_net;
    }
    net_output(ifp);
    *remainsize = net_outbuffer_bytes_left(ifp);
    check_buffspace(*curr_size + (2 * olsr_cnf->ipsize), *remainsize, "HNA2");
  }
#ifdef DEBUG
  OLSR_PRINTF(BMSG_DBGLVL, "\tNet: %s\n", olsr_ip_prefix_to_string(&h->net));
#endif /* DEBUG */

  olsr_prefix_to_netmask(&ip_addr, h->net.prefix_len);
#ifdef __linux__
  if (olsr_cnf->smart_gw_active && is_def_route) {
    /* this is the default route, overwrite it with the smart gateway */
    olsr_modifiy_inetgw_netmask(&ip_addr, h->net.prefix_len, zero);
    *sgw_set = true;
  }
#endif /* __linux__ */
  (*pair)->addr = h->net.prefix.v4.s_addr;
  (*pair)->netmask = ip_addr.v4.s_addr;
  *pair = &(*pair)[1];
  *curr_size += (2 * olsr_cnf->ipsize);
}

/**
 *IP version 4
 *
 *@param h the list of HNAs
 *@param ifp the interface to send on
 *@param is_zero_bw true when the HNA is a 'de-announcing zero-bandwidth' HNA
 *@return nada
 */
static bool
serialize_hna4(struct ip_prefix_list *h, struct interface_olsr *ifp, bool is_zero_bw)
{
  uint16_t remainsize, curr_size, needsize;
  /* preserve existing data in output buffer */
  union olsr_message *m;
  struct hnapair *pair;
  bool h_empty = !h;
  bool sgw_set = false;

  if (ifp == NULL) {
    return false;
  }

  if (olsr_cnf->ip_version != AF_INET) {
    return false;
  }

  if (h_empty && !ifp->sgw_sgw_zero_bw_timeout) {
    return false;
  }

  remainsize = net_outbuffer_bytes_left(ifp);

  curr_size = OLSR_HNA_IPV4_HDRSIZE;

  needsize = curr_size;

  if (!h_empty) {
    /* calculate size needed for HNA */
    struct ip_prefix_list *h_tmp = h;
    while (h_tmp) {
      needsize += olsr_cnf->ipsize*2;
      h_tmp = h_tmp->next;
    }

    /* Send pending packet if not room in buffer */
    if (needsize > remainsize) {
      net_output(ifp);
      remainsize = net_outbuffer_bytes_left(ifp);
    }
    check_buffspace(curr_size, remainsize, "HNA");

    m = (union olsr_message *)msg_buffer;

    /* Fill header */
    m->v4.olsr_msgtype = HNA_MESSAGE;
    m->v4.olsr_vtime = is_zero_bw ? reltime_to_me(ifp->sgw_sgw_zero_bw_timeout) : ifp->valtimes.hna;
    // olsr_msgsize
    m->v4.originator = olsr_cnf->main_addr.v4.s_addr;
    m->v4.ttl = MAX_TTL;
    m->v4.hopcnt = 0;
    // seqno

    pair = m->v4.message.hna.hna_net;

    for (; h != NULL; h = h->next) {
      appendHNAEntry(ifp, h, &remainsize, &curr_size, m, &pair, is_zero_bw, &sgw_set);
    }

    m->v4.olsr_msgsize = htons(curr_size);
    m->v4.seqno = htons(get_msg_seqno());

    net_outbuffer_push(ifp, msg_buffer, curr_size);

#ifdef __linux__
    if (sgw_set && !is_zero_bw) {
      /* (re)set zero bandwidth sgw HNAs timeout */
      ifp->sgw_sgw_zero_bw_timeout = ifp->valtimes.hna_reltime;
    }
#endif
  }

#ifdef __linux__
  if (olsr_cnf->smart_gw_active /* sgw is active */
      && (h_empty || !sgw_set) /* there are no HNAs at all or no sgw HNA */
      && ifp->sgw_sgw_zero_bw_timeout /* the zero bandwidth sgw HNA window is still valid */
      && !is_zero_bw /* prevent infinite recursion */
      ) {
    struct ip_prefix_list h_zero;

    memset(&h_zero, 0, sizeof(h_zero));
    serialize_hna4(&h_zero, ifp, true);

    /* decrement the window validity time */
    {
      unsigned int hna_period = ifp->hna_gen_timer->timer_period;
      if (ifp->sgw_sgw_zero_bw_timeout <= hna_period) {
        ifp->sgw_sgw_zero_bw_timeout = 0;
      } else {
        ifp->sgw_sgw_zero_bw_timeout -= hna_period;
      }
    }
  }
#endif /* __linux__ */

  //printf("Sending HNA (%d bytes)...\n", outputsize);
  return false;
}

static void appendHNA6Entry(struct interface_olsr *ifp, struct ip_prefix_list *h, uint16_t * remainsize, uint16_t * curr_size,
    union olsr_message *m, struct hnapair6 **pair, bool zero
#ifndef __linux__
__attribute__((unused))
#endif
  , bool * sgw_set
#ifndef __linux__
__attribute__((unused))
#endif
  ) {
  union olsr_ip_addr ip_addr;
#ifdef __linux__
  bool is_def_route = is_prefix_inetgw(&h->net);
#endif

#ifdef __linux__
  if (!zero && olsr_cnf->smart_gw_active && is_def_route && smartgw_is_zero_bandwidth(olsr_cnf)) {
    /* this is the default route, with zero bandwidth, do not append it */
    return;
  }
#endif /* __linux__ */

  if ((*curr_size + (2 * olsr_cnf->ipsize)) > *remainsize) {
    /* Only add HNA message if it contains data */
    if (*curr_size > OLSR_HNA_IPV6_HDRSIZE) {
#ifdef DEBUG
      OLSR_PRINTF(BMSG_DBGLVL, "Sending partial(size: %d, buff left:%d)\n", *curr_size, *remainsize);
#endif /* DEBUG */
      m->v6.olsr_msgsize = htons(*curr_size);
      m->v6.seqno = htons(get_msg_seqno());
      net_outbuffer_push(ifp, msg_buffer, *curr_size);
      *curr_size = OLSR_HNA_IPV6_HDRSIZE;
      *pair = m->v6.message.hna.hna_net;
    }
    net_output(ifp);
    *remainsize = net_outbuffer_bytes_left(ifp);
    check_buffspace(*curr_size + (2 * olsr_cnf->ipsize), *remainsize, "HNA2");
  }
#ifdef DEBUG
  OLSR_PRINTF(BMSG_DBGLVL, "\tNet: %s\n", olsr_ip_prefix_to_string(&h->net));
#endif /* DEBUG */

  olsr_prefix_to_netmask(&ip_addr, h->net.prefix_len);
#ifdef __linux__
  if (olsr_cnf->smart_gw_active && is_def_route) {
    /* this is the default route, overwrite it with the smart gateway */
    olsr_modifiy_inetgw_netmask(&ip_addr, h->net.prefix_len, zero);
    *sgw_set = true;
  }
#endif /* __linux__ */
  (*pair)->addr = h->net.prefix.v6;
  (*pair)->netmask = ip_addr.v6;
  *pair = &(*pair)[1];
  *curr_size += (2 * olsr_cnf->ipsize);
}

/**
 *IP version 6
 *
 *@param h the list of HNAs
 *@param ifp the interface to send on
 *@param is_zero_bw true when the HNA is a 'de-announcing zero-bandwidth' HNA
 *@return nada
 */
static bool
serialize_hna6(struct ip_prefix_list *h, struct interface_olsr *ifp, bool is_zero_bw)
{
  uint16_t remainsize, curr_size, needsize;
  /* preserve existing data in output buffer */
  union olsr_message *m;
  struct hnapair6 *pair;
  bool h_empty = !h;
  bool sgw_set = false;

  if (ifp == NULL) {
    return false;
  }

  if (olsr_cnf->ip_version != AF_INET6) {
    return false;
  }

  if (h_empty && !ifp->sgw_sgw_zero_bw_timeout) {
    return false;
  }

  remainsize = net_outbuffer_bytes_left(ifp);

  curr_size = OLSR_HNA_IPV6_HDRSIZE;

  needsize = curr_size;

  if (!h_empty) {
    /* calculate size needed for HNA */
    struct ip_prefix_list *h_tmp = h;
    while (h_tmp) {
      needsize += olsr_cnf->ipsize*2;
      h_tmp = h_tmp->next;
    }

    /* Send pending packet if not room in buffer */
    if (needsize > remainsize) {
      net_output(ifp);
      remainsize = net_outbuffer_bytes_left(ifp);
    }
    check_buffspace(curr_size, remainsize, "HNA");

    m = (union olsr_message *)msg_buffer;

    /* Fill header */
    m->v6.olsr_msgtype = HNA_MESSAGE;
    m->v6.olsr_vtime = is_zero_bw ? reltime_to_me(ifp->sgw_sgw_zero_bw_timeout) : ifp->valtimes.hna;
    // olsr_msgsize
    m->v6.originator = olsr_cnf->main_addr.v6;
    m->v6.ttl = MAX_TTL;
    m->v6.hopcnt = 0;
    // seqno

    pair = m->v6.message.hna.hna_net;

    for (; h != NULL; h = h->next) {
      appendHNA6Entry(ifp, h, &remainsize, &curr_size, m, &pair, is_zero_bw, &sgw_set);
    }

    m->v6.olsr_msgsize = htons(curr_size);
    m->v6.seqno = htons(get_msg_seqno());

    net_outbuffer_push(ifp, msg_buffer, curr_size);

#ifdef __linux__
    if (sgw_set && !is_zero_bw) {
      /* (re)set zero bandwidth sgw HNAs timeout */
      ifp->sgw_sgw_zero_bw_timeout = ifp->valtimes.hna_reltime;
    }
#endif
  }

#ifdef __linux__
  if (olsr_cnf->smart_gw_active /* sgw is active */
      && (h_empty || !sgw_set) /* there are no HNAs at all or no sgw HNA */
      && ifp->sgw_sgw_zero_bw_timeout /* the zero bandwidth sgw HNA window is still valid */
      && !is_zero_bw /* prevent infinite recursion */
      ) {
    struct ip_prefix_list h_zero;

    memset(&h_zero, 0, sizeof(h_zero));
    serialize_hna6(&h_zero, ifp, true);

    /* decrement the window validity time */
    {
      unsigned int hna_period = ifp->hna_gen_timer->timer_period;
      if (ifp->sgw_sgw_zero_bw_timeout <= hna_period) {
        ifp->sgw_sgw_zero_bw_timeout = 0;
      } else {
        ifp->sgw_sgw_zero_bw_timeout -= hna_period;
      }
    }
  }
#endif /* __linux__ */

  //printf("Sending HNA (%d bytes)...\n", outputsize);
  return false;
}

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
