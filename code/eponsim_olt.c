

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
          onuAttrs[currPktONUNum].disableQueueTracking = is_ONU_in_queue(currPktONUNum);

          // Calculate the potential sleep time for this ONU
          onuAttrs[currPktONUNum].heavy_traffic_sleep_duration = ((double)onuAttrs[currPktONUNum].queuesize * simParams.TIME_PER_BYTE) - simParams.ONU_TIME_WAKEUP;
          
          // If sleep time is > 0, sleep. (The sleep time can be less than zero since
          // the WAKEUP time is subtracted from the queue time)
          if(onuAttrs[currPktONUNum].heavy_traffic_sleep_duration > 0)
          {
            onuAttrs[currPktONUNum].src_sleep = 0;
            set(HEAVY_TRAFFIC_SLEEP_TRIGGERED[currPktONUNum]);
          }



          // Check if any of the other ONUs can sleep
          for (int iaa = 0; iaa < simParams.NUM_ONU; iaa++) 
          {
            if(onuAttrs[iaa].state == ONU_ST_ACTIVE && onuAttrs[iaa].disableQueueTracking == 0)
            {
              // Calculate the potential sleep time for this ONU
              onuAttrs[iaa].heavy_traffic_sleep_duration = ((double)onuAttrs[iaa].queuesize * simParams.TIME_PER_BYTE) - simParams.ONU_TIME_WAKEUP;
              // If sleep time is > 0, sleep. (The sleep time can be less than zero since
              // the WAKEUP time is subtracted from the queue time)

              if(onuAttrs[iaa].heavy_traffic_sleep_duration > 0)
              {
                onuAttrs[iaa].src_sleep = 2;
                set(HEAVY_TRAFFIC_SLEEP_TRIGGERED[iaa]);
              }
            }
          }
        }
        else
        {
          if(simParams.simType == PILOT_RUN)
          {
            test_vars.onu_not_servicable_cnt[test_vars.runNum][test_vars.loadOrderCounter][currPkt->onuNum][0]++;
            if(test_vars.onu_not_servicable_cnt[test_vars.runNum][test_vars.loadOrderCounter][currPkt->onuNum][0] <= 1000)
              PDHprint("ONU #%d, src %d, Short Calc %.3e, Long Calc %.3e, Sleep Dur %.3e, Actual Time %.3e\n", currPkt->onuNum, onuAttrs[currPkt->onuNum].src_sleep, onuAttrs[currPkt->onuNum].last_sleep_time_short, onuAttrs[currPkt->onuNum].last_sleep_time_long, onuAttrs[currPkt->onuNum].heavy_traffic_sleep_duration, simtime()-onuAttrs[currPkt->onuNum].start_of_sleep);
          }
          if(simParams.simType == ACTUAL_RUN)
          {
            test_vars.onu_not_servicable_cnt[test_vars.runNum][test_vars.loadOrderCounter][currPkt->onuNum][1]++;
            if(test_vars.onu_not_servicable_cnt[test_vars.runNum][test_vars.loadOrderCounter][currPkt->onuNum][1] <= 1000)
              PDHprint("ONU #%d, src %d, Short Calc %.3e, Long Calc %.3e, Sleep Dur %.3e, Actual Time %.3e\n", currPkt->onuNum, onuAttrs[currPkt->onuNum].src_sleep, onuAttrs[currPkt->onuNum].last_sleep_time_short, onuAttrs[currPkt->onuNum].last_sleep_time_long, onuAttrs[currPkt->onuNum].heavy_traffic_sleep_duration, simtime()-onuAttrs[currPkt->onuNum].start_of_sleep);
          }
          // Once a certain number of packets miss their mark, turn off the sim to prevent creating massive files

          currPkt = currPkt->next; 
        }
      }
    }

    // Record the number of complete cycles the OLT goes through
    if(simParams.simType == PILOT_RUN)
      test_vars.olt_cycle_cnt[test_vars.runNum][test_vars.loadOrderCounter][0]++;
    if(simParams.simType == ACTUAL_RUN)
      test_vars.olt_cycle_cnt[test_vars.runNum][test_vars.loadOrderCounter][1]++;

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




