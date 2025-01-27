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

#ifdef __linux__
#ifdef LINUX_NL80211 /* Optional - not supported on all platforms */

#include "lq_plugin_ffeth_nl80211.h"
#include "tc_set.h"
#include "link_set.h"
#include "lq_plugin.h"
#include "olsr_spf.h"
#include "lq_packet.h"
#include "packet.h"
#include "olsr.h"
#include "parser.h"
#include "fpm.h"
#include "mid_set.h"
#include "scheduler.h"
#include "log.h"

#ifdef LINUX_NL80211
#include "nl80211_link_info.h"
#define WEIGHT_ETX 50
#define WEIGHT_BANDWIDTH 50
#endif

#define LQ_PLUGIN_LC_MULTIPLIER 1024
#define LQ_PLUGIN_RELEVANT_COSTCHANGE_FF 16

static void lq_initialize_ffeth_nl80211(void);

static olsr_linkcost lq_calc_cost_ffeth_nl80211(const void *lq);

static void lq_packet_loss_worker_ffeth_nl80211(struct link_entry *link, void *lq, bool lost);
static void lq_memorize_foreign_hello_ffeth_nl80211(void *local, void *foreign);

static int lq_serialize_hello_lq_pair_ffeth_nl80211(unsigned char *buff, void *lq);
static void lq_deserialize_hello_lq_pair_ffeth_nl80211(const uint8_t **curr, void *lq);
static int lq_serialize_tc_lq_pair_ffeth_nl80211(unsigned char *buff, void *lq);
static void lq_deserialize_tc_lq_pair_ffeth_nl80211(const uint8_t **curr, void *lq);

static void lq_copy_link2neigh_ffeth_nl80211(void *t, void *s);
static void lq_copy_link2tc_ffeth_nl80211(void *target, void *source);
static void lq_clear_ffeth_nl80211(void *target);
static void lq_clear_ffeth_nl80211_hello(void *target);

static const char *lq_print_ffeth_nl80211(void *ptr, char separator, struct lqtextbuffer *buffer);
static double lq_print_cost_ffeth_nl80211(olsr_linkcost cost);

/* etx lq plugin (freifunk fpm version) settings */
struct lq_handler lq_etx_ffeth_nl80211_handler = {
    &lq_initialize_ffeth_nl80211,
    &lq_calc_cost_ffeth_nl80211,
    &lq_calc_cost_ffeth_nl80211,

    &lq_packet_loss_worker_ffeth_nl80211,

    &lq_memorize_foreign_hello_ffeth_nl80211,
    &lq_copy_link2neigh_ffeth_nl80211,
    &lq_copy_link2tc_ffeth_nl80211,
    &lq_clear_ffeth_nl80211_hello,
    &lq_clear_ffeth_nl80211,

    &lq_serialize_hello_lq_pair_ffeth_nl80211,
    &lq_serialize_tc_lq_pair_ffeth_nl80211,
    &lq_deserialize_hello_lq_pair_ffeth_nl80211,
    &lq_deserialize_tc_lq_pair_ffeth_nl80211,

    &lq_print_ffeth_nl80211,
    &lq_print_ffeth_nl80211,
    &lq_print_cost_ffeth_nl80211,

    sizeof(struct lq_ffeth_hello),
    sizeof(struct lq_ffeth),
    4,
    4};

uint8_t nl80211_lq_window = LQ_FFETH_WINDOW;
// added for trend
uint8_t trend_lq_window = TREND_WINDOW;
uint8_t rssi_lq_window = RSSI_WINDOW;

