
#include "eponsim.h"

/* Simulation End Event */
EVENT SIM_END_EVENT;

// Debugging the Cycle length and the queueing delay mismatch

int minSwitch = 1;
double minQueueDelay;


//record_packet_stats_finish(currPkt.onuNum, &currPkt);

/* Record stats after packet arrival */
void record_packet_stats_finish(sENTITY_PKT *pkt)
{
  pkt->arrivalTime = simtime();
  /* Record packet transmission time in proper table */
  record(pkt->arrivalTime - pkt->creationTime, oltAttrs.transmitTimeTable);
}


/* Record stats after packet is dequeued */
void record_packet_stats_dequeue(int onuNum)
{
  oltAttrs.packetsHead->transmissionTime = simtime();
  /* Record packet queue time in proper table */
  record(oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime, oltAttrs.queueTimeTable);
  /* Record packet queue time in overall queueing delay table */
  record(oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime, overallQueueDelay);
  if(onuNum < simParams.NUM_HEAVY_ONU)
  {
    /* Record packet queue time in Heavy ONU queueing delay table */
    record(oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime, heavyQueueDelay);
  }
  else
  {
    /* Record packet queue time in Heavy ONU queueing delay table */
    record(oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime, lightQueueDelay);
  }
  if((simParams.TRAFFIC_TYPE == TRAFFIC_SELF_SIMILAR) && (simInitPhase == 0))
  {
    /* Record confidence interval data for Self-similar traffic */
    if(((oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime) < overallQueueDelayStat.intervalLower)
       || ((oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime) > overallQueueDelayStat.intervalUpper))
    {
      overallQueueDelayStat.outsideIntervalCount++;
      if(onuNum < simParams.NUM_HEAVY_ONU)
      {
        heavyQueueDelayStat.outsideIntervalCount++;
      }
      else
      {
        lightQueueDelayStat.outsideIntervalCount++;
      }
    }
    else
    {
      overallQueueDelayStat.insideIntervalCount++;
      if(onuNum < simParams.NUM_HEAVY_ONU)
      {
        heavyQueueDelayStat.insideIntervalCount++;
      }
      else
      {
        lightQueueDelayStat.insideIntervalCount++;
      }
    }
  }
}


/* Record stats for queue length */
void record_stats_queue_length(int onuNum)
{
  /* Record queue length in proper table */
  record(oltAttrs.packetQueueSize, oltAttrs.queueLengthTable);
  record(oltAttrs.packetQueueSize, overallQueueLength);
  if(onuNum < simParams.NUM_HEAVY_ONU)
  {
    record(oltAttrs.packetQueueSize, heavyQueueLength);
  }
  else
  {
    record(oltAttrs.packetQueueSize, lightQueueLength);
  }
}


/* Record stats after packet is dequeued */
void record_packet_stats_dequeue_tx_time(int onuNum)
{
  oltAttrs.packetsHead->transmissionTime = simtime() + oltAttrs.packetsHead->size*simParams.TIME_PER_BYTE; 
  /* Record packet queue time in proper table */
  record(oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime, oltAttrs.queueTimeTable);
  /* Record packet queue time in delay table for cycle based observation */
  record(oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime, cycleQueueDelay);
  /* Record packet queue time in overall queueing delay table */
  record(oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime, overallQueueDelay);
  if(onuNum < simParams.NUM_HEAVY_ONU)
  {
    /* Record packet queue time in Heavy ONU queueing delay table */
    record(oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime, heavyQueueDelay);
  }
  else
  {
    /* Record packet queue time in Light ONU queueing delay table */
    record(oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime, lightQueueDelay);
  }
  if((simParams.TRAFFIC_TYPE == TRAFFIC_SELF_SIMILAR) && (simInitPhase == 0))
  {
    /* Record confidence interval data for Self-similar traffic */
    if(((oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime) < overallQueueDelayStat.intervalLower)
       || ((oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime) > overallQueueDelayStat.intervalUpper))
    {
      overallQueueDelayStat.outsideIntervalCount++;
      if(onuNum < simParams.NUM_HEAVY_ONU)
      {
        heavyQueueDelayStat.outsideIntervalCount++;
      }
      else
      {
        lightQueueDelayStat.outsideIntervalCount++;
      }
    }
    else
    {
      overallQueueDelayStat.insideIntervalCount++;
      if(onuNum < simParams.NUM_HEAVY_ONU)
      {
        heavyQueueDelayStat.insideIntervalCount++;
      }
      else
      {
        lightQueueDelayStat.insideIntervalCount++;
      }
    }
  }
}


