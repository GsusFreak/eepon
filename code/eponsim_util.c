
#include <values.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "hungarian.h"
#include "eponsim.h"

/* 
 * General Utility Functions 
 */

int int_max(int A, int B)
{
  if( A > B)
  {
    return A;
  }
  else
  {
    return B;
  }
}

int int_min(int A, int B)
{
  if( A < B)
  {
    return A;
  }
  else
  {
    return B;
  }
}

void rand_permute_int(int *a, int size)
{
  int i;
  for(i = 0; i < size-1; i++)
  {
    int c = uniform(0, size-i);
    int t = a[i]; a[i] = a[i+c]; a[i+c] = t;  /* swap */
  }
}

void rand_permute_double(double *a, int size)
{
  int i;
  for(i = 0; i < size-1; i++)
  {
    int c = uniform(0, size-i);
    double t = a[i]; a[i] = a[i+c]; a[i+c] = t; /* swap */
  }
}

void bsort_int_ascend(int numbers[], int array_size)
{
  int i, j, temp;
  
  for (i = (array_size - 1); i >= 0; i--)
  {
    for (j = 1; j <= i; j++)
    {
      if (numbers[j-1] > numbers[j])
      {
        temp = numbers[j-1];
        numbers[j-1] = numbers[j];
        numbers[j] = temp;
      }
    }
  }
}

void bsort_int_descend(int numbers[], int array_size)
{
  int i, j, temp;
  for (i = (array_size - 1); i >= 0; i--)
  {
    for (j = 1; j <= i; j++)
    {
      if (numbers[j-1] < numbers[j])
      {
        temp = numbers[j-1];
        numbers[j-1] = numbers[j];
        numbers[j] = temp;
      }
    }
  }
}

void bsort_double_ascend(double numbers[], int array_size)
{
  int i, j;
  double temp;
  
  for (i = (array_size - 1); i >= 0; i--)
  {
    for (j = 1; j <= i; j++)
    {
      if (numbers[j-1] > numbers[j])
      {
        temp = numbers[j-1];
        numbers[j-1] = numbers[j];
        numbers[j] = temp;
      }
    }
  }
}

void bsort_double_descend(double numbers[], int array_size)
{
  int i, j;
  double temp;
  
  for (i = (array_size - 1); i >= 0; i--)
  {
    for (j = 1; j <= i; j++)
    {
      if (numbers[j-1] < numbers[j])
      {
        temp = numbers[j-1];
        numbers[j-1] = numbers[j];
        numbers[j] = temp;
      }
    }
  }
}

void grantCycle()
{
  create("Grant Cycle");
  while(!terminateSim)
  {
    // Check to see if the granted ONU can be put to sleep under the HEAVY_TRAFFIC
    // sleep type
    int onuNum = simParams.ONU_GRANTED;
    int queueSize;
    if(onuAttrs[iaa].state == ONU_ST_ACTIVE)
    {
      // Start with the ONU right after the one that was just serviced
      int iaa = oltAttrs.lastONUServiced + 1;
      while(iaa ~= onuNum)
      {
        queueSize += get_ONU_queue_size(iaa);
        iaa++;
        if(iaa == simParams.NUM_ONU)
        {
          iaa = 0;
        }
        if(iaa == onuNum)
        {
          break;
        }
      }
      if(onuAttrs.heavy_traffic_sleep_duration >= simParams.ONU_TIME_WAKEUP)
      {
        // Set the expected sleep duration for this ONU (minus the wakeup duration)
        onuAttrs.heavy_traffic_sleep_duration = queueSize*simParams.TIME_PER_BYTE - simParams.ONU_TIME_WAKEUP;
        // If this ONU will sleep for longer than the wakeup time, then sleep.
        set(HEAVY_TRAFFIC_SLEEP_TRIGGERED[onuNum]);
      }
    } 
    simParams.ONU_GRANTED += 1;
    if(simParams.ONU_GRANTED == simParams.NUM_ONU)
      simParams.ONU_GRANTED = 0;
    hold(simParams.ONU_TIME_PROBE/2.0/simParams.NUM_ONU);   
  }
}

void changeState(int onuNum, eONU_STATE stateNew)
{
  if(simParams.boolRecordStateTime == 1)
  { 
    onuAttrs[onuNum].timeInState[onuAttrs[onuNum].state] += simtime() - onuAttrs[onuNum].timeStateStarted;
    onuAttrs[onuNum].cntState[onuAttrs[onuNum].state]++; 
    //if(onuNum == 0 && table_cnt(overallQueueDelay) < BOBB)
    //  TSprint("[%d] %d state delay : %f sec\n", onuNum, onuAttrs[onuNum].state, simtime() - onuAttrs[onuNum].timeStateStarted); 
  }
  onuAttrs[onuNum].state = stateNew; 
  onuAttrs[onuNum].timeStateStarted = simtime();
}