static void
lq_ffeth_nl80211_handle_lqchange(void)
{
  struct lq_ffeth_hello *lq;
  struct ipaddr_str buf;
  struct link_entry *link;

  bool triggered = false;
  // OLSR_PRINTF(1,"In handle change ---\n ");
  OLSR_FOR_ALL_LINK_ENTRIES(link)
  {
    bool relevant = false;
    lq = (struct lq_ffeth_hello *)link->linkquality;

#if 0
  fprintf(stderr, "%s: old = %u/%u   new = %u/%u\n", olsr_ip_to_string(&buf, &link->neighbor_iface_addr),
      lq->smoothed_lq.valueLq, lq->smoothed_lq.valueNlq,
      lq->lq.valueLq, lq->lq.valueNlq);
#endif
    ////struct ipaddr_str buflink1;
    ///     OLSR_PRINTF(1,"value LQ for %s is %d \n",olsr_ip_to_string(&buflink1,&link->neighbor_iface_addr),lq->lq.valueLq);

    if (lq->smoothed_lq.valueLq < lq->lq.valueLq)
    {
      if (lq->lq.valueLq >= 254 || lq->lq.valueLq - lq->smoothed_lq.valueLq > lq->smoothed_lq.valueLq / 10)
      {
        relevant = true;
      }
    }
    else if (lq->smoothed_lq.valueLq > lq->lq.valueLq)
    {
      if (lq->smoothed_lq.valueLq - lq->lq.valueLq > lq->smoothed_lq.valueLq / 10)
      {
        relevant = true;
      }
    }
    if (lq->smoothed_lq.valueNlq < lq->lq.valueNlq)
    {
      if (lq->lq.valueNlq >= 254 || lq->lq.valueNlq - lq->smoothed_lq.valueNlq > lq->smoothed_lq.valueNlq / 10)
      {
        relevant = true;
      }
    }
    else if (lq->smoothed_lq.valueNlq > lq->lq.valueNlq)
    {
      if (lq->smoothed_lq.valueNlq - lq->lq.valueNlq > lq->smoothed_lq.valueNlq / 10)
      {
        relevant = true;
      }
    }
    else if (lq->smoothed_lq.valueLq == lq->lq.valueLq)
    {
      if (lq->lq.valueLq >= 254)
      {
        relevant = true;
      }
    }

    // update set relevant to true as long as LQ and NLQ are not 0
    if(lq->smoothed_lq.valueLq != 0 && lq->smoothed_lq.valueNlq !=0 ){
      relevant =true;
    }

    if (relevant)
    {
      memcpy(&lq->smoothed_lq, &lq->lq, sizeof(struct lq_ffeth));

      struct ipaddr_str buflink;
      // OLSR_PRINTF(1, "Cost for %s", olsr_ip_to_string(&buflink, &link->neighbor_iface_addr));
      olsr_linkcost sampledLC = lq_calc_cost_ffeth_nl80211(&lq->smoothed_lq);

      // OLSR_PRINTF(1,"In NL80211 Relevant- Trend: %d, previous cost %d, sampled cost %d, ",lq->smoothed_lq.valueBandwidth, link->linkcost,sampledLC);
    
      triggered = true;

      if (link->linkcost==0){
          link->linkcost=sampledLC;
        }

        // if there's a trend, set the neighbor cost as the current cost 
        // sampledLC is usually in the range up to 10 or 11 based on the max penalty value of 7 and the LQ cost (converges to 1)
        if((lq->smoothed_lq.valueBandwidth & 0x03)!=1){
              link->linkcost = sampledLC;
            }
        // else if we don't have a trend, degrade the cost gradually 
        else{

            // if the current cost is higher than 10.x, set the neighbor cost and the sampledLC
            if(sampledLC > 41000)
            {
              link->linkcost= sampledLC;
            }
            // if the previous neighbor cost is high, set it to the current value
            // this is important for when we suddenly drop the sampledLC cost 
            if(link->linkcost > 41000)
            {
              link->linkcost= sampledLC;
            }
            // in this case, if the sampled is higher than 10, the neigh cost will be the same value
            link->linkcost = (0.99*link->linkcost) +(0.01*sampledLC);
        }

          // OLSR_PRINTF(1,"weighted cost: %f \n",link->linkcost*1.0/1024.0);



    }
  }
  OLSR_FOR_ALL_LINK_ENTRIES_END(link)

  if (!triggered)
  {
    return;
  }

  OLSR_FOR_ALL_LINK_ENTRIES(link)
  {
    lq = (struct lq_ffeth_hello *)link->linkquality;

    if (lq->smoothed_lq.valueLq >= 254 && lq->smoothed_lq.valueNlq >= 254)
    {
      continue;
    }

    if (lq->smoothed_lq.valueLq == lq->lq.valueLq && lq->smoothed_lq.valueNlq == lq->lq.valueNlq)
    {
      continue;
    }

    memcpy(&lq->smoothed_lq, &lq->lq, sizeof(struct lq_ffeth));

    struct ipaddr_str buflink;
    // OLSR_PRINTF(1, "Cost for %s ", olsr_ip_to_string(&buflink, &link->neighbor_iface_addr));
    olsr_linkcost sampledLC = lq_calc_cost_ffeth_nl80211(&lq->smoothed_lq);

      // OLSR_PRINTF(1,"In NL80211 Triggered- Trend: %d, previous Cost %d, sampled cost %d, ",lq->smoothed_lq.valueBandwidth, link->linkcost,sampledLC);
   
    if (link->linkcost==0){
      link->linkcost=sampledLC;
    }

    // if there's a trend, set the neighbor cost as the current cost 
    // sampledLC is usually in the range up to 10 or 11 based on the max penalty value of 7 and the LQ cost (converges to 1)
    if((lq->smoothed_lq.valueBandwidth & 0x03)!=1){
        link->linkcost = sampledLC;
    }	
    // else if we don't have a trend, degrade the cost gradually 
    else{

        // if the current cost is higher than 10.x, set the neighbor cost and the sampledLC
        if(sampledLC > 41000)
        {
          link->linkcost= sampledLC;
        }
        // if the previous neighbor cost is high, set it to the current value
        // this is important for when we suddenly drop the sampledLC cost 
        if(link->linkcost > 41000)
        {
          link->linkcost= sampledLC;
        }
        // in this case, if the sampled is higher than 10, the neigh cost will be the same value
        link->linkcost = (0.99*link->linkcost) +(0.01*sampledLC);
    }

          // OLSR_PRINTF(1,"weighted cost: %f \n",link->linkcost*1.0/1024.0);


  }
  OLSR_FOR_ALL_LINK_ENTRIES_END(link)

  olsr_relevant_linkcost_change();
}