/* Record stats after packet is dequeued */
void record_packet_stats_dequeue_minus_prop(int onuNum)
{
  oltAttrs.packetsHead->transmissionTime = simtime() - onuAttrs[onuNum].latency;
  /* Record packet queue time in proper table */
  record(oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime, oltAttrs.queueTimeTable);
  /* Record packet queue time in overall queueing delay table */
  record(oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime, overallQueueDelay);
  if(onuNum < simParams.NUM_HEAVY_ONU)
  {
    /* Record packet queue time in Heavy ONU queueing delay table */
    record(oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime, heavyQueueDelay);
  }
  else
  {
    /* Record packet queue time in Heavy ONU queueing delay table */
    record(oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime, lightQueueDelay);
  }
  if((simParams.TRAFFIC_TYPE == TRAFFIC_SELF_SIMILAR) && (simInitPhase == 0))
  {
    /* Record confidence interval data for Self-similar traffic */
    if(((oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime) < overallQueueDelayStat.intervalLower) || ((oltAttrs.packetsHead->transmissionTime - oltAttrs.packetsHead->creationTime) > overallQueueDelayStat.intervalUpper))
    {
      overallQueueDelayStat.outsideIntervalCount++;
      if(onuNum < simParams.NUM_HEAVY_ONU)
      {
        heavyQueueDelayStat.outsideIntervalCount++;
      }
      else
      {
        lightQueueDelayStat.outsideIntervalCount++;
      }
    }
    else
    {
      overallQueueDelayStat.insideIntervalCount++;
      if(onuNum < simParams.NUM_HEAVY_ONU)
      {
        heavyQueueDelayStat.insideIntervalCount++;
      }
      else
      {
        lightQueueDelayStat.insideIntervalCount++;
      }
    }
  }
}


/*! 
   Calculate the Raj Jain Fairness index
   based on actual Allocation set, A 
   and optimal allocation set, O
   with the following Fairness calculation:
 
  Xi = Ai/Oi
 
   Fairness = [  sigma(Xi)^2  ] / [ n*sigma(Xi^2) ]
              
 \author John McAlarney
 \param a Actual Allocation set
 \param o Optimal Allocation set
 \param n Number of Samples
 \return Raj Jain Fairness Index
*/
double rj_fairness_index(double * a, double * o, int n)
{
  //Iterator for array indexing
  int i = 0;
  //accumulating numerator for fairness calculation
  double num = 0;
  //accumulating denominator for fairness calculation
  double denom = 0;
  //relative allocation X[i]=A[i]/O[i]
  double x[n];
  
  for(i=0;i<n;i++)
  {
    //find current relative allocation for set
    x[i] = a[i]/o[i];
    //accumulate numerator and denominator
    num += x[i];
    denom += x[i]*x[i];
  }
  
  return ((num*num)/(n*denom));
}

void reset_throughput_counters()
{
  int i;
  
  /* Clear Throughput Counters */
  for(i=0; i < simParams.NUM_ONU; i++)
  {
    onuAttrs[i].transmitByteCnt = 0;
  }
  oltAttrs.transmitByteCnt = 0;
  reset_throughput_flag = 1;
}

/* 
 * FUNCTION: onu_throughput_calc()
 * DESCRIPTION: polls ONUs for throughput statistics
 *
 */
void onu_throughput_calc()
{
  int loopIndex;
  float const poll_rate = 0.1; //times per second
  create("ONU Throughput Calculator");

  while(!terminateSim)
  {
    hold(1/poll_rate);
    if (reset_throughput_flag == 0)
    {
      for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
      {
        /* record throughput over the last polling period */
        actual_tput[loopIndex] = onuAttrs[loopIndex].transmitByteCnt*8*poll_rate;
        /* Mark in throughput table */
        record(actual_tput[loopIndex], onuAttrs[loopIndex].transmitThroughput);
      }
      actual_tput_olt = oltAttrs.transmitByteCnt*8*poll_rate;
      record(actual_tput_olt, oltAttrs.transmitThroughput);
      /* Compute Raj Jain's fairness index and record in a table */
      record(rj_fairness_index(actual_tput,ideal_tput,simParams.NUM_ONU), throughputFairness);
    }
    else
    {
      reset_throughput_flag = 0;
    }
    for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
      onuAttrs[loopIndex].transmitByteCnt = 0;
    }
    oltAttrs.transmitByteCnt = 0;
  } 
}




