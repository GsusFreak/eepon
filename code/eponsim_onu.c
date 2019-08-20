
#include <values.h>
#include <stdlib.h>
#include <string.h>
#include <csim.h>
#include "hungarian.h"
#include "eponsim.h"
#include "eponsim_util.h"
#include "eponsim_onu.h"

void onu(int onuNum)
{
  create("ONU");
  printf("OLT %d started\n", onuNum);

  // Test Variables
  status_processes_print();
  
  long  outcome;
  int   NumGrantsToWait = 0;

  //typedef enum {ONU_ST_ACTIVE, ONU_ST_IDLE, ONU_ST_SLEEP, ONU_ST_PROBE, FINAL_eONU_STATE_ENTRY} eONU_STATE;
  //simParams.ONU_TIME_SLEEP   = 0.005;
  //simParams.ONU_TIME_TRIGGER = 0.0001;
  //simParams.ONU_TIME_WAKEUP  = 0.001;
  //simParams.ONU_TIME_PROBE   = 0.00025;

  /* Permanent OLT behavior */
  while(!terminateSim)
  {
    NumGrantsToWait = 0;
    switch(onuAttrs[onuNum].state)
    {
      case ONU_ST_ACTIVE:
        wait(ONU_HAS_NO_QUEUED_PACKETS[onuNum]);
        changeState(onuNum, ONU_ST_IDLE);
        break;
      case ONU_ST_IDLE:
        outcome = timed_wait(PACKET_ARRIVED[onuNum], simParams.ONU_TIME_TRIGGER);
        if(outcome == EVENT_OCCURRED)
          changeState(onuNum, ONU_ST_ACTIVE);
        if(outcome == TIMED_OUT)
          changeState(onuNum, ONU_ST_SLEEP);
        break;
      case ONU_ST_SLEEP:
        hold(simParams.ONU_TIME_SLEEP);
        changeState(onuNum, ONU_ST_PROBE);
        break;
      case ONU_ST_PROBE:
        // Calculate the number of grant cycles until this
        // ONU will receive its cycle
        NumGrantsToWait = (simParams.ONU_GRANTED - onuNum + simParams.NUM_ONU)%32;
        
        // Wait the first half of the probe time when it is 
        // impossible for the probe to receive a report,
        // and then wait the appropriate number of grant cycles.
        hold(simParams.ONU_TIME_PROBE/2.0 + simParams.TIME_PER_GRANT*NumGrantsToWait);

        // Check if any packets have arrived
        outcome = timed_wait(PACKET_ARRIVED[onuNum], simParams.TIME_PER_GRANT);
        
        // If packets were found, stop the Probe state immediately and switch
        // to the active state.
        if(outcome == EVENT_OCCURRED)
          changeState(onuNum, ONU_ST_ACTIVE);
        // If packets were not found, hold for the remainder of the probe
        // period, and then switch back to the sleep state.
        if(outcome == TIMED_OUT)
        {
          hold(simParams.TIME_PER_GRANT*(32-NumGrantsToWait-1));
          changeState(onuNum, ONU_ST_SLEEP);
        }
        break;
      case FINAL_eONU_STATE_ENTRY:
        break;
    } 
  }
}




