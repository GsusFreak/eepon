
#include <stdlib.h>
#include <math.h>
#include <csim.h>
#include "eponsim.h"
#include "eponsim_util.h"
#include "eponsim_traffic.h"
#include <string.h>
#include <stdio.h>

extern EVENT SIM_END_EVENT;

/* Data structures for empirical distribution of packet sizes */
double  EMPIRICAL_PROB[5]  = { 0.60, 0.04, 0.11, 0.25 };
double  EMPIRICAL_VALUE[5] = { 64.0, 300.0, 580.0, 1518.0 };
double  EMPIRICAL_CUTOFF[5];
long  EMPIRICAL_ALIAS[5];
char  type[1] = "I";

/* Pareto distribution random number generator */
double stream_pareto_epon(STREAM randomStream, double shapeParam, double locationParam)
{
  return (locationParam/(pow(stream_uniform(randomStream,0,1),(1/shapeParam))));
}

/* Packet assignment function */
void assign_packet(sENTITY_PKT *pkt)
{
  if(pkt != NULL)
  {
    pkt->creationTime = simtime();
    if(oltAttrs.packetsHead == NULL)
    {
      /* this is the only packet */
      oltAttrs.packetsHead = pkt;
      oltAttrs.packetsTail = pkt;
    }
    else
    {
      /* Go to the end of the packet list for this ONU */
      pkt->prev = oltAttrs.packetsTail;
      oltAttrs.packetsTail->next = pkt;
      pkt->next = NULL;
      oltAttrs.packetsTail = pkt;
    }
    /* Add this packet's size to the queue size */
    oltAttrs.packetQueueSize += pkt->size;
    /* Add this packet to the queue packet count */
    oltAttrs.packetQueueNum++;

    // Tell the ONU that a packet has arrived
    //if(table_cnt(overallQueueDelay) < BOBB && pkt->onuNum == 0)
    //  TSprint("[%d] PACKET_ARRIVED -> 1\n", pkt->onuNum); 
    set(PACKET_ARRIVED[pkt->onuNum]);

    //if(table_cnt(overallQueueDelay) < BOBB && pkt->onuNum == 0)
    //  TSprint("[%d] ONU_HAS_NO_QUEUED_PACKETS -> 0\n", pkt->onuNum); 
    clear(ONU_HAS_NO_QUEUED_PACKETS[pkt->onuNum]);
  }
}

/*
 * Traffic models
 */

/* Process model of a Poisson traffic generator */
void traffic_src_poisson(int onuNum)
{
  int pktSize;
  sENTITY_PKT *pktPtr;
  
  /* Initialize the process */
  procId[0] = '\0';
  sprintf(procId,"TrafficGen #%d",onuNum);
  create(procId);

  while(!terminateSim)
  {
    /* Generate packets according to a particular distribution */
    hold(stream_exponential(oltAttrs.pktInterArrivalStream, simParams.AVG_PKT_INTER_ARVL_TIME));
    pktSize = (int)stream_empirical(oltAttrs.pktSizeStream, EMPIRICAL_SIZE, EMPIRICAL_CUTOFF, EMPIRICAL_ALIAS, EMPIRICAL_VALUE);
    pktPtr = create_a_packet(pktSize, onuNum);
    assign_packet(pktPtr);
    
    // Tell the OLT that a packet has arrived
    //if(table_cnt(overallQueueDelay) < BOBB)
    //  TSprint("SERVICE_OLT -> 1\n"); 
    set(SERVICE_OLT);
    
    // printf("[%10.5e] ---> Generating Packet with %d bytes for ONU #%d\n",simtime(),pktSize,onuNum);
  }
  TSprint("traffic_src_poisson #%d has ended\n", onuNum);
}

/* 
 * FUNCTION: traffic_src_self_similar()
 * DESCRIPTION: Process model of a Self-similar traffic generator
 *
 * Individual self-similar packet stream generator
 *
 * Pareto distributed burst size and Pareto distributed OFF time
 *
 */