static void
lq_parser_ffeth_nl80211(struct olsr *olsr, struct interface_olsr *in_if, union olsr_ip_addr *from_addr)
{
  const union olsr_ip_addr *main_addr;
  struct link_entry *lnk;
  struct lq_ffeth_hello *lq;
  uint32_t seq_diff;

  // OLSR_PRINTF(1,"At %s Msg Type, %u\n", olsr_wallclock_string(), olsr->olsr_msg->olsr_msgtype);
  /* Find main address */
  main_addr = mid_lookup_main_addr(from_addr);

  /* Loopup link entry */
  lnk = lookup_link_entry(from_addr, main_addr, in_if);
  if (lnk == NULL)
  {
    return;
  }

  lq = (struct lq_ffeth_hello *)lnk->linkquality;

  /* ignore double package */
  if (lq->last_seq_nr == olsr->olsr_seqno)
  {
    struct ipaddr_str buf;
    olsr_syslog(OLSR_LOG_INFO, "detected duplicate packet with seqnr %d from %s on %s (%d Bytes)",
                olsr->olsr_seqno, olsr_ip_to_string(&buf, from_addr), in_if->int_name, ntohs(olsr->olsr_packlen));
    return;
  }

  if (lq->last_seq_nr > olsr->olsr_seqno)
  {
    seq_diff = (uint32_t)olsr->olsr_seqno + 65536 - lq->last_seq_nr;
  }
  else
  {
    seq_diff = olsr->olsr_seqno - lq->last_seq_nr;
  }

  /* Jump in sequence numbers ? */
  if (seq_diff > 256)
  {
    seq_diff = 1;
  }

  lq->received[lq->activePtr]++;
  lq->total[lq->activePtr] += seq_diff;

  lq->last_seq_nr = olsr->olsr_seqno;
  lq->missed_hellos = 0;
}

