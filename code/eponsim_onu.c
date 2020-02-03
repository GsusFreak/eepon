
#include <values.h>
#include <stdlib.h>
#include <string.h>
#include <csim.h>
#include "hungarian.h"
#include "eponsim.h"
#include "eponsim_util.h"
#include "eponsim_onu.h"

void journal(int onuNum)
{
  long  outcome;
  switch(onuAttrs[onuNum].state)
  {
    case ONU_ST_ACTIVE:


      wait(ONU_HAS_NO_QUEUED_PACKETS[onuNum]);
      //if(table_cnt(overallQueueDelay) < BOBB && onuNum == 0)
      //  TSprint("[%d] ACTIVE -> IDLE\n", onuNum); 
      changeState(onuNum, ONU_ST_IDLE);
      break;
    case ONU_ST_IDLE:
      outcome = timed_wait(PACKET_ARRIVED[onuNum], simParams.ONU_TIME_TRIGGER);
      if(outcome == EVENT_OCCURRED)
      {
        //if(table_cnt(overallQueueDelay) < BOBB && onuNum == 0)
        //{
        //  TSprint("[%d] IDLE -> ACTIVE\n", onuNum); 
        //}
        changeState(onuNum, ONU_ST_ACTIVE);
      }
      else if(outcome == TIMED_OUT)
      {
        //if(table_cnt(overallQueueDelay) < BOBB && onuNum == 0)
        //{
        //  TSprint("[%d] IDLE -> SLEEP\n", onuNum);
        //}
        changeState(onuNum, ONU_ST_SLEEP);
      }
      else
      {
        //if(table_cnt(overallQueueDelay) < BOBB && onuNum == 0)
        //{
        //  TSprint("[%d] IDLE -> SLEEP\n", onuNum);
        //}
        changeState(onuNum, ONU_ST_SLEEP);
      }
      break;
    case ONU_ST_SLEEP:
      hold(simParams.ONU_TIME_SLEEP);
      //if(table_cnt(overallQueueDelay) < BOBB && onuNum == 0)
      //  TSprint("[%d] SLEEP -> PROBE\n", onuNum); 
      changeState(onuNum, ONU_ST_PROBE);
      break;
    case ONU_ST_PROBE:
      hold(simParams.ONU_TIME_PROBE);
      outcome = timed_wait(PACKET_ARRIVED[onuNum], 0.0); 
      if(outcome == EVENT_OCCURRED)
      {
        //if(table_cnt(overallQueueDelay) < BOBB && onuNum == 0)
        //{
        //  TSprint("[%d] PROBE -> WAKEUP\n", onuNum); 
        //}
        changeState(onuNum, ONU_ST_WAKEUP);
      }
      else if(outcome == TIMED_OUT)
      {
        //if(table_cnt(overallQueueDelay) < BOBB && onuNum == 0)
        //{
        //  TSprint("[%d] PROBE -> SLEEP\n", onuNum); 
        //}
        changeState(onuNum, ONU_ST_SLEEP);
      }
      else 
      {
        //if(table_cnt(overallQueueDelay) < BOBB && onuNum == 0)
        //{
        //  TSprint("[%d] PROBE -> SLEEP\n", onuNum); 
        //}
        changeState(onuNum, ONU_ST_SLEEP);
      }
      
      //// Calculate the number of grant cycles until this
      //// ONU will receive its cycle
      //NumGrantsToWait = (simParams.ONU_GRANTED - onuNum + simParams.NUM_ONU)%32;
      //
      //// Wait the first half of the probe time when it is 
      //// impossible for the probe to receive a report,
      //// and then wait the appropriate number of grant cycles.
      //hold(simParams.ONU_TIME_PROBE/2.0 + simParams.TIME_PER_GRANT*NumGrantsToWait);

      //// Check if any packets have arrived
      //outcome = timed_wait(PACKET_ARRIVED[onuNum], simParams.TIME_PER_GRANT);
      //
      //// If packets were found, stop the Probe state immediately and switch
      //// to the active state.
      //if(outcome == EVENT_OCCURRED)
      //  changeState(onuNum, ONU_ST_ACTIVE);
      //// If packets were not found, hold for the remainder of the probe
      //// period, and then switch back to the sleep state.
      //if(outcome == TIMED_OUT)
      //{
      //  hold(simParams.TIME_PER_GRANT*(32-NumGrantsToWait-1));
      //  changeState(onuNum, ONU_ST_SLEEP);
      //}
     
    case ONU_ST_WAKEUP:
      hold(simParams.ONU_TIME_WAKEUP);
      changeState(onuNum, ONU_ST_ACTIVE);
      break;

    case FINAL_eONU_STATE_ENTRY:
      break;
  } 
}

