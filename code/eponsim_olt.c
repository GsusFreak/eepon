
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

  if(oltAttrs.packetsHead != NULL)
  {
    tmp = oltAttrs.packetsHead;
    arrivalSum = 0;
    while(tmp != NULL)
    {
      arrivalSum += tmp->creationTime;
      tmp = tmp->next;
    }
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
    txPktCount = 0;
    transmitPkt = 1;
    double frameTimeRemaining = 0;
    int END_OF_FRAME = 0;
      
    // Service the OLT packet queue
       
    /* Check for excessive buffer size */
    if(oltAttrs.packetQueueSize > MAX_PKT_BUF_SIZE)
    {
      fatalErrorCode = FATAL_CAUSE_BUFFER_OVR;
      dump_sim_core();
      transmitPkt = 0;
    }
    
    // Reset the EndOfFrame flag
    END_OF_FRAME = 0;
    frameTimeRemaining = simParams.OLT_FRAME_TIME;
    while(transmitPkt && END_OF_FRAME == 0)
    {
      if(oltAttrs.packetsHead == NULL)
      {
        // If there are no packets, simply wait until the next OLT frame 
        transmitPkt = 0;
        hold(frameTimeRemaining);

        //wait(SERVICE_OLT);
      }
      else
      {
        /* Copy packet to temporary data structure */
        currPkt.creationTime = oltAttrs.packetsHead->creationTime;
        currPkt.transmissionTime = oltAttrs.packetsHead->transmissionTime;
        currPkt.arrivalTime = oltAttrs.packetsHead->arrivalTime;
        currPkt.size = oltAttrs.packetsHead->size;
        
        packet_transmission_time = (currPkt.size)*simParams.TIME_PER_BYTE;
        
        if(frameTimeRemaining >= packet_transmission_time)
        {
          /* collect statistics on this packet */
          record_stats_queue_length(currPkt.onuNum);
          record_packet_stats_dequeue(currPkt.onuNum);
          
          /* remove packet */
          remove_packet();    
          
          hold(packet_transmission_time);
    
          /* Increment transmitted packet counter */
          txPktCount++;
    
          /* Incremement throughput statistics per ONU*/
          oltAttrs.transmitByteCnt += currPkt.size;
          
          /* Collect statistics on this packet */
          record_packet_stats_finish(&currPkt);

          ///* Make sure that any SERVICE_OLT events that happened while
          // * serviceing the queue don't immediately trigger the OLT
          // * again.
          // */
          //clear(SERVICE_OLT);

          // Subtract the transmission time from the remaining 
          // frame time.
          frameTimeRemaining -= packet_transmission_time;
        }
        else
        {
          // If there's no time left to transmit packets in this frame, 
          // end the frame.
          END_OF_FRAME = 1;
          transmitPkt = 0;
        }
      }
    }
  }
}