static void
lq_ffeth_nl80211_timer(void __attribute__((unused)) * context)
{
  struct link_entry *link;

#ifdef LINUX_NL80211
  nl80211_link_info_get();
#endif

  OLSR_FOR_ALL_LINK_ENTRIES(link)
  {
    struct lq_ffeth_hello *tlq = (struct lq_ffeth_hello *)link->linkquality;
    fpm ratio;
    //,ratio_instant,ratio_est,ratio_dev,ratio_calculated;
    int i, received, total;

    received = 0;

    total = 0;

    // initialize the trend_counter to zero at quick start 
    if(tlq->twindowSize ==LQ_FFETH_QUICKSTART_INIT){
      tlq->trend_prev = -2;
      tlq->bufferPtr = 0;

      tlq->trend_counter=0;
    }

    /* enlarge window if still in quickstart phase */
    if (tlq->windowSize < nl80211_lq_window)
    {
      tlq->windowSize++;
    }

    /* enlarge window if still in quickstart phase */
    if (tlq->twindowSize < trend_lq_window)
    {
      tlq->twindowSize++;
            tlq->trend_counter=0;

    }


    for (i = 0; i < tlq->windowSize; i++)
    {
      received += tlq->received[i];
      total += tlq->total[i];
    }

    /* calculate link quality */
    if (total == 0)
    {
      tlq->lq.valueLq = 0;
    }
    else
    {
      // start with link-loss-factor
      ratio = fpmidiv(itofpm(link->loss_link_multiplier), LINK_LOSS_MULTIPLIER);
      // to initialize the second ratio metric
      // ratio_instant= fpmidiv(itofpm(link->loss_link_multiplier), LINK_LOSS_MULTIPLIER);
      /* keep missed hello periods in mind (round up hello interval to seconds) */
      if (tlq->missed_hellos > 1)
      {
        received = received - received * tlq->missed_hellos * link->inter->hello_etime / 1000 / nl80211_lq_window;
      }

      // calculate received/total factor
      // multiple with the link-loss value

      // after change this considers all elements except the last one
      ratio = fpmmuli(ratio, received);
      // divide by the total
      ratio = fpmidiv(ratio, total);
      // normalize with 255
      ratio = fpmmuli(ratio, 255);

      ///// the following is for the last element only

      // ratio_instant = fpmmuli(ratio_instant, tlq->received[tlq->windowSize-1]);
      //  divide by the total
      // ratio_instant = fpmidiv(ratio_instant, tlq->total[tlq->windowSize-1]);
      //  normalize with 255
      //  = fpmmuli(ratio_instant, 255);

      // to normalize both previous and instant
      // 0.75 * previous + 0.25 * instant
      // ratio_est = fpmadd(fpmidiv(fpmmuli(ratio,75),100),fpmidiv(fpmmuli(ratio_instant,25,100)));
      // calculate the deviation
      // ratio_dev = fpmadd(fpmidiv(fpmmuli(ratio,75),100),fpmidiv(fpmmuli(fpmsub(ratio_est,ratio_instant),25,100)));

      // ratio_calculated =

      // set the value in the TLQ
      tlq->lq.valueLq = (uint8_t)(fpmtoi(ratio));
    }

    /* ethernet booster */
    if (link->inter->mode == IF_MODE_ETHER)
    {
      if (tlq->lq.valueLq > (uint8_t)(0.95 * 255))
      {
        tlq->perfect_eth = true;
      }
      else if (tlq->lq.valueLq > (uint8_t)(0.90 * 255))
      {
        tlq->perfect_eth = false;
      }

      if (tlq->perfect_eth)
      {
        tlq->lq.valueLq = 255;
      }
    }
    else if (link->inter->mode != IF_MODE_ETHER && tlq->lq.valueLq > 0)
    {
      tlq->lq.valueLq--;
    }

    // RSSI window
    // this puts directly the rssi to the window
    // tlq->rssi[tlq->ractivePtr] = link->rssi * -1;

    // what we want is to avg the prev last 5 entries and put them in the new rssi window

    // OLSR_PRINTF(1,"Value to add to the full RSSI window: %d \n",MAX_RSSI);
    // we start from the current rssi value and add the previous 4 values 
	  int sum_rssi=0, count=0;
    uint8_t avg_rssi=0;
    // only when negative RSSI are being read
    if(link->rssi <0){
        // OLSR_PRINTF(1,"/n New RSSI: %d\n",link->rssi);
        /* enlarge window if still in quickstart phase */
        if (tlq->rwindowSize < rssi_lq_window)
        {
          tlq->rwindowSize++;
        }
        // OLSR_PRINTF(1,"rssi wnd size = %d\n", tlq->rwindowSize);
        // only for values above -100  since the current threshold is 
         if((link->rssi*-1)<MAX_RSSI){
          sum_rssi=link->rssi*-1;
          count=1;
         }
        
        // OLSR_PRINTF(1,"%d, ",sum_rssi);
      
        for(int i=tlq->rwindowSize-5; i<tlq->rwindowSize-1; i++)
        {
          uint8_t ptr = (tlq->ractivePtr+1+i)%tlq->rwindowSize;
          // OLSR_PRINTF(1,"%d, ",tlq->rssi[ptr]);
          if(tlq->rssi[ptr]!=0 && tlq->rssi[ptr]<MAX_RSSI){
            sum_rssi=sum_rssi+tlq->rssi[ptr];
            count++;
          }
        }
        
        
        float avg_r ;
        if(count!=0)
        {
          avg_r =(sum_rssi*1.0)/(count*1.0);
          avg_rssi=(uint8_t)round(avg_r);
        }

        tlq->rssi[tlq->ractivePtr] = avg_rssi;
        
        // OLSR_PRINTF(1,"\n average rssi = %d \n",avg_rssi);
    }
    // OLSR_PRINTF(1,"printing rssi to check for 250 %d ",tlq->rssi[tlq->activePtr]);

    // if (tlq->rssi[tlq->ractivePtr] > 250)
    // {
    //   OLSR_PRINTF(1, "Something is wrong when reading rssi");
    // }
    tlq->lqWin[tlq->activePtr] = tlq->lq.valueLq;

    //*************************Computation RSSI window*********************************
    //*********************************************************************************
    int sumconc = 0;
    int sumdis = 0;
    int diff=2;
    // OLSR_PRINTF(1, "Full RSSI window: ");

    int samplesNo = 0;
    for (int i = 0; i < tlq->rwindowSize - 1; i++)
    {

      uint8_t ptr = (tlq->ractivePtr + 1 + i) % tlq->rwindowSize;
      // OLSR_PRINTF(1, "%d, ", tlq->rssi[ptr]);

      if (tlq->rssi[ptr] == 0)
        continue;

      samplesNo = samplesNo + 1;
      int concordants = 0;
      int discordants = 0;
      for (int j = i + 1; j < tlq->rwindowSize - 1; j++)
      {
        uint8_t ptrcomp = (tlq->ractivePtr + 1 + j) % tlq->rwindowSize;
        if (tlq->rssi[ptrcomp] == 0)
          continue;

        if (tlq->rssi[ptr] > tlq->rssi[ptrcomp])
        {
          if((tlq->rssi[ptr]-tlq->rssi[ptrcomp])>=diff)
            discordants++;
        }
        if (tlq->rssi[ptr] < tlq->rssi[ptrcomp])
        {
          if((tlq->rssi[ptrcomp]-tlq->rssi[ptr])>=diff)
            concordants++;
        }
      }
      sumconc += concordants;
      sumdis += discordants;
    }
    // OLSR_PRINTF(1, "\n");

    // float tau = (1.0 * (sumconc - sumdis)) / (1.0 * (sumconc + sumdis));

    float tau = (1.0 * (sumconc - sumdis)) / (1.0 * (samplesNo*(samplesNo-1)/2.0));
    double zScore = abs((3.0 * tau * sqrt(samplesNo * (samplesNo - 1))) / (sqrt(2.0 * (2.0 * samplesNo + 5.0))));

    double a1 = 0.254829592;
    double a2 = -0.284496736;
    double a3 = 1.421413741;
    double a4 = -1.453152027;
    double a5 = 1.061405429;
    double p = 0.3275911;

    zScore = (zScore) / sqrt(2.0);

    // A&S formula 7.1.26
    double t = 1.0 / (1.0 + p * zScore);
    double y = 1.0 - (((((a5 * t + a4) * t) + a3) * t + a2) * t + a1) * t * exp(-zScore * zScore);

    double pValue = 0.5 * (1.0 + y);
    pValue = 2 - (2 * pValue);

    // trend is the current trend value, which we add to the window of trends
    int trend = 0;
    if (pValue < 0.05)
    {
      trend = 1;
    }
    if (samplesNo == 0)
    {
      trend = 0;
    }
    tlq->tau = tau;
    // we put the trend 0 or 1 to the window of trends
    tlq->trend_w[tlq->tactivePtr] = trend;
    
    // if the trend is negative, overwrite the window trend with the signed trend
    if (tau<0){
      tlq->trend_w[tlq->tactivePtr] = trend*-1;
    }

    // for trend to be determined based on the window of trends
    // OLSR_PRINTF(1, "trend window:");
    int maj_trend = 0;
    int p_count=0;
    int n_count=0;
    for (int i = 0; i < tlq->twindowSize; i++)
    {
      uint8_t ptr = (tlq->tactivePtr + 1 + i) % tlq->twindowSize;
      // OLSR_PRINTF(1, "%d, ", tlq->trend_w[ptr]);
      // this is adding the trend with the sign, which makes moving away and towards cancel each other
      maj_trend = maj_trend + abs(tlq->trend_w[ptr]);
      if(tlq->trend_w[ptr]>0){
        p_count=p_count+1;
      }
      else if(tlq->trend_w[ptr]<0){
        n_count=n_count+1;
      }
    }

    // OLSR_PRINTF(1, "\n");

    if (abs(maj_trend) >= tlq->twindowSize/2)
    {
      // set the trend in tlq based on the majority voting and reset the trend variable for the next parts
      tlq->trend = 1;
      trend = 1;
      // link->mob_trend=1;
    }
    else
    {
      tlq->trend = 0;
      trend = 0;
    }


    if (tlq->trend_prev == -2){
      tlq->trend_prev = trend;
      tlq->tau_prev = tau;
    }
    else{
      if(tlq->bufferPtr == BUFFER_WINDOW){
        // OLSR_PRINTF(1,"\nReseting the pointer!\n");
        tlq->trend_prev = trend;
        tlq->tau_prev = tau;
        tlq->bufferPtr = 0;
      }
      else if((abs(tlq->trend_prev) - abs(trend))>0){
        // OLSR_PRINTF(1,"\n A change from 1 to 0!\n");
        tlq->trend_buffer[tlq->bufferPtr] = trend;
        tlq->tau_buffer[tlq->bufferPtr] = tau;
        trend = tlq->trend_prev;
        tau = tlq->tau_prev;

        // tlq->trend_prev = tlq->trend_buffer[bufferPtr]
        // tlq->tau_prev = tlq->tau_buffer[bufferPtr]
        
        tlq->bufferPtr++;

        // if(tlq->buffer)
        //make sure we are not at the end of the buffer counter 
      }
      else if (tlq->trend_prev  == trend && ((tau>0 && tlq->tau_prev<0) || (tau<0 && tlq->tau_prev>0))){
        // OLSR_PRINTF(1,"\n A change from 1 to -1 or vise versa!\n");

        tlq->trend_buffer[tlq->bufferPtr] = trend;
        tlq->tau_buffer[tlq->bufferPtr] = tau;
        trend = tlq->trend_prev;
        tau = tlq->tau_prev;

        // tlq->trend_prev = tlq->trend_buffer[bufferPtr]
        // tlq->tau_prev = tlq->tau_buffer[bufferPtr]
        
        tlq->bufferPtr++;

        // if(tlq->buffer)
        //make sure we are not at the end of the buffer counter 
        // if (tlq->bufferPtr < BUFFER_WINDOW){
        //   tlq->trend_buffer[bufferPtr] = trend;
        //   tlq->tau_buffer[bufferPtr] = tau;
        //   trend = tlq->trend_prev;
        //   tau = tlq->tau_prev;

        //   tlq->trend_prev = tlq->trend_buffer[bufferPtr]
        //   tlq->tau_prev = tlq->tau_buffer[bufferPtr]
        //   tlq->bufferPtr++;
        // }
      }
      else{
        tlq->trend_prev = trend;
        tlq->tau_prev = tau;
        tlq->bufferPtr = 0;
      }

    }



    // Save the tau and trend in valueBandwidth uint8
    // 1 bit(least significant bit) for sign and 7 bits for tau
    // valueBandwidth is 0 if trend is 0, otherwise it contains the value of tau and its sign.
    uint8_t trend_and_tau = 0, modified_tau = 0, const_one = 4;

    if (trend == 0)
    {
      trend_and_tau = 0;
    }
    else if (trend == 1)
    {
      // if negative don't add anything
      // OLSR_PRINTF(1,"Setting trend based on sign positive trends %d while negative trends %d",p_count,n_count);
      if (p_count >n_count)
      {
        trend_and_tau = trend_and_tau + const_one;
      }
      if (tau < 0)
        tau = tau * -1.0;
      modified_tau = ((uint8_t)(tau * 32.0));
      modified_tau = modified_tau << 3;
      trend_and_tau = modified_tau + trend_and_tau;
    }
    trend_and_tau = trend_and_tau + get_prediction(&(link->neighbor_iface_addr));
    // OLSR_PRINTF(1,"The tau i am printing: %.3f\n",tau);
    // OLSR_PRINTF(1,"The trend i am printing: %d\n",trend);
    // OLSR_PRINTF(1,"The trend and tau: %d\n",trend_and_tau);

    // if trend (which is majority) == 0 and sampled (last added to window) == 0, or trend == 1 and maj_trend > window/2 
    // otherwise trend == 0 and maj >= windo/2 or trend == 1 and maj < window/2 don't set 
    if(trend==abs(tlq->trend_w[tlq->tactivePtr])){
        tlq->lq.valueBandwidth = trend_and_tau;
        tlq->smoothed_lq.valueBandwidth = trend_and_tau;
    }
    struct ipaddr_str bufprint;
    // OLSR_PRINTF(1, " Calculating trend: %d, tau: %.3f for link: %s\n", trend, tau, olsr_ip_to_string(&bufprint, &link->neighbor_iface_addr));

    // OLSR_PRINTF(1,"RSSI Window: %d, %d\n",link->rssi,tlq->rssi[tlq->activePtr]);
    // OLSR_PRINTF(1,"LQ Window: %d, %d\n",tlq->lq.valueLq, tlq->lqWin[tlq->activePtr]);

    /////////////////To Log the RSSI values //////////////////////////////////////////////////
    /*struct ipaddr_str bufxxx,buffile;
    FILE *fptr;
    char filename[100] = "rssi.txt";
    sprintf(filename,"rssi_%s.txt",olsr_ip_to_string(&buffile, &link->neighbor_iface_addr));

    const char* filename2 = (const char*) filename;
    fptr = fopen(filename2,"a");

    fprintf(
      fptr,"%s %s %d %d %d %d %d %d\n",
      olsr_wallclock_string(),
      olsr_ip_to_string(&bufxxx, &link->neighbor_iface_addr),
      link->rssi,
      total,
      received,
      tlq->received[tlq->activePtr],
      tlq->lq.valueLq ,
      tlq->lq.valueNlq
    );
    fclose(fptr);*/
    /////////////////////////////////////////////////////////////////

    // shift buffer
    tlq->activePtr = (tlq->activePtr + 1) % nl80211_lq_window;
    tlq->total[tlq->activePtr] = 0;
    tlq->received[tlq->activePtr] = 0;

    tlq->lqWin[tlq->activePtr] = 0;

    // update //most probably unnecessary
    //  clearing rssi
    if(link->rssi<0){
      tlq->ractivePtr = (tlq->ractivePtr + 1) % rssi_lq_window;
      tlq->rssi[tlq->ractivePtr] = 0;
    }
    // clearing trend
    tlq->tactivePtr = (tlq->tactivePtr + 1) % trend_lq_window;
    tlq->trend_w[tlq->tactivePtr] = 0;
  }
  OLSR_FOR_ALL_LINK_ENTRIES_END(link);

  lq_ffeth_nl80211_handle_lqchange();
}

