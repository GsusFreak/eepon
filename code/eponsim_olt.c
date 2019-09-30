

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
  pktPtr = oltAttrs.packetsHead;
  while (pktPtr != NULL) {
    pktPtr = pktPtr->next;
    length++;
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

  if(oltAttrs.packetsHead != NULL)
  {
    tmp = oltAttrs.packetsHead;
    while(tmp != NULL)
    {
      arrivalSum += tmp->creationTime;
      tmp = tmp->next;
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
  sENTITY_PKT *currPkt, *tmpPkt;
  //int iaa;  // This is the number of the onu being serviced by the OLT

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
    txPktCount = 0;
   
    /* Check for excessive buffer size */
    if(oltAttrs.packetQueueSize > MAX_PKT_BUF_SIZE)
    {
      fatalErrorCode = FATAL_CAUSE_BUFFER_OVR;
      dump_sim_core();
      transmitPkt = 0;
    }
    
    transmitPkt = 1;
    currPkt = oltAttrs.packetsHead;
    while(transmitPkt)
    {
      if(currPkt == NULL)
      {
        transmitPkt = 0;
      }
      else
      {
        if(onuAttrs[currPkt->onuNum].state == ONU_ST_ACTIVE)
        {
          /* Copy packet to temporary data structure */
          packet_transmission_time = (currPkt->size)*simParams.TIME_PER_BYTE;
          
          /* collect statistics on this packet */
          record_stats_queue_length(currPkt);
          record_packet_stats_dequeue(currPkt);
          
          /* Incremement throughput statistics per ONU */
          oltAttrs.transmitByteCnt += currPkt->size;
          onuAttrs[currPkt->onuNum].transmitByteCnt += currPkt->size;
          
          /* Have the OLT process wait while it sends the packet */
          hold(packet_transmission_time);

          /* Increment transmitted packet counter */
          txPktCount++;
    
          /* Collect statistics on this packet */
          // This function probably needs tweaking
          record_packet_stats_finish(currPkt);

          /* Iterate to next packet */
          tmpPkt = currPkt;
          currPkt = currPkt->next;
    
          /* remove packet */
          remove_packet(tmpPkt); 
        }
        else
        {
          currPkt = currPkt->next; 
        }
      }
    }
    //if(table_cnt(overallQueueDelay) < BOBB && iaa == 0)
    //  TSprint("[%d] ONU_HAS_NO_QUEUED_PACKETS -> 1\n", iaa); 
    //set(ONU_HAS_NO_QUEUED_PACKETS[iaa]);
    //if(table_cnt(overallQueueDelay) < BOBB && iaa == 0)
    //  TSprint("[%d] PACKET_ARRIVED -> 0\n", iaa); 
    //clear(PACKET_ARRIVED[iaa]);
    //}

    /* Make sure that any SERVICE_OLT events that happened while
     * servicing the queue don't immediately trigger the OLT
     * again. */
    //if(table_cnt(overallQueueDelay) < BOBB)
    //  TSprint("SERVICE_OLT -> 0\n"); 
    clear(SERVICE_OLT);

    /* Wait for the next packet to come in */
    wait(SERVICE_OLT);
  }
}