/* Packet creation function */
sENTITY_PKT *create_a_packet(int size, int onuNum)
{
  sENTITY_PKT *newPkt;
  newPkt = (sENTITY_PKT *)(malloc(sizeof(sENTITY_PKT)));
  test_vars.data_pkt_created[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
  test_vars.data_pkt_created_olt[test_vars.runNum][test_vars.loadOrderCounter]++;
  if(newPkt != NULL)
  {
    newPkt->creationTime = 0;
    newPkt->size = size;
    newPkt->next = NULL;
    newPkt->onuNum = onuNum;
  }
  else
  {  
    /* Fill out some context information */
    dump_msg_buf[0] = '\0';
    sprintf(dump_msg_buf,"Out of memory creating packet of size %d\n",size);
    fatalErrorCode = FATAL_CAUSE_NO_MEM;
    dump_sim_core();
  }
  return newPkt;
}

/* Remove a packet entity from the system */
void remove_packet(int onuNum)
{
  sENTITY_PKT *tmp;
  tmp = oltAttrs.packetsHead[onuNum];
  oltAttrs.packetsHead[onuNum] = oltAttrs.packetsHead[onuNum]->next;
  /* Remove this packets size from the queue size */
  oltAttrs.packetQueueSize -= tmp->size;
  /* Remove this packet from the queue packet count */
  if(oltAttrs.packetQueueNum == 0)
  {
    /* Some error has occurred */
    printf("[%10.5e] FATAL ERROR: Stray Packet [OLT]\n", simtime());
    fatalErrorCode = FATAL_CAUSE_STRAY_PKT;
    /* Fill out some context information */
    dump_msg_buf[0] = '\0';
    sprintf(dump_msg_buf,"on OLT\n");
    dump_sim_core();
  }
  else
  {
    oltAttrs.packetQueueNum--;
    test_vars.data_pkt_destroyed[test_vars.runNum][test_vars.loadOrderCounter][tmp->onuNum]++;
    test_vars.data_pkt_destroyed_olt[test_vars.runNum][test_vars.loadOrderCounter]++;
  }
  free(tmp);
}


void remove_all_packets()
{
  sENTITY_PKT *tmp;
  for(int onuNum = 0; onuNum < simParams.NUM_ONU; onuNum++)
  {
    while(oltAttrs.packetsHead[onuNum] != NULL)
    {
      tmp = oltAttrs.packetsHead[onuNum];
      oltAttrs.packetsHead[onuNum] = oltAttrs.packetsHead[onuNum]->next;
      /* Remove this packets size from the queue size */
      oltAttrs.packetQueueSize -= tmp->size;
      /* Remove this packet from the queue packet count */
      oltAttrs.packetQueueNum--;
      test_vars.data_pkt_destroyed[test_vars.runNum][test_vars.loadOrderCounter][tmp->onuNum]++;
      test_vars.data_pkt_destroyed_olt[test_vars.runNum][test_vars.loadOrderCounter]++;
      free(tmp);
    }
    oltAttrs.packetsTail[onuNum] = NULL;
  }
}

int get_ONU_queue_size(int onuNum)
{
  sENTITY_PKT *tmp;
  int queueSize = 0;
  if(onuAttrs[onuNum].state == ONU_ST_ACTIVE)
  {
    tmp = oltAttrs.packetsHead[onuNum];
    while(tmp != NULL)
    {
      queueSize += tmp->size;
      tmp = tmp->next;
    }
  }
  return queueSize;
}

int get_OLT_queue_size()
{
  sENTITY_PKT *tmp;
  int queueSize = 0;
  for(int onuNum = 0; onuNum < simParams.NUM_ONU; onuNum++)
  {
    if(onuAttrs[onuNum].state == ONU_ST_ACTIVE)
    {
      tmp = oltAttrs.packetsHead[onuNum];
      while(tmp != NULL)
      {
        queueSize += tmp->size;
        tmp = tmp->next;
      }
    }
  }
  return queueSize;
}


/*
 * ONU List Utility Functions
 */
eSORT_EQUAL check_equal(eSORT_CRITERIA sortCriteria, sONU_LIST *node1, sONU_LIST *node2)
{
  switch(sortCriteria)
  {
  case SORT_ONU_NUM:
    if(node1->onuNum == node2->onuNum)
    {
      return SORT_EQ;
    }
    else if (node1->onuNum > node2->onuNum)
    {
      return SORT_GT;
    }
    else
    {
      return SORT_LT;
    }
    break;
  case SORT_POS_NUM:
    if(node1->posNum == node2->posNum)
    {
      return SORT_EQ;
    }
    else if (node1->posNum > node2->posNum)
    {
      return SORT_GT;
    }
    else
    {
      return SORT_LT;
    }
    break;
  case SORT_GRANT_LEN:
    if(node1->grantLen == node2->grantLen)
    {
      return SORT_EQ;
    }
    else if (node1->grantLen > node2->grantLen)
    {
      return SORT_GT;
    }
    else
    {
      return SORT_LT;
    }
    break;
  case SORT_NUM_FRAMES:
    if(node1->numFrames == node2->numFrames)
    {
      return SORT_EQ;
    }
    else if (node1->numFrames > node2->numFrames)
    {
      return SORT_GT;
    }
    else
    {
      return SORT_LT;
    }
    break;
  case SORT_GRANT_TIME:
    if(node1->grantTime == node2->grantTime)
    {
      return SORT_EQ;
    }
    else if (node1->grantTime > node2->grantTime)
    {
      return SORT_GT;
    }
    else
    {
      return SORT_LT;
    }
    break;
  case SORT_POOL_TIME:
    if(node1->poolTime == node2->poolTime)
    {
      return SORT_EQ;
    }
    else if (node1->poolTime > node2->poolTime)
    {
      return SORT_GT;
    }
    else
    {
      return SORT_LT;
    }
    break;
  case SORT_MIN_ARRIVAL:
    if(node1->minArrivalTime == node2->minArrivalTime)
    {
      return SORT_EQ;
    }
    else if (node1->minArrivalTime > node2->minArrivalTime)
    {
      return SORT_GT;
    }
    else
    {
      return SORT_LT;
    }
    break;
  case SORT_AVG_ARRIVAL:
    if(node1->avgArrivalTime == node2->avgArrivalTime)
    {
      return SORT_EQ;
    }
    else if (node1->avgArrivalTime > node2->avgArrivalTime)
    {
      return SORT_GT;
    }
    else
    {
      return SORT_LT;
    }
    break;
  case SORT_PROP_DELAY:
    if(node1->latency == node2->latency)
    {
      return SORT_EQ;
    }
    else if (node1->latency > node2->latency)
    {
      return SORT_GT;
    }
    else
    {
      return SORT_LT;
    }
    break;
  }
  return SORT_EQ;
}

/* Insert an ONU into the ONU list */
sONU_LIST *onu_list_insert(eSORT_METHOD sortMethod1, eSORT_METHOD sortMethod2, eSORT_CRITERIA sortCriteria1, eSORT_CRITERIA sortCriteria2, sONU_LIST *newEntry, sONU_LIST *currentList)
{
  sONU_LIST *newElement, *travPtr, *prevPtr;

  /* Allocate memory for new element */
  newElement = (sONU_LIST *)(malloc(sizeof(sONU_LIST)));
  if(newElement == NULL)
  {
    /* Fill out some context information */
    dump_msg_buf[0] = '\0';
    sprintf(dump_msg_buf,"Out of memory creating ONU list item\n");
    fatalErrorCode = FATAL_CAUSE_NO_MEM;
    dump_sim_core();
  }

  newElement->onuNum = newEntry->onuNum;
  newElement->posNum = newEntry->posNum;
  newElement->grantLen = newEntry->grantLen;
  newElement->numFrames = newEntry->numFrames;
  newElement->grantTime = newEntry->grantTime;
  newElement->poolTime = newEntry->poolTime;
  newElement->minArrivalTime = newEntry->minArrivalTime;
  newElement->avgArrivalTime = newEntry->avgArrivalTime;
  newElement->latency = newEntry->latency;
  newElement->next = NULL;

  /* Check if list is currently empty */
  if (currentList == NULL) 
  {
    return newElement;
  }

  /* Otherwise, traverse the list and insert in proper location */
  prevPtr = NULL;
  travPtr = currentList;
  while(travPtr != NULL)
  {
    switch(check_equal(sortCriteria1,newElement,travPtr))
    {
    case SORT_GT:
      if(sortMethod1 == SORT_DESCENDING_ORDER)
      {
        /* insert before this element */
        if (prevPtr == NULL) 
        {
          newElement->next = currentList;
          return newElement; /* new data is the new head of the list */
        }
        else
        {
          newElement->next = prevPtr->next;
          prevPtr->next = newElement;
          return currentList; /* head of the list is the same */
        }
      }
      break;
    case SORT_LT:
      if(sortMethod1 == SORT_ASCENDING_ORDER)
      {
        /* insert before this element */
        if (prevPtr == NULL) 
        {
          newElement->next = currentList;
          return newElement; /* new data is the new head of the list */
        }
        else
        {
          newElement->next = prevPtr->next;
          prevPtr->next = newElement;
          return currentList; /* head of the list is the same */
        }
      }
      break;
    case SORT_EQ:
      /* Check second criteria */
      switch(check_equal(sortCriteria2,newElement,travPtr))
      {
      case SORT_GT:
      case SORT_EQ:
        if(sortMethod2 == SORT_DESCENDING_ORDER)
        {
          /* insert before this element */
          if (prevPtr == NULL) 
          {
            newElement->next = currentList;
            return newElement; /* new data is the new head of the list */
          }
          else
          {
            newElement->next = prevPtr->next;
            prevPtr->next = newElement;
            return currentList; /* head of the list is the same */
          }
        }
        break;
      case SORT_LT:
        if(sortMethod2 == SORT_ASCENDING_ORDER)
        {
          /* insert before this element */
          if (prevPtr == NULL) 
          {
            newElement->next = currentList;
            return newElement; /* new data is the new head of the list */
          }
          else
          {
            newElement->next = prevPtr->next;
            prevPtr->next = newElement;
            return currentList; /* head of the list is the same */
          }
        }
        break;
      }
    }
    /* move to next element */
    prevPtr = travPtr;
    travPtr = travPtr->next;
  }

  /* append to the end of the list */
  prevPtr->next = newElement;
  newElement->next = NULL;

  return currentList; /* head of the list is the same */
}

/* Pop an ONU from the front of the ONU list */
sONU_LIST *onu_list_pop(sONU_LIST *currentList, sONU_LIST *poppedItem)
{
  sONU_LIST   *newHead;

  if(currentList != NULL)
  {
    poppedItem->onuNum = currentList->onuNum;
    poppedItem->posNum = currentList->posNum;
    poppedItem->grantLen = currentList->grantLen;
    poppedItem->numFrames = currentList->numFrames;
    poppedItem->grantTime = currentList->grantTime;
    poppedItem->poolTime = currentList->poolTime;
    poppedItem->minArrivalTime = currentList->minArrivalTime;
    poppedItem->avgArrivalTime = currentList->avgArrivalTime;
    poppedItem->latency = currentList->latency;
    newHead = currentList->next;
    free(currentList);
  }
  else
  {
    newHead = NULL;
  }

  return newHead;
}

void onu_list_print(sONU_LIST *currentList)
{
  sONU_LIST *travPtr;

  if(currentList == NULL)
  {
    printf("List is empty\n");
  }

  travPtr = currentList;
  while(travPtr != NULL)
  {
    printf("ONU #%d, Grant Len = %ld, Grant Time = %g, Num Frames = %ld, Min Arrival Time = %g, Avg Arrival Time = %g, Avg Arrival Time = %e\n", travPtr->onuNum, travPtr->grantLen, travPtr->grantTime, travPtr->numFrames, travPtr->minArrivalTime, travPtr->avgArrivalTime, travPtr->latency);
    travPtr = travPtr->next;
  }
  fflush(NULL);
}

/*
 * Weighted Bipartite Matching Cost Matrix Utility Functions
 */

void debug_print_cost_matrix(int **cost_matrix, int rows, int cols)
{
  int loopIndex,loopIndex2;
  printf("Cost Matrix\n");
  for(loopIndex=0; loopIndex < rows; loopIndex++)
  {
    for(loopIndex2=0;loopIndex2 < cols; loopIndex2++)
    {
      if(cost_matrix[loopIndex][loopIndex2] == COST_INFINITE)
      {
        printf("Inf  ");
      }
      else
      {
        printf("%4d ",cost_matrix[loopIndex][loopIndex2]);
      }
    }
    printf("\n");
    fflush(NULL);
  }
}

void dump_cost_matrix(int **cost_matrix, int rows, int cols)
{
  FILE *costMatrixFile;
  int loopIndex,loopIndex2;
  
  costMatrixFile = fopen("cm_dump","w");
  fprintf(costMatrixFile,"Cost Matrix\n");
  fflush(NULL);
  for(loopIndex=0; loopIndex < rows; loopIndex++)
  {
    for(loopIndex2=0;loopIndex2 < cols; loopIndex2++)
    {
      fprintf(costMatrixFile,"%4d ",cost_matrix[loopIndex][loopIndex2]);
      fflush(NULL);
    }
    fprintf(costMatrixFile,"\n");
    fflush(NULL);
  }
  fclose(costMatrixFile);
  fflush(NULL);
}