static void
lq_initialize_ffeth_nl80211(void)
{
  if (olsr_cnf->lq_nat_thresh < 1.0f)
  {
    fprintf(stderr, "Warning, nat_treshold < 1.0 is more likely to produce loops with etx_ffeth\n");
  }

  if (olsr_cnf->ffeth_nl80211_window)
  {
    nl80211_lq_window = olsr_cnf->ffeth_nl80211_window;
  }

  printf("WINDOW SIZE %hhd\n", nl80211_lq_window);

  olsr_packetparser_add_function(&lq_parser_ffeth_nl80211);
  olsr_start_timer(1000, 0, OLSR_TIMER_PERIODIC, &lq_ffeth_nl80211_timer, NULL, 0);

#ifdef LINUX_NL80211
  nl80211_link_info_init();
#endif
}

static olsr_linkcost
lq_calc_cost_ffeth_nl80211(const void *ptr)
{
  const struct lq_ffeth *lq = ptr;
  // extract the value and sign of tau of
  uint8_t prediction;
  uint8_t sign, atau;
  float tauf;
  if (lq->valueBandwidth == 0)
  {
    tauf = 0;
  }
  else
  {
    sign = lq->valueBandwidth & 0x04;
    atau = (lq->valueBandwidth & 0xF8) >> 3;
    tauf = (float)(1.0 * atau / 32.0);
    if (sign == 0)
    {
      tauf = tauf * -1.0;
    }
  }
  prediction = lq->valueBandwidth & 0x03;
  OLSR_PRINTF(1,"The predition I got from the message: %d\n",prediction);

  olsr_linkcost cost;
  bool ether;
  int lq_int, nlq_int;
#ifdef LINUX_NL80211
  // if we are using the plugin, normalize the value RSSI and then convert it to fpm

  fpm nl80211 = fpmidiv(itofpm(lq->valueRSSI), 100);

#endif

  // MINIMAL_USEFUL_LQ is a float, multiplying by 255 converts it to uint8_t
  if (lq->valueLq < (unsigned int)(255 * MINIMAL_USEFUL_LQ) || lq->valueNlq < (unsigned int)(255 * MINIMAL_USEFUL_LQ))
  {
    return LINK_COST_BROKEN;
  }

  ether = lq->valueLq == 255 && lq->valueNlq == 255;

  lq_int = (int)lq->valueLq;
  if (lq_int > 0 && lq_int < 255)
  {
    lq_int++;
  }

  nlq_int = (int)lq->valueNlq;
  if (nlq_int > 0 && nlq_int < 255)
  {
    nlq_int++;
  }

#ifdef LINUX_NL80211
  cost = fpmidiv(itofpm(255 * 255), lq_int * nlq_int); // 1 / (LQ * NLQ)
  // OLSR_PRINTF(1, "\n LQ cost: %.3f, ", fpmtod(cost));

#else
  cost = fpmidiv(itofpm(255 * 255), lq_int * nlq_int); // 1 / (LQ * NLQ)
#endif
  if (ether)
  {
    /* ethernet boost */
    cost /= 10;
  }


  // we are adding a penalty based on the normalized average rssi value 
  // OLSR_PRINTF(1, "Normalized RSSI: %.3f,  lq->valueBandwidth %d ", fpmtof(nl80211),lq->valueBandwidth);
  
  // if a node is moving away, penalize with 6 times the normalized rssi value
  //update ML

  
  // if (lq->valueBandwidth != 0 && tauf > 0)
  // {
  //   cost = fpmadd(cost, fpmmuli(nl80211,20));
  //   // cost = fpmadd(cost, nl80211);
  //   // cost =fpmmul(fpmmuli(cost, 5),ftofpm(tauf));
  //     // OLSR_PRINTF(1, "moving away, ");
  // }
  if (prediction==2)
    cost = fpmadd(cost, fpmmuli(nl80211,20));

  // if a node is moving in, penalize with 1 factor of the normalized rssi value to differentiate static and moving towards node
  // else if (lq->valueBandwidth != 0 && tauf < 0)
  // { 
  //   // OLSR_PRINTF(1, "moving towards, ");
  //   // cost = fpmadd(cost,ftofpm(tauf*-1.0));
  //   cost = fpmadd(cost, fpmmuli(nl80211, 2));
  // }
  else if(prediction==0)
    cost = fpmadd(cost, fpmmuli(nl80211,2));
  


  // OLSR_PRINTF(1, " new cost: %.3f\n", fpmtod(cost));

  if (cost > LINK_COST_BROKEN)
    return LINK_COST_BROKEN;
  if (cost == 0)
    return 1;
  return cost;
}