void heavy_traffic(int onuNum)
{
  switch(onuAttrs[onuNum].state)
  {
    case ONU_ST_ACTIVE:
      // Calculate the potential sleep time for this ONU
      onuAttrs[onuNum].heavy_traffic_sleep_duration = ((double)onuAttrs[onuNum].queuesize * simParams.TIME_PER_BYTE) - (simParams.ONU_TIME_WAKEUP * 1.01);

      // If sleep time is > 0, sleep. (The sleep time can be less than zero since
      // the WAKEUP time is subtracted from the queue time)
      if(onuAttrs[onuNum].heavy_traffic_sleep_duration > 0)
      {
        // Only use the PDHprint command with short simulations
        // Otherwise, huge files can be generated
        //if(simParams.simType == ACTUAL_RUN)
        //  PDHprint("%0.1f, %e\n", simParams.DESIRED_LOAD, onuAttrs[pkt->onuNum].heavy_traffic_sleep_duration);
        //set(HEAVY_TRAFFIC_SLEEP_TRIGGERED[onuNum]);
      }

      wait(HEAVY_TRAFFIC_SLEEP_TRIGGERED[onuNum]);
      
      // Record the projected sleep times used so that they can be debuged 
      // if they don't work at the OLT
      onuAttrs[onuNum].last_sleep_time_short = onuAttrs[onuNum].queuesize * simParams.TIME_PER_BYTE;
      onuAttrs[onuNum].last_sleep_time_long = get_queue_size_until_certain_ONU(onuNum) * simParams.TIME_PER_BYTE;
      onuAttrs[onuNum].start_of_sleep = simtime();

      changeState(onuNum, ONU_ST_SLEEP);
      break;
    case ONU_ST_SLEEP:
      hold(onuAttrs[onuNum].heavy_traffic_sleep_duration);
      changeState(onuNum, ONU_ST_WAKEUP);
      break;
    case ONU_ST_WAKEUP:
      hold(simParams.ONU_TIME_WAKEUP);

      // Clear the "sleep due to heavy traffic flag" 
      // so that it won't matter if it was set during
      // the sleeping period
      clear(HEAVY_TRAFFIC_SLEEP_TRIGGERED[onuNum]);
      changeState(onuNum, ONU_ST_ACTIVE);
      break;
    case ONU_ST_IDLE:
      // In this alrogithm, the IDLE state should never be used
      changeState(onuNum, ONU_ST_ACTIVE);
      break;
    case ONU_ST_PROBE:
      // In this alrogithm, the PROBE state should never be used
      changeState(onuNum, ONU_ST_ACTIVE);
      break;
    case FINAL_eONU_STATE_ENTRY:
      break;
  } 
}


void onu(int onuNum)
{
  create("ONU");
  printf("OLT %d started\n", onuNum);

  // Test Variables
  status_processes_print();
  
  //int   NumGrantsToWait = 0;

  /* Permanent OLT behavior */
  while(!terminateSim)
  {
    //NumGrantsToWait = 0;
    switch(simParams.sleepScheduler)
    {
      case SLEEP_SCHEDULER_JOURNAL:
        journal(onuNum);
        break;
      case SLEEP_SCHEDULER_HEAVY_TRAFFIC:
        heavy_traffic(onuNum);
        break;
      case SLEEP_SCHEDULER_HEAVY_TRAFFIC_HYBRID:
        journal(onuNum);
        break;
    }
  } 
}




