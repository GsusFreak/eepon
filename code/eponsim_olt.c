

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
  int currPktONUNum = 0;
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
      TSprint("ERROR: FATAL_CAUSE_BUFFER_OVR\n");
      dump_sim_core();
      transmitPkt = 0;
    }
    
    transmitPkt = 1;
    // Start with the first packet in the OLT queue
    currPkt = oltAttrs.packetsHead;
    // Iterate through the OLT queue until the next packet is NULL
    while(transmitPkt)
    {
      //if(simParams.simType == PILOT_RUN)
      //  TSprint("pilot run OLT cycle start\n");
      if(currPkt == NULL)
      {
        transmitPkt = 0;
      }
      else
      {
        // If the state of the destination ONU is active, send the packet
        if(onuAttrs[currPkt->onuNum].state == ONU_ST_ACTIVE)
        {
          //if(simParams.simType == PILOT_RUN)
          //  TSprint("sim_time = %.9f\n", simtime());
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

          
          // Re-enable tracking of the queue for this ONU since the packeting preventing
          // this from increasing was just serviced
          onuAttrs[currPkt->onuNum].disableQueueTracking = 0;

          // Take this packet out of the queue size of all ONUs
          for (int iaa = 0; iaa < simParams.NUM_ONU; iaa ++)
          {
            onuAttrs[iaa].queuesize -= currPkt->size;
          }
          currPktONUNum = currPkt->onuNum;


          // Create a pointer to the packet so that it can be destroyed
          tmpPkt = currPkt;

          // Iterate to next packet 
          currPkt = currPkt->next;
          
          // Remove packet 
          remove_packet(tmpPkt); 
          

          // Reset the ONU queue size for this ONU since none
          // of the packets that arrived after the packet that 
          // was just serviced have been counted
          onuAttrs[currPktONUNum].queuesize = get_queue_size_until_certain_ONU(currPktONUNum);

          // Calculate the potential sleep time for this ONU
          double timeUntilONUsNextPacket = onuAttrs[currPktONUNum].queuesize * simParams.TIME_PER_BYTE;
          double timeForWholeQueue = oltAttrs.packetQueueSize * simParams.TIME_PER_BYTE;
          onuAttrs[currPktONUNum].heavy_traffic_sleep_duration = timeUntilONUsNextPacket - simParams.ONU_TIME_WAKEUP;
          // If sleep time is > 0, sleep. (The sleep time can be less than zero since
          // the WAKEUP time is subtracted from the queue time)

          if(onuAttrs[currPktONUNum].heavy_traffic_sleep_duration >= 0)
          {
            // Only use the PDHprint command with short simulations
            // Otherwise, huge files can be generated
            if(simParams.simType == ACTUAL_RUN)
            //if(timeUntilONUsNextPacket > simParams.ONU_TIME_WAKEUP && simParams.simType == ACTUAL_RUN && table_cnt(overallQueueDelay) <= 10000)
              PDHprint("ONU %d, Load %0.1f, Sleep Duration %e, Queue Depth %e\n", currPktONUNum, simParams.DESIRED_LOAD, timeUntilONUsNextPacket, timeForWholeQueue); 
          
            // Only use the PDHprint command with short simulations
            // Otherwise, huge files can be generated
            //if(simParams.simType == ACTUAL_RUN)
            //  PDHprint("%0.1f, %e\n", simParams.DESIRED_LOAD, onuAttrs[pkt->onuNum].heavy_traffic_sleep_duration);
            set(HEAVY_TRAFFIC_SLEEP_TRIGGERED[currPktONUNum]);
          }
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




