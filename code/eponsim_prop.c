
#include <stdio.h>
#include <csim.h>
#include "eponsim.h"
#include "eponsim_util.h"

#define DIST_TO_PROP(A) ((A)/(299792458*0.66667))

STREAM  propDelayStream;

void onu_prop_delay_distr(int runNumber)
{
  int loopIndex;
  /* For prop delay output to file */
  FILE *propFile;
  sONU_LIST onuListItem;
  double prop1[MAX_ONU], prop2[MAX_ONU];

  /* Only draw propagation delays for the first run */
  if(runNumber == 0)
  {
    if(simParams.FIXED_PROP_DELAY == FIXED_PROP_DELAY_ON)
    {
      /* Setup ONU latency/propagation delays */
      for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
      {
        simParams.ONU_PROP[loopIndex] = simParams.FIXED_PROP_DELAY_VALUE;
      }
      simParams.ACTUAL_MAX_PROP_DELAY = simParams.FIXED_PROP_DELAY_VALUE;
      simParams.ACTUAL_MIN_PROP_DELAY = simParams.FIXED_PROP_DELAY_VALUE;
    }
    else
    {
      /* Create random number stream for latency */
      propDelayStream = create_stream();
      // Force an ONU to have the minimum prop delay and another to have the maximum prop delay
      simParams.ACTUAL_MAX_PROP_DELAY = simParams.MAX_PROP_DELAY;
      simParams.ACTUAL_MIN_PROP_DELAY = 6.68e-6;
      simParams.ONU_PROP[0] = 6.68e-6;
      simParams.ONU_PROP[1] = simParams.MAX_PROP_DELAY;
  
      /* Setup ONU latency/propagation delays */
      for(loopIndex=2; loopIndex < simParams.NUM_ONU; loopIndex++)
      {
        simParams.ONU_PROP[loopIndex] = stream_uniform(propDelayStream,6.68e-6, simParams.MAX_PROP_DELAY);
        if(simParams.ONU_PROP[loopIndex] > simParams.ACTUAL_MAX_PROP_DELAY)
        {
          simParams.ACTUAL_MAX_PROP_DELAY = simParams.ONU_PROP[loopIndex];
        }
      }
    }
  }

  switch(runNumber)
  {
    case 1: /* Sort prop delays in ascending order */
      bsort_double_ascend(simParams.ONU_PROP,simParams.NUM_ONU);
      break;
    case 2: /* Sort prop delays in descending order */
      bsort_double_descend(simParams.ONU_PROP,simParams.NUM_ONU);
      break;
    case 3: /* Sort prop delays in ascending order */
      propFile = fopen("prop","a");
      fprintf(propFile,"Sinusoid pattern:");
      bsort_double_ascend(simParams.ONU_PROP,simParams.NUM_ONU);
      /* Round robin them among two lists */
      for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex+=2)
      {
        prop1[loopIndex/2] = simParams.ONU_PROP[loopIndex];
        if(loopIndex+1 < simParams.NUM_ONU)
        {
          prop2[loopIndex/2] = simParams.ONU_PROP[loopIndex+1];
        }
        fprintf(propFile,"prop1[%d] = %g, prop2[%d] = %g\n",loopIndex/2,prop1[loopIndex/2],loopIndex/2,prop2[loopIndex/2]);
      }
      /* Sort the first list in ascending order */
      //bsort_double_ascend(prop1,(simParams.NUM_ONU/2)+(simParams.NUM_ONU%2));
      /* Sort the second list in descending order */
      bsort_double_descend(prop2,(simParams.NUM_ONU/2));
      /* Merge the two lists into ONU_PROP */
      for(loopIndex=0; loopIndex < ((simParams.NUM_ONU/2)+(simParams.NUM_ONU%2)); loopIndex++)
      {
        simParams.ONU_PROP[loopIndex] = prop1[loopIndex];
        fprintf(propFile,"ONU_PROP[%d] = %g\n",loopIndex,simParams.ONU_PROP[loopIndex]);
      }
      for(loopIndex=((simParams.NUM_ONU/2)+(simParams.NUM_ONU%2)); loopIndex < simParams.NUM_ONU; loopIndex++)
      {
        simParams.ONU_PROP[loopIndex] = prop2[loopIndex-(simParams.NUM_ONU/2)+(simParams.NUM_ONU%2)];
        fprintf(propFile,"ONU_PROP[%d] = %g\n",loopIndex,simParams.ONU_PROP[loopIndex]);
      }
      fclose(propFile);
      break;
    default:rand_permute_double(simParams.ONU_PROP,simParams.NUM_ONU);
      break;
  }

  propFile = fopen("prop","a");
  fprintf(propFile,"\n\n");
  for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
  {
    fprintf(propFile, "%g, ",simParams.ONU_PROP[loopIndex]);
  }
  fprintf(propFile,"\n\n");

  /*
   * Assign the propagation delays to ONUs and print in ascending order (i.e., SPD order)
   */
  for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
  {
    /* Fill out ONU list record for ONU */
    onuListItem.onuNum = loopIndex;
    //onuListItem.latency = DIST_TO_PROP(distance[loopIndex]*1000);
    onuListItem.latency = simParams.ONU_PROP[loopIndex];
    /* Place ONU into sorted list, only first run */
    if(runNumber == 0)
    {
      scheduleList[0] = onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_PROP_DELAY,SORT_ONU_NUM,&onuListItem,scheduleList[0]);
    }
  }
  /* Only print propagation delays in SPD order for first run */
  if(runNumber == 0)
  {
    while(scheduleList[0] != NULL)
    {
      scheduleList[0] = onu_list_pop(scheduleList[0],&onuListItem);
      fprintf(propFile, "%g, ",onuListItem.latency);
    }
    fprintf(propFile,"\n");
  }
  fclose(propFile);
}




