/*
	TITLE: 		EPON Simulator ONU Model
	AUTHOR: 	Michael P. McGarry
	DATE:
	VERSION:	1.0
	
	NOTES:
	
*/

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

    if(oltAttrs.packetsHead != NULL)
    {
        tmp = oltAttrs.packetsHead;
        arrivalSum = 0;
        while(tmp != NULL)
        {
            arrivalSum += tmp->creationTime;
            tmp = tmp->next;
        }
        *avgVal = arrivalSum/(double)oltAttrs.packetQueueNum;
    }
    else
    {
        *avgVal = 0;
    }
}

// Hello, joe
void olt()
{
	int transmitPkt;
	long txPktCount = 0;
	double packet_transmission_time;
	sENTITY_PKT currPkt;

	/* Initialize the process */
	procId[0] = '\0';
	sprintf(procId,"OLT");
	create(procId);
	
	// Test Variables
	status_processes_print();
	
	/* Upstream transmission gate functinnality of new OLT */
	while(!terminateSim) /* permanent behavior of the new OLT process moel */
	{
    /* Reset the loop variables */
    txPktCount = 0;
    transmitPkt = 1;
			
		// Service the OLT packet queue
			 
    /* Check for excessive buffer size */
    if(oltAttrs.packetQueueSize > MAX_PKT_BUF_SIZE)
    {
    	fatalErrorCode = FATAL_CAUSE_BUFFER_OVR;
    	dump_sim_core();
    	transmitPkt = 0;
    }
    
    while(transmitPkt)
    {
    	if(oltAttrs.packetsHead == NULL)
    	{
        // If there are no packets, simply a short period of time 
        transmitPkt = 0;
        hold(simParams.TIME_PER_BYTE);
    	}
      else
      {
    	  /* transmit a packet */
    	  /* collect statistics on this packet */
    	  record_packet_stats_dequeue_tx_time(currPkt.onuNum);
    	  /* Copy packet to temporary data structure */
    	  currPkt.creationTime = oltAttrs.packetsHead->creationTime;
    	  currPkt.transmissionTime = oltAttrs.packetsHead->transmissionTime;
    	  currPkt.arrivalTime = oltAttrs.packetsHead->arrivalTime;
    	  currPkt.size = oltAttrs.packetsHead->size;
  
        /* remove packet */
    	  remove_packet();		
    	  
    	  packet_transmission_time = (currPkt.size)*simParams.TIME_PER_BYTE;
    	  
    	  // result = timed_reserve(1, 0.0);
    	  hold(packet_transmission_time);
    	  // release(lambda[1]);
    
    	  /* Increment transmitted packet counter */
    	  txPktCount++;
    
    	  /* Incremement throughput statistics per ONU*/
    	  oltAttrs.transmitByteCnt += currPkt.size;
    	  
    	  /* Collect statistics on this packet */
    	  // ToDo: Integrate a variable for the onu currently being sent to 
        // and call it onuNum.
        //record_packet_stats_finish(onuNum, &currPkt);
     	}

      /* Record number of packets transmitted */
      //record(txPktCount,overallGrantSizePkt);
    }
	}
}




