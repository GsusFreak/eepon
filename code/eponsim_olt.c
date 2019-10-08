

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
  double packet_transmission_time;
  sENTITY_PKT *currPkt, *tmpPkt;
  //int iaa;  // This is the number of the onu being serviced by the OLT

  /* Initialize the process */
  procId[0] = '\0';
  sprintf(procId,"OLT");
  create(procId);
  
  // Test Variables
  status_processes_print();
  
  /* Upstream transmission gate functionality of new OLT */
  while(!terminateSim) /* permanent behavior of the new OLT process moel */
  {
    /* Check for excessive buffer size */
    if(oltAttrs.packetQueueSize > MAX_PKT_BUF_SIZE)
    {
      fatalErrorCode = FATAL_CAUSE_BUFFER_OVR;
      dump_sim_core();
      transmitPkt = 0;
    }
    
    transmitPkt = 1;
    // Start with the first packet in the OLT queue
    currPkt = oltAttrs.packetsHead;
    // Iterate through the OLT queue until the next packet is NULL
    while(transmitPkt)
    {
      if(currPkt == NULL)
      {
        transmitPkt = 0;
      }
      else
      {
        // If the state of the destination ONU is active, send the packet
        if(onuAttrs[currPkt->onuNum].state == ONU_ST_ACTIVE)
        {
          // Calculate the necessary transmission time
          packet_transmission_time = (currPkt->size)*simParams.TIME_PER_BYTE;
          
          // Record the queue length to the various (3?) queue length tables
          record_stats_queue_length(currPkt);

          // Record transmission time & calculate delay
          record_packet_stats_dequeue(currPkt);
          
          // Update ONU and OLT throughput statistics
          oltAttrs.transmitByteCnt += currPkt->size;
          onuAttrs[currPkt->onuNum].transmitByteCnt += currPkt->size;
          
          // Have the OLT process wait while the packet is sent
          hold(packet_transmission_time);

          // Record the arrival time of the packet in the appropriate table
          record_packet_stats_finish(currPkt);

          // Create a pointer to the packet so that it can be destroyed
          tmpPkt = currPkt;

          // Iterate to next packet 
          currPkt = currPkt->next;
          
          // Remove packet 
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