static int
lq_serialize_hello_lq_pair_ffeth_nl80211(unsigned char *buff, void *ptr)
{
  struct lq_ffeth *lq = ptr;

#ifdef LINUX_NL80211
  buff[0] = lq->valueBandwidth;
  buff[1] = lq->valueRSSI;
  // buff[2] = lq->prediction;

#else
  buff[0] = (unsigned char)(0);
  buff[1] = (unsigned char)(0);
  // buff[2] = (unsigned char)(0);

#endif
  buff[2] = (unsigned char)lq->valueLq;
  buff[3] = (unsigned char)lq->valueNlq;

  return 4;
}

static void
lq_deserialize_hello_lq_pair_ffeth_nl80211(const uint8_t **curr, void *ptr)
{
  struct lq_ffeth *lq = ptr;

#ifdef LINUX_NL80211
  pkt_get_u8(curr, &lq->valueBandwidth);
  pkt_get_u8(curr, &lq->valueRSSI);
  // pkt_get_u8(curr, &lq->prediction);

#else
  pkt_ignore_u16(curr);
  // pkt_ignore_u8(curr);

#endif
  pkt_get_u8(curr, &lq->valueLq);
  pkt_get_u8(curr, &lq->valueNlq);
}

static int
lq_serialize_tc_lq_pair_ffeth_nl80211(unsigned char *buff, void *ptr)
{
  struct lq_ffeth *lq = ptr;

#ifdef LINUX_NL80211
  buff[0] = lq->valueBandwidth;
  buff[1] = lq->valueRSSI;
  // buff[2] = lq->prediction;

#else
  buff[0] = (unsigned char)(0);
  buff[1] = (unsigned char)(0);
  // buff[2] = (unsigned char)(0);

#endif
  buff[2] = (unsigned char)lq->valueLq;
  buff[3] = (unsigned char)lq->valueNlq;

  return 4;
}

