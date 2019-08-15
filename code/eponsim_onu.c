
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
  
  long outcome;

  //typedef enum {ONU_ST_ACTIVE, ONU_ST_IDLE, ONU_ST_SLEEP, ONU_ST_PROBE, FINAL_eONU_STATE_ENTRY} eONU_STATE;
  //simParams.ONU_TIME_SLEEP   = 0.005;
  //simParams.ONU_TIME_TRIGGER = 0.0001;
  //simParams.ONU_TIME_WAKEUP  = 0.001;
  //simParams.ONU_TIME_PROBE   = 0.00025;

  /* Permanent OLT behavior */
  while(!terminateSim)
  {
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
        outcome = timed_wait(PACKET_ARRIVED[onuNum], simParams.ONU_TIME_PROBE);
        if(outcome == EVENT_OCCURRED)
          changeState(onuNum, ONU_ST_ACTIVE);
        if(outcome == TIMED_OUT)
          changeState(onuNum, ONU_ST_SLEEP);
        break;
      case FINAL_eONU_STATE_ENTRY:
        break;
    } 
  }
}