void traffic_src_self_similar(int onuNum, int streamNum)
{
  int pktSize;
  sENTITY_PKT *pktPtr;
  double currPktTime, offTime;
  int burstSize, loop;
  
  /* Initialize the process */
  procId[0] = '\0';
  sprintf(procId,"TrafficGen %d:%d",onuNum, streamNum);
  create(procId); 
  // printf("Traffic #%d for ONU #%d\n",streamNum, onuNum);

  /* Sleep until first ON period */
  if(onuNum < simParams.NUM_HEAVY_ONU)
  {
    /* Obtain a Pareto distributed random number (using Uniform distribution transformation) */
    offTime = stream_pareto_epon(oltAttrs.pktInterArrivalStream,simParams.SS_PARETO_SHAPE_PARAM,simParams.SS_OFF_LOC_PARAM_HEAVY);
  }
  else
  {
    /* Obtain a Pareto distributed random number (using Uniform distribution transformation) */
    offTime = stream_pareto_epon(oltAttrs.pktInterArrivalStream,simParams.SS_PARETO_SHAPE_PARAM,simParams.SS_OFF_LOC_PARAM);
  }
  hold(offTime);

  while(!terminateSim)
  {
    /* Pareto distributed burst size */
    burstSize = (int) rintf(stream_pareto_epon(oltAttrs.pktInterArrivalStream,simParams.SS_PARETO_SHAPE_PARAM,simParams.SS_PARETO_LOC_PARAM));
    /* Transmit Burst */
    for(loop=0; loop < burstSize; loop++)
    {
      pktSize = (int) stream_empirical(oltAttrs.pktSizeStream, EMPIRICAL_SIZE, EMPIRICAL_CUTOFF, EMPIRICAL_ALIAS, EMPIRICAL_VALUE);
      pktPtr = create_a_packet(pktSize, onuNum);
      /* assign_packet(pktPtr, onuNum); */
      /* Send packet to packet arrival mailbox */
      send(oltAttrs.pktMailbox, (long) pktPtr);
      currPktTime = pktSize * simParams.TIME_PER_BYTE + simParams.PREAMBLE_IPG_TIME;
      hold(currPktTime);
    }
    /* Sleep until next ON period */
    if(onuNum < simParams.NUM_HEAVY_ONU)
    {
      /* Obtain a Pareto distributed random number (using Uniform distribution transformation) */
      offTime = stream_pareto_epon(oltAttrs.pktInterArrivalStream,simParams.SS_PARETO_SHAPE_PARAM,simParams.SS_OFF_LOC_PARAM_HEAVY);
    }
    else
    {
      /* Obtain a Pareto distributed random number (using Uniform distribution transformation) */
      offTime = stream_pareto_epon(oltAttrs.pktInterArrivalStream,simParams.SS_PARETO_SHAPE_PARAM,simParams.SS_OFF_LOC_PARAM);
    }
    /*
     * Factor out Preamble and IPG time from OFF time period
     * This way the load is computed as Ethernet frames only
     */
    if(offTime >= (simParams.PREAMBLE_IPG_TIME*burstSize))
    {
      offTime -= (simParams.PREAMBLE_IPG_TIME*burstSize);
    }
    else
    {
      offTime = 0;
    }
    hold(offTime);
  }
}

/* 
 * FUNCTION: traffic_agg_self_similar()
 * DESCRIPTION: Process model of a Self-similar traffic aggregator
 *
 * Aggregates the packets generated by the individual traffic streams
 *
 */
void traffic_agg_self_similar(int onuNum)
{
  sENTITY_PKT *pktPtr;
  double currPktTime;

  /* Initialize the process */
  procId[0] = '\0';
  sprintf(procId,"TrafficAgg %d",onuNum);
  create(procId); 
  // printf("Traffic Agg for ONU #%d\n",onuNum);

  while(!terminateSim)
  {
    /* Get packet from mailbox */
    receive(oltAttrs.pktMailbox, (long *) &pktPtr);

    /* Place in Queue for ONU */
    assign_packet(pktPtr);

    // Tell the OLT that a packet has arrived
    set(SERVICE_OLT);
    
    /* Check for excessive buffer size */
    if(oltAttrs.packetQueueSize > MAX_PKT_BUF_SIZE)
    {
      fatalErrorCode = FATAL_CAUSE_BUFFER_OVR;
      dump_sim_core();
    }

    // printf("[%10.5e] ---> Generating Packet with %d bytes for ONU #%d\n",simtime(),pktPtr->size,onuNum);

    /* expire time to transmit packet */
    currPktTime = pktPtr->size * simParams.TIME_PER_BYTE + simParams.PREAMBLE_IPG_TIME;
    hold(currPktTime);
  }
}


/*
   get_line() takes and array and maximum length, reads in one line of input 
   - but not to exceed the maximum length - places the input in the array 
   and returns the length of the input.
*/
   
int get_line(FILE *input, char *line, int maxlen)
{
  char c;              /* c is the next character, i the loop counter */
  int i;               /* read in until we have EOF or end-of-line    */
  int exit_iaa = 1;
  
  for (i = 0; (i < maxlen-1) && ((c = fgetc(input)) != EOF) && (c != '\n'); i++)
  {
    if (c == '#') {
      while (exit_iaa) {
        c = fgetc(input);
        if (c == '\n') {
          c = fgetc(input);
          if (c != '#') exit_iaa = 0;
        }
      }
    }
    if (c != '\t')  line[i] = c;
    else if (c == '\t' && i != 0)
    {
      line[i] = '\0';          /* terminate the string */
      return 500;
    }
    else 
    {
      line[i] = '\0'; 
      return 250;
    }
  }
  if ((c == '\n') && (line[0] != '#')) 
  {
    line[i] = '\0';          /* terminate the string */
    return 1000;
  }
  
  if (c == EOF)
  {
    line[i] = '\0';          /* terminate the string */
    return 2000;
  }
        
  return i;                /* return the length */
}

int get_line2(FILE *input, char *line, int maxlen)
{
  char c;              /* c is the next character, i the loop counter */
  int i;               /* read in until we have EOF or end-of-line    */
  int exit_iaa = 1;

  for (i = 0; (i < maxlen-1) && ((c = fgetc(input)) != EOF) && (c != '\n'); i++)
  {
    if (c == '#') {
      while (exit_iaa) {
        c = fgetc(input);
        if (c == '\n') {
          c = fgetc(input);
          if (c != '#') exit_iaa = 0;
        }
      }
    }
    if (c != '\t')  line[i] = c;
    else if (c == '\t' && i != 0)
    {
      line[i] = '\0';          /* terminate the string */
      return 500;
    }
    else 
    {
      line[i] = '\0'; 
      return 250;
    }
  }
  if ((c == '\n') && (line[0] != '#')) 
  {
    line[i] = '\0';          /* terminate the string */
    return 1000;
  }
  
  if ((c == '\n') && (line[0] == '#')) 
  {
    line[i] = '\0';          /* terminate the string */
    return -1;
  } 
  
  if (c == EOF)
  {
    line[i] = '\0';          /* terminate the string */
    return 2000;
  }

  return i;                /* return the length */
}




