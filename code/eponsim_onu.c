
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
  
  /* Permanent OLT behavior */
  while(!terminateSim)
  {
    switch(onuAttrs[onuNum].state)
    {
      case ONU_ST_ACTIVE:
        break;
      case ONU_ST_IDLE:
        break;
      case ONU_ST_SLEEP:
        break;
      case ONU_ST_PROBE:
        break;
      case FINAL_eONU_STATE_ENTRY:
        break;
    } 
    hold(1);
  }  
}




