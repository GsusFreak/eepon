/*
	TITLE: 		EPON Simulator Utility Functions
	AUTHOR: 	Michael P. McGarry
	DATE:
	VERSION:	1.0
	
	NOTES:
	
*/

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
		int t = a[i]; a[i] = a[i+c]; a[i+c] = t;	/* swap */
	}
}

void rand_permute_double(double *a, int size)
{
    int i;
	for(i = 0; i < size-1; i++)
	{
		int c = uniform(0, size-i);
		double t = a[i]; a[i] = a[i+c]; a[i+c] = t;	/* swap */
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

/* Grant Trace flush function */
void grant_trace_flush()
{
    FILE *grantTrcFile;
    int loopIndex, loopIndex2;
    char filename_str[100];

    /* Write what's in the Grant Trace arrays to the files */
    for(loopIndex = 0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
    {
        /* Flush array to a file */
        filename_str[0] = '\0';
        strcat(filename_str, "grant_trc_");
        sprintf(filename_str, "%s%dw", filename_str, loopIndex+1);
        grantTrcFile = fopen(filename_str,"a");
        for(loopIndex2=0; loopIndex2 < grantTracePtr[loopIndex]; loopIndex2++)
        {
            fprintf(grantTrcFile,"%d %e %e\n",grantTrace[loopIndex][loopIndex2].onuNum,
                    grantTrace[loopIndex][loopIndex2].startTime,grantTrace[loopIndex][loopIndex2].length);
        }
        fclose(grantTrcFile);
        /* Reset Pointer */
        grantTracePtr[loopIndex] = 0;
    }
}

/*
 * Packet and GATE message creation/deletion Utility Functions
 */
/* Packet creation function */
sENTITY_PKT *create_a_packet(int size, int onuNum)
{
	sENTITY_PKT *newPkt;
	newPkt = (sENTITY_PKT *)(malloc(sizeof(sENTITY_PKT)));
	test_vars.data_pkt_created[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
    if(newPkt != NULL)
    {
        newPkt->creationTime = 0;
        newPkt->size = size;
        newPkt->next = NULL;
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

/* Video packet creation function -Rami */
// sENTITY_PKT *create_a_video_packet(int size, char frameType, double frameTimeStamp, int forecastSize, int forecastPktNumber, int layer)
sENTITY_PKT *create_a_video_packet(int size, char frameType, double frameTimeStamp, int forecastSize, int forecastPktNumber, int layer, int onuNum)
{
	sENTITY_PKT *newPkt;
	newPkt = (sENTITY_PKT *)(malloc(sizeof(sENTITY_PKT)));
	test_vars.vid_pkt_created[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
    if(newPkt != NULL)
    {
        newPkt->creationTime = 0;
        newPkt->size = size;
        newPkt->frameType = frameType;
        newPkt->frameTimeStamp = frameTimeStamp;
        newPkt->forecastSize = forecastSize;
        newPkt->forecastPktNumber = forecastPktNumber;
        newPkt->layer = layer;
        /* Forecast field, type field, etc. */
        newPkt->next = NULL;
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
	tmp = onuAttrs[onuNum].packetsHead;
	onuAttrs[onuNum].packetsHead = onuAttrs[onuNum].packetsHead->next;
	/* Remove this packets size from the queue size */
	onuAttrs[onuNum].packetQueueSize -= tmp->size;
	/* Remove this packet from the queue packet count */
	if(onuAttrs[onuNum].packetQueueNum == 0)
	{
		/* Some error has occurred */
		printf("[%10.5e] FATAL ERROR: Stray Packet [ONU #%d]\n", simtime(), onuNum);
        fatalErrorCode = FATAL_CAUSE_STRAY_PKT;
		/* Fill out some context information */
		dump_msg_buf[0] = '\0';
		sprintf(dump_msg_buf,"on ONU #%d\n",onuNum);
		dump_sim_core();
	}
	else
	{
		onuAttrs[onuNum].packetQueueNum--;
		test_vars.data_pkt_destroyed[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
	}
	free(tmp);
}


/* Remove a packet entity from the system */
// void remove_video_packet(int onuNum)
void remove_video_packet(int onuNum)
{
	sENTITY_PKT *tmp;
	tmp = onuAttrs[onuNum].packetsVideoHead;
	onuAttrs[onuNum].packetsVideoHead = onuAttrs[onuNum].packetsVideoHead->next;
	/* Remove this packets size from the queue size */
	onuAttrs[onuNum].packetVideoQueueSize -= tmp->size;
	/* Remove this packet from the queue packet count */
	if(onuAttrs[onuNum].packetVideoQueueNum == 0)
	{
		/* Some error has occurred */
		printf("[%10.5e] FATAL ERROR: Stray Packet [ONU #%d]\n", simtime(), onuNum);
		fatalErrorCode = FATAL_CAUSE_STRAY_PKT;
		/* Fill out some context information */
		dump_msg_buf[0] = '\0';
		sprintf(dump_msg_buf,"on ONU #%d\n",onuNum);
		dump_sim_core();
	}
	else
	{
		onuAttrs[onuNum].packetVideoQueueNum--;
		test_vars.vid_pkt_destroyed[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
	}
	free(tmp);
}

 /* Remove all packet entities for specified ONU from the system */
void remove_all_packets(int onuNum)
{
	sENTITY_PKT *tmp;
	while(onuAttrs[onuNum].packetsHead != NULL)
	{
		tmp = onuAttrs[onuNum].packetsHead;
		onuAttrs[onuNum].packetsHead = onuAttrs[onuNum].packetsHead->next;
		 /* Remove this packets size from the queue size */
		onuAttrs[onuNum].packetQueueSize -= tmp->size;
		 /* Remove this packet from the queue packet count */
		onuAttrs[onuNum].packetQueueNum--;
		free(tmp);
		test_vars.data_pkt_destroyed[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
	}
	onuAttrs[onuNum].packetsTail = NULL;
}



// /* Remove all packet entities for specified ONU from the system */
//void remove_all_packets(int onuNum)
//{
	//sENTITY_PKT *tmp;
	//while(onuAttrs[onuNum].packetsVideoHead != NULL)
	//{
		//tmp = onuAttrs[onuNum].packetsVideoHead;
		//onuAttrs[onuNum].packetsVideoHead = onuAttrs[onuNum].packetsVideoHead->next;
		// /* Remove this packets size from the queue size */
		//onuAttrs[onuNum].packetVideoQueueSize -= tmp->size;
		// /* Remove this packet from the queue packet count */
		//onuAttrs[onuNum].packetVideoQueueNum--;
		//free(tmp);
	//}
	//onuAttrs[onuNum].packetsVideoTail = NULL;
	
	//while(onuAttrs[onuNum].packetsHead != NULL)
	//{
		//tmp = onuAttrs[onuNum].packetsHead;
		//onuAttrs[onuNum].packetsHead = onuAttrs[onuNum].packetsHead->next;
		// /* Remove this packets size from the queue size */
		//onuAttrs[onuNum].packetQueueSize -= tmp->size;
		// /* Remove this packet from the queue packet count */
		//onuAttrs[onuNum].packetQueueNum--;
		//free(tmp);
	//}
	//onuAttrs[onuNum].packetsTail = NULL;
//}




/* GATE creation function */
sENTITY_GATE_PKT *create_a_gate()
{
	sENTITY_GATE_PKT *newGate;
	newGate = (sENTITY_GATE_PKT *)(malloc(sizeof(sENTITY_GATE_PKT)));
	test_vars.gate_created[test_vars.runNum][test_vars.loadOrderCounter]++;

    if(newGate == NULL)
    {
        /* Fill out some context information */
        dump_msg_buf[0] = '\0';
        sprintf(dump_msg_buf,"Out of memory creating gate message\n");
        fatalErrorCode = FATAL_CAUSE_NO_MEM;
        dump_sim_core();
    }
	return newGate;
}

/* Remove a GATE packet entity from the system */
void remove_gate(sENTITY_GATE_PKT *tmp)
{
	free(tmp);
	test_vars.gate_destroyed[test_vars.runNum][test_vars.loadOrderCounter]++;
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
        printf("ONU #%d, Grant Len = %ld, Grant Time = %g, Num Frames = %ld, Min Arrival Time = %g, Avg Arrival Time = %g, Avg Arrival Time = %e\n", travPtr->onuNum, travPtr->grantLen, travPtr->grantTime, 
               travPtr->numFrames, travPtr->minArrivalTime, travPtr->avgArrivalTime, travPtr->latency);
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


int find_stair_step(double value, double lowerBound, double upperBound, int bottomStep, int topStep) {
	// This function finds the discrete step that a value falls on given the values for the
	// gound floor (lowerBound) which is at zero and the top floor (upperBound) which is at
	// numSteps. 
	
	int numSteps = topStep - bottomStep,
		step;
	double threshold;
	for (step = 0; step < numSteps; step++) {
		threshold = (upperBound - lowerBound)*step/(numSteps - 1) + lowerBound;
		if (value <= threshold) return (numSteps - step + bottomStep);
	}
	
	return bottomStep;
}


int find_stair_step_exp(double value, double lowerBound, double upperBound, int bottomStep, int topStep, double expAdjust) {
	// This function finds the discrete step that a value falls on given the values for the
	// gound floor (lowerBound) which is at zero and the top floor (upperBound) which is at
	// numSteps. 
	
	int numSteps = topStep - bottomStep,
		step;
	double threshold;
	for (step = 0; step < numSteps; step++) {
		//threshold = (upperBound - lowerBound)*step/(numSteps - 1) + lowerBound;
		threshold = (upperBound - lowerBound)*log(expAdjust*step/(numSteps - 1) + 1)/log(expAdjust + 1) + lowerBound;
		if (value <= threshold) return (numSteps - step + bottomStep);
	}
	
	return bottomStep;
}

void drop_scalable_video_packets(int onuNum, double video_grant, double lower_bound, double upper_bound, int select) {
	sENTITY_PKT		*prevPktPtr = NULL,
					*currPktPtr = NULL,
					*tempPktPtr = NULL;
	
	if (select == 1) {
		// Dropping for removing whole enhancement layers
		if (simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM == SCALABLE_VIDEO_DROPPING_THRESHOLD) {
			currPktPtr = onuAttrs[onuNum].packetsVideoHead;
			while (currPktPtr != NULL) {
				if (currPktPtr->layer > simParams.SV_DROP_LAYER_THRESHOLD) {
					// Drop the offending packet
					tempPktPtr = currPktPtr;
					
					// The current packet will be removed.
					// That is, if the currPktPtr is the packetsVideoHead
					if (currPktPtr != onuAttrs[onuNum].packetsVideoHead) {
						prevPktPtr->next = currPktPtr->next;
					}
					else {
						onuAttrs[onuNum].packetsVideoHead = currPktPtr->next;
					}
					currPktPtr = currPktPtr->next;
					
					if (onuNum == 0) fprintf(droppedScalPackets, "%d,%c,%d,%d,%.1f\n", 
							(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
							tempPktPtr->frameType,
							tempPktPtr->size,
							tempPktPtr->layer,
							0.1*(test_vars.loadOrderCounter + 1));
					
					//if (onuNum == 0) fprintf(droppedScalPackets, "__\t\t%d\t\t%c\t\t%d\t\t58\t\t??\t\t%d\n", 
							//(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
							//tempPktPtr->frameType,
							//tempPktPtr->size - 58,
							//tempPktPtr->layer);

					//if (onuNum == 0) TSprint("__\t\t%d\t\t%c\t\t%d\t\t58\t\t??\t\t%d\n", 
							//(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
							//tempPktPtr->frameType,
							//tempPktPtr->size - 58,
							//tempPktPtr->layer);
					
					onuAttrs[onuNum].packetVideoQueueSize -= tempPktPtr->size;
					if(onuAttrs[onuNum].packetVideoQueueNum == 0)
					{
						/* Some error has occurred */
						printf("[%10.5e] FATAL ERROR: Stray Packet [ONU #%d]\n", simtime(), onuNum);
						fatalErrorCode = FATAL_CAUSE_STRAY_PKT;
						/* Fill out some context information */
						dump_msg_buf[0] = '\0';
						sprintf(dump_msg_buf,"on ONU #%d\n",onuNum);
						dump_sim_core();
					}
					else
					{
						onuAttrs[onuNum].packetVideoQueueNum--;
						test_vars.vid_pkt_destroyed[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
					}
					free(tempPktPtr);
				}
				else {
					// Increment the loop
					prevPktPtr = currPktPtr;
					currPktPtr = currPktPtr->next;
				}
			}		
		}
	}
	
	if (select == 2) {
		// Dropping for removing a dynamic number of packets (a sort of smoothing) 
		if (simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM == SCALABLE_VIDEO_DROPPING_DYNAMIC) {
			int queueSize = onuAttrs[onuNum].rptVideoQueueSize;
			int grantLimit = (int)(video_grant);
		//	int grantLimit = (int)(simParams.SV_DROP_SENSITIVITY*video_grant);
			int currentThreshold = simParams.SCALABLE_VIDEO_NUM_LAYERS - 1;
			
			currPktPtr = onuAttrs[onuNum].packetsVideoHead;
			if (currPktPtr != NULL) {
				while (queueSize > grantLimit) {
					if (currPktPtr == NULL) {
						if (currentThreshold > 1) {
							currentThreshold -= 1;
							currPktPtr = onuAttrs[onuNum].packetsVideoHead;
						}
						else break;
					}
					
					if (currPktPtr->layer > currentThreshold) {
						// Drop the offending packet
						tempPktPtr = currPktPtr;
						
						// The current packet will be removed.
						// That is, if the currPktPtr is the packetsVideoHead
						if (currPktPtr != onuAttrs[onuNum].packetsVideoHead) {
							prevPktPtr->next = currPktPtr->next;
						}
						else {
							onuAttrs[onuNum].packetsVideoHead = currPktPtr->next;
						}
						currPktPtr = currPktPtr->next;
						
						if (onuNum == 0) fprintf(droppedScalPackets, "%d,%c,%d,%d,%.1f\n", 
								(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
								tempPktPtr->frameType,
								tempPktPtr->size,
								tempPktPtr->layer,
								0.1*(test_vars.loadOrderCounter + 1));
						
						//if (onuNum == 0) fprintf(droppedScalPackets, "__\t\t%d\t\t%c\t\t%d\t\t58\t\t??\t\t%d\n", 
								//(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
								//tempPktPtr->frameType,
								//tempPktPtr->size - 58,
								//tempPktPtr->layer);

						//if (onuNum == 0) TSprint("__\t\t%d\t\t%c\t\t%d\t\t58\t\t??\t\t%d\n", 
								//(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
								//tempPktPtr->frameType,
								//tempPktPtr->size - 58,
								//tempPktPtr->layer);
						
						onuAttrs[onuNum].packetVideoQueueSize -= tempPktPtr->size;
						if (onuAttrs[onuNum].rptVideoQueueSize < (tempPktPtr->size + PREAMBLE_IPG_BYTES))
							TSprint("Error: rptVideoQueueSize < tempPktPtr->size\n");
						else
							onuAttrs[onuNum].rptVideoQueueSize -= (tempPktPtr->size + PREAMBLE_IPG_BYTES);
						queueSize -= (tempPktPtr->size + PREAMBLE_IPG_BYTES);
						if(onuAttrs[onuNum].packetVideoQueueNum == 0)
						{
							/* Some error has occurred */
							printf("[%10.5e] FATAL ERROR: Stray Packet [ONU #%d]\n", simtime(), onuNum);
							fatalErrorCode = FATAL_CAUSE_STRAY_PKT;
							/* Fill out some context information */
							dump_msg_buf[0] = '\0';
							sprintf(dump_msg_buf,"on ONU #%d\n",onuNum);
							dump_sim_core();
						}
						else
						{
							onuAttrs[onuNum].packetVideoQueueNum--;
							if (onuAttrs[onuNum].rptVideoQueueNum == 0)
								TSprint("Error: rptVideoQueueNum == 0\n");
							else
								onuAttrs[onuNum].rptVideoQueueNum--;
							test_vars.vid_pkt_destroyed[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
						}
						free(tempPktPtr);
					}
					else {
						// Increment the loop
						prevPktPtr = currPktPtr;
						currPktPtr = currPktPtr->next;
					}
					
					
				}
			}
			// else what?
		}
	}
	
	if (select == 3) {
		// Dropping packets when they cross a linear threshold based on a moving average of the video delay
		if (simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM == SCALABLE_VIDEO_DROPPING_LINEAR_THRESHOLD) {
			double currentVideoQueueDelay = table_mean(overallVideoQueueDelay_MovingAverage);
			//TSprint("The current Video Queue Delay is: %lf\n", currentVideoQueueDelay);
			int base_layer_num = 1;
			int threshold_layer = find_stair_step(currentVideoQueueDelay, simParams.SV_DROP_MIN_BOUND, simParams.SV_DROP_MAX_BOUND, base_layer_num, simParams.SCALABLE_VIDEO_NUM_LAYERS);
			//TSprint("The current threshold layer is: %d\n", threshold_layer);
			
			currPktPtr = onuAttrs[onuNum].packetsVideoHead;
			while (currPktPtr != NULL) {
				if (currPktPtr->layer > threshold_layer) {
					// Drop the offending packet
					tempPktPtr = currPktPtr;
					
					// The current packet will be removed.
					// That is, if the currPktPtr is the packetsVideoHead
					if (currPktPtr != onuAttrs[onuNum].packetsVideoHead) {
						prevPktPtr->next = currPktPtr->next;
					}
					else {
						onuAttrs[onuNum].packetsVideoHead = currPktPtr->next;
					}
					currPktPtr = currPktPtr->next;
					
					if (onuNum == 0) fprintf(droppedScalPackets, "%d,%c,%d,%d,%.1f\n", 
							(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
							tempPktPtr->frameType,
							tempPktPtr->size,
							tempPktPtr->layer,
							0.1*(test_vars.loadOrderCounter + 1));
					
					//if (onuNum == 0) fprintf(droppedScalPackets, "__\t\t%d\t\t%c\t\t%d\t\t58\t\t??\t\t%d\n", 
							//(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
							//tempPktPtr->frameType,
							//tempPktPtr->size - 58,
							//tempPktPtr->layer);

					//if (onuNum == 0) TSprint("__\t\t%d\t\t%c\t\t%d\t\t58\t\t??\t\t%d\n", 
							//(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
							//tempPktPtr->frameType,
							//tempPktPtr->size - 58,
							//tempPktPtr->layer);
					
					onuAttrs[onuNum].packetVideoQueueSize -= tempPktPtr->size;
					if(onuAttrs[onuNum].packetVideoQueueNum == 0)
					{
						/* Some error has occurred */
						printf("[%10.5e] FATAL ERROR: Stray Packet [ONU #%d]\n", simtime(), onuNum);
						fatalErrorCode = FATAL_CAUSE_STRAY_PKT;
						/* Fill out some context information */
						dump_msg_buf[0] = '\0';
						sprintf(dump_msg_buf,"on ONU #%d\n",onuNum);
						dump_sim_core();
					}
					else
					{
						onuAttrs[onuNum].packetVideoQueueNum--;
						test_vars.vid_pkt_destroyed[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
					}
					free(tempPktPtr);
				}
				else {
					// Increment the loop
					prevPktPtr = currPktPtr;
					currPktPtr = currPktPtr->next;
				}
			}		
		}
	}

	if (select == 4) {
		// Drop packets based on a Discrete Controls Algorithm (CFDL-MFAC)
		if (simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM == SCALABLE_VIDEO_DROPPING_CFDL_MFAC) {
			// Use the lower bound as the desired reference signal so that it will try to minimize
			// video queue delay just enough so as not to drop any packets
			//double	referenceSignal = lower_bound;
			
			// Set the initial values for the inputs to be the middle of the stairstep
			if (flag_firstRun_MFAC == 0) {
				input_MFAC = (simParams.SV_DROP_MIN_BOUND + simParams.SV_DROP_MAX_BOUND)/2.0;
				inputPrev_MFAC = (simParams.SV_DROP_MIN_BOUND + simParams.SV_DROP_MAX_BOUND)/2.0;
				flag_firstRun_MFAC = 1;
			}
			
			double	referenceSignal = (simParams.SV_DROP_MIN_BOUND + simParams.SV_DROP_MAX_BOUND)/2.0;
			// Please note that the system output values are updated directly in eponsim_stats.c
			// Calculate the input and output deltas
			double	inputDelta = input_MFAC - inputPrev_MFAC;
			double	outputDelta = output_MFAC - outputPrev_MFAC;
			// Update the value of psi_MFAC
			psi_MFAC = psi_MFAC + eta_MFAC*inputDelta*(outputDelta - psi_MFAC*inputDelta)/(mu_MFAC + inputDelta*inputDelta);
			// Check to make sure the magnitude of psi_MFAC stays greater than epsilon (the minimum)
			if (epsilon >= fabs(psi_MFAC)) {
				psi_MFAC = psiInitial_MFAC;
				TSprint("psi mag reset\n");
			}
			// Check to make sure the sign of psi_MFAC is the same as the sign as psi_MFAC
			if (((psi_MFAC > 0) && (psiInitial_MFAC < 0)) || ((psi_MFAC < 0) && (psiInitial_MFAC > 0))) {
				psi_MFAC = psiInitial_MFAC;
				TSprint("psi sign reset\n");
			}
			inputPrev_MFAC = input_MFAC;
			input_MFAC = input_MFAC + rho_MFAC*psi_MFAC*(referenceSignal - output_MFAC)/(lambda_MFAC + psi_MFAC*psi_MFAC);
			
			// If the input goes too far away from the stairstep, force it back. 
			if (input_MFAC > simParams.SV_DROP_MAX_BOUND * simParams.SCALABLE_VIDEO_NUM_LAYERS/(simParams.SCALABLE_VIDEO_NUM_LAYERS - 1.0)) {
				input_MFAC = simParams.SV_DROP_MAX_BOUND * simParams.SCALABLE_VIDEO_NUM_LAYERS/(simParams.SCALABLE_VIDEO_NUM_LAYERS - 1.0);
				//TSprint("max bound passed\n");
			}
			if (input_MFAC < simParams.SV_DROP_MIN_BOUND * (simParams.SCALABLE_VIDEO_NUM_LAYERS - 1.0)/simParams.SCALABLE_VIDEO_NUM_LAYERS) {
				input_MFAC = simParams.SV_DROP_MIN_BOUND * (simParams.SCALABLE_VIDEO_NUM_LAYERS - 1.0)/simParams.SCALABLE_VIDEO_NUM_LAYERS;
				//TSprint("min bound passed\n");
			}
			//if ((output_MFAC - referenceSignal) > 0) {
				//TSprint("Should upswing\n");
			//}
			/*
			TSprint("psi = %lf\n", psi_MFAC);
			TSprint("input = %e\n", input_MFAC);
			TSprint("output = %e\n", output_MFAC);
			//TSprint("%e\n", output_MFAC);
			TSprint("-------------------------------\n");
			*/
			
			
			/* Start Borrowed Section */
			// Borrow the linear threshold dropping code but use input_MFAC as the input instead of the moving average
			double currentVideoQueueDelay = input_MFAC;
			int base_layer_num = 1;
			int threshold_layer = find_stair_step(currentVideoQueueDelay, simParams.SV_DROP_MIN_BOUND, simParams.SV_DROP_MAX_BOUND, base_layer_num, simParams.SCALABLE_VIDEO_NUM_LAYERS);
			
			currPktPtr = onuAttrs[onuNum].packetsVideoHead;
			while (currPktPtr != NULL) {
				if (currPktPtr->layer > threshold_layer) {
					// Drop the offending packet
					tempPktPtr = currPktPtr;
					
					// The current packet will be removed.
					// That is, if the currPktPtr is the packetsVideoHead
					if (currPktPtr != onuAttrs[onuNum].packetsVideoHead) {
						prevPktPtr->next = currPktPtr->next;
					}
					else {
						onuAttrs[onuNum].packetsVideoHead = currPktPtr->next;
					}
					currPktPtr = currPktPtr->next;
					
					if (onuNum == 0) fprintf(droppedScalPackets, "%d,%c,%d,%d,%.1f\n", 
							(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
							tempPktPtr->frameType,
							tempPktPtr->size,
							tempPktPtr->layer,
							0.1*(test_vars.loadOrderCounter + 1));
					
					onuAttrs[onuNum].packetVideoQueueSize -= tempPktPtr->size;
					if(onuAttrs[onuNum].packetVideoQueueNum == 0)
					{
						/* Some error has occurred */
						printf("[%10.5e] FATAL ERROR: Stray Packet [ONU #%d]\n", simtime(), onuNum);
						fatalErrorCode = FATAL_CAUSE_STRAY_PKT;
						/* Fill out some context information */
						dump_msg_buf[0] = '\0';
						sprintf(dump_msg_buf,"on ONU #%d\n",onuNum);
						dump_sim_core();
					}
					else
					{
						onuAttrs[onuNum].packetVideoQueueNum--;
						test_vars.vid_pkt_destroyed[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
					}
					free(tempPktPtr);
				}
				else {
					// Increment the loop
					prevPktPtr = currPktPtr;
					currPktPtr = currPktPtr->next;
				}
			}		
			/* End Borrowed Section */
		}
	}
	
	if (select == 5) {
		// Dropping packets when they cross an exponential threshold based on a moving average of the video delay
		if (simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM == SCALABLE_VIDEO_DROPPING_EXP_THRESHOLD) {
			//TSprint("I think it made it in.\n");
			double currentVideoQueueDelay = table_mean(overallVideoQueueDelay_MovingAverage);
			//TSprint("The current Video Queue Delay is: %lf\n", currentVideoQueueDelay);
			int base_layer_num = 1;
			int threshold_layer = find_stair_step_exp(currentVideoQueueDelay, simParams.SV_DROP_MIN_BOUND, simParams.SV_DROP_MAX_BOUND, base_layer_num, simParams.SCALABLE_VIDEO_NUM_LAYERS, simParams.SV_DROP_EXP_ADJUST);
			//TSprint("The current threshold layer is: %d\n", threshold_layer);
			
			currPktPtr = onuAttrs[onuNum].packetsVideoHead;
			while (currPktPtr != NULL) {
				if (currPktPtr->layer > threshold_layer) {
					// Drop the offending packet
					tempPktPtr = currPktPtr;
					
					// The current packet will be removed.
					// That is, if the currPktPtr is the packetsVideoHead
					if (currPktPtr != onuAttrs[onuNum].packetsVideoHead) {
						prevPktPtr->next = currPktPtr->next;
					}
					else {
						onuAttrs[onuNum].packetsVideoHead = currPktPtr->next;
					}
					currPktPtr = currPktPtr->next;
					
					if (onuNum == 0) fprintf(droppedScalPackets, "%d,%c,%d,%d,%.1f\n", 
							(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
							tempPktPtr->frameType,
							tempPktPtr->size,
							tempPktPtr->layer,
							0.1*(test_vars.loadOrderCounter + 1));
					
					//if (onuNum == 0) fprintf(droppedScalPackets, "__\t\t%d\t\t%c\t\t%d\t\t58\t\t??\t\t%d\n", 
							//(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
							//tempPktPtr->frameType,
							//tempPktPtr->size - 58,
							//tempPktPtr->layer);

					//if (onuNum == 0) TSprint("__\t\t%d\t\t%c\t\t%d\t\t58\t\t??\t\t%d\n", 
							//(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
							//tempPktPtr->frameType,
							//tempPktPtr->size - 58,
							//tempPktPtr->layer);
					
					onuAttrs[onuNum].packetVideoQueueSize -= tempPktPtr->size;
					if(onuAttrs[onuNum].packetVideoQueueNum == 0)
					{
						/* Some error has occurred */
						printf("[%10.5e] FATAL ERROR: Stray Packet [ONU #%d]\n", simtime(), onuNum);
						fatalErrorCode = FATAL_CAUSE_STRAY_PKT;
						/* Fill out some context information */
						dump_msg_buf[0] = '\0';
						sprintf(dump_msg_buf,"on ONU #%d\n",onuNum);
						dump_sim_core();
					}
					else
					{
						onuAttrs[onuNum].packetVideoQueueNum--;
						test_vars.vid_pkt_destroyed[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
					}
					free(tempPktPtr);
				}
				else {
					// Increment the loop
					prevPktPtr = currPktPtr;
					currPktPtr = currPktPtr->next;
				}
			}		
		}
	}

	return;
}





//void drop_specific_video_packet(sENTITY_PKT *currPktPtr, sENTITY_PKT *prevPktPtr, int onuNum) {
	//TSprint("Entered dropping function.\n");
	//sENTITY_PKT	*tempPktPtr = NULL;
	
	//tempPktPtr = currPktPtr;
	
	//// The current packet will be removed.
	//// That is, if the currPktPtr is the packetsVideoHead
	//if (currPktPtr != onuAttrs[onuNum].packetsVideoHead) {
		//prevPktPtr->next = currPktPtr->next;
	//}
	//else {
		//onuAttrs[onuNum].packetsVideoHead = currPktPtr->next;
	//}
	//currPktPtr = currPktPtr->next;
	
	//if (onuNum == 0) fprintf(droppedScalPackets, "__\t\t%d\t\t%c\t\t%d\t\t58\t\t??\t\t%d\n", 
			//(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
			//tempPktPtr->frameType,
			//tempPktPtr->size - 58,
			//tempPktPtr->layer);

	//if (onuNum == 0) TSprint("__\t\t%d\t\t%c\t\t%d\t\t58\t\t??\t\t%d\n", 
			//(int)(tempPktPtr->frameTimeStamp - simParams.TIME_SHIFT/1000.0 * onuNum - onuAttrs[onuNum].startoffset*1000),
			//tempPktPtr->frameType,
			//tempPktPtr->size - 58,
			//tempPktPtr->layer);
	
	//onuAttrs[onuNum].packetVideoQueueSize -= tempPktPtr->size;
	//if(onuAttrs[onuNum].packetVideoQueueNum == 0)
	//{
		// /* Some error has occurred */
		//printf("[%10.5e] FATAL ERROR: Stray Packet [ONU #%d]\n", simtime(), onuNum);
		//fatalErrorCode = FATAL_CAUSE_STRAY_PKT;
		// /* Fill out some context information */
		//dump_msg_buf[0] = '\0';
		//sprintf(dump_msg_buf,"on ONU #%d\n",onuNum);
		//dump_sim_core();
	//}
	//else
	//{
		//onuAttrs[onuNum].packetVideoQueueNum--;
		//test_vars.vid_pkt_destroyed[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
	//}
	//free(tempPktPtr);
	
	//return;
//}

//void drop_scalable_video_packets(int onuNum) {
	//sENTITY_PKT	*prevPktPtr = NULL,
					//*currPktPtr = NULL;
	//// int		numEnhLayers = simParams.SCALABLE_VIDEO_NUM_LAYERS;
	//// char	line[200];
	//// int new_top_enh_layer = 2;
	
	////fprintf(droppedScalPackets, "Started\n");
	
	////fclose(droppedScalPackets);
	////droppedScalPackets = fopen("A_Dropped_Scalable_Packets", "a");
	//if (simParams.SCALABLE_VIDEO_DROPPING_ALGORITHM == SCALABLE_VIDEO_DROPPING_THRESHOLD) {
		//currPktPtr = onuAttrs[onuNum].packetsVideoHead;
		//while (currPktPtr != NULL) {
			//if (currPktPtr->layer > simParams.SV_DROP_LAYER_THRESHOLD) {
				//TSprint("Entered if 1\n");
				//// Drop the offending packet
				//drop_specific_video_packet(currPktPtr, prevPktPtr, onuNum);
			//}
			//else {
				//// Increment the loop
				//prevPktPtr = currPktPtr;
				//currPktPtr = currPktPtr->next;
			//}
		//}		
	//}
	
	//return;
//}

