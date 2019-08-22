
#include <csim.h>
#include "eponsim.h"
#include "eponsim_util.h"
#include "eponsim_stats.h"

/* Simulation End Event */
EVENT SIM_END_EVENT;

FILE *maxFile;

void check_data_packet_list() {
  sENTITY_PKT *pktPtr;
  int length = 0;
  for(int iaa = 0; iaa < simParams.NUM_ONU; iaa++) {
    pktPtr = oltAttrs.packetsHead[iaa];
    while (pktPtr != NULL) {
      pktPtr = pktPtr->next;
      length++;
    }
  }
  if (length > DATA_QUEUE_MAX_LENGTH) {
    fatalErrorCode = FATAL_CAUSE_LENGTH_DATA_BUFFER_OVR;
    dump_sim_core();
  }
  return;
}


/* Calculate packet arrival time statistics */
void calc_avg_arrival(double *avgVal)
{
  sENTITY_PKT *tmp;
  double  arrivalSum;
  arrivalSum = 0;

  for(int iaa = 0; iaa < simParams.NUM_ONU; iaa++)
  {
    if(oltAttrs.packetsHead[iaa] != NULL)
    {
      tmp = oltAttrs.packetsHead[iaa];
      while(tmp != NULL)
      {
        arrivalSum += tmp->creationTime;
        tmp = tmp->next;
      }
    }
  }
  if(arrivalSum != 0)
  {
    *avgVal = arrivalSum/(double)oltAttrs.packetQueueNum;
  }
  else
  {
    *avgVal = 0;
  }
}


void olt()
{
  int transmitPkt;
  long txPktCount = 0;
  double packet_transmission_time;
  sENTITY_PKT currPkt;

  /* Initialize the process */
  procId[0] = '\0';
  sprintf(procId,"OLT");
  create(procId);
  
  // Test Variables
  status_processes_print();
  
  /* Upstream transmission gate functinnality of new OLT */
  while(!terminateSim) /* permanent behavior of the new OLT process moel */
  {
    /* Reset the loop variables */
    txPktCount  = 0;
      
    // Service the OLT packet queue
       
    /* Check for excessive buffer size */
    if(oltAttrs.packetQueueSize > MAX_PKT_BUF_SIZE)
    {
      fatalErrorCode = FATAL_CAUSE_BUFFER_OVR;
      dump_sim_core();
      transmitPkt = 0;
    }
    
    for(int iaa = 0; iaa < simParams.NUM_ONU; iaa++)
    {
      if(onuAttrs[iaa].state == ONU_ST_ACTIVE)
      {
        transmitPkt = 1;
        while(transmitPkt)
        {
          if(oltAttrs.packetsHead[iaa] == NULL)
          {
            transmitPkt = 0;
          }
          else
          {
            /* Copy packet to temporary data structure */
            currPkt.creationTime = oltAttrs.packetsHead[iaa]->creationTime;
            currPkt.transmissionTime = oltAttrs.packetsHead[iaa]->transmissionTime;
            currPkt.arrivalTime = oltAttrs.packetsHead[iaa]->arrivalTime;
            currPkt.size = oltAttrs.packetsHead[iaa]->size;
            currPkt.onuNum = oltAttrs.packetsHead[iaa]->onuNum;
            
            packet_transmission_time = (currPkt.size)*simParams.TIME_PER_BYTE;
            
            /* collect statistics on this packet */
            record_stats_queue_length(currPkt.onuNum);
            record_packet_stats_dequeue(currPkt.onuNum);
            
            /* Have the OLT process wait while it sends the packet */
            hold(packet_transmission_time);
        
            /* remove packet */
            remove_packet(currPkt.onuNum);    
            
            /* Increment transmitted packet counter */
            txPktCount++;
        
            /* Incremement throughput statistics per ONU */
            oltAttrs.transmitByteCnt += currPkt.size;
            
            /* Collect statistics on this packet */
            record_packet_stats_finish(&currPkt);
          }
        }
        if(table_cnt(overallQueueDelay) < BOBB && iaa == 0)
          TSprint("[%d] ONU_HAS_NO_QUEUED_PACKETS -> 1\n", iaa); 
        set(ONU_HAS_NO_QUEUED_PACKETS[iaa]);
        if(table_cnt(overallQueueDelay) < BOBB && iaa == 0)
          TSprint("[%d] PACKET_ARRIVED -> 0\n", iaa); 
        clear(PACKET_ARRIVED[iaa]);
      }
    }
    /* Make sure that any SERVICE_OLT events that happened while
     * serviceing the queue don't immediately trigger the OLT
     * again. */
    //if(table_cnt(overallQueueDelay) < BOBB)
    //  TSprint("SERVICE_OLT -> 0\n"); 
    clear(SERVICE_OLT);

    /* Wait for the next packet to come in */
    wait(SERVICE_OLT);
  }
}