static void
lq_deserialize_tc_lq_pair_ffeth_nl80211(const uint8_t **curr, void *ptr)
{
  struct lq_ffeth *lq = ptr;

#ifdef LINUX_NL80211
  pkt_get_u8(curr, &lq->valueBandwidth);
  pkt_get_u8(curr, &lq->valueRSSI);
  // pkt_get_u8(curr, &lq->prediction);

#else
  pkt_ignore_u16(curr);
  // pkt_ignore_u8(curr);

  
#endif
  pkt_get_u8(curr, &lq->valueLq);
  pkt_get_u8(curr, &lq->valueNlq);
}

static void
lq_packet_loss_worker_ffeth_nl80211(struct link_entry *link,
                                    void __attribute__((unused)) * ptr, bool lost)
{
  struct lq_ffeth_hello *tlq = (struct lq_ffeth_hello *)link->linkquality;

  if (lost)
  {
    tlq->missed_hellos++;
  }
  return;
}

static void
lq_memorize_foreign_hello_ffeth_nl80211(void *ptrLocal, void *ptrForeign)
{
  struct lq_ffeth_hello *local = ptrLocal;
  struct lq_ffeth *foreign = ptrForeign;

  if (foreign)
  {
    local->lq.valueNlq = foreign->valueLq;
  }
  else
  {
    local->lq.valueNlq = 0;
  }
}

static void
lq_copy_link2neigh_ffeth_nl80211(void *t, void *s)
{
  struct lq_ffeth *target = t;
  struct lq_ffeth_hello *source = s;
  *target = source->smoothed_lq;
}

static void
lq_copy_link2tc_ffeth_nl80211(void *t, void *s)
{
  struct lq_ffeth *target = t;
  struct lq_ffeth_hello *source = s;
  *target = source->smoothed_lq;
}

static void
lq_clear_ffeth_nl80211(void *target)
{
  memset(target, 0, sizeof(struct lq_ffeth));
}

static void
lq_clear_ffeth_nl80211_hello(void *target)
{
  struct lq_ffeth_hello *local = target;
  int i;

  lq_clear_ffeth_nl80211(&local->lq);
  lq_clear_ffeth_nl80211(&local->smoothed_lq);
  local->windowSize = LQ_FFETH_QUICKSTART_INIT;
  // added for trend
  local->twindowSize = LQ_FFETH_QUICKSTART_INIT;
  local->rwindowSize = LQ_FFETH_QUICKSTART_INIT;

  for (i = 0; i < nl80211_lq_window; i++)
  {
    local->total[i] = 3;
  }
}

static const char *
lq_print_ffeth_nl80211(void *ptr, char separator, struct lqtextbuffer *buffer)
{
  struct lq_ffeth *lq = ptr;
  int lq_int, nlq_int;

  lq_int = (int)lq->valueLq;
  if (lq_int > 0 && lq_int < 255)
  {
    lq_int++;
  }

  nlq_int = (int)lq->valueNlq;
  if (nlq_int > 0 && nlq_int < 255)
  {
    nlq_int++;
  }

  snprintf(buffer->buf, sizeof(buffer->buf), "%s%c%s", fpmtoa(fpmidiv(itofpm(lq_int), 255)), separator,
           fpmtoa(fpmidiv(itofpm(nlq_int), 255)));
  return buffer->buf;
}

static double
lq_print_cost_ffeth_nl80211(olsr_linkcost cost)
{
  // snprintf(buffer->buf, sizeof(buffer->buf), "%s", fpmtoa(cost));
  return fpmtod(cost);
}

#endif /* LINUX_NL80211 */
#endif /* __linux__ */

/*
 * Local Variables:
 * c-basic-offset: 2
 * indent-tabs-mode: nil
 * End:
 */
