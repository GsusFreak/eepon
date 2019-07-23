/*
	TITLE: 		EPON Simulator Statistics Collection Functions
	AUTHOR: 	Michael P. McGarry
	DATE:
	VERSION:	1.0
	
	NOTES:
	
*/

#include "eponsim.h"

/* Simulation End Event */
EVENT SIM_END_EVENT;

// Debugging the Cycle length and the queueing delay mismatch
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

int minSwitch = 1;
double minQueueDelay;

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


/* Record stats after packet arrival */
void record_packet_stats_finish(int onuNum, sENTITY_PKT *pkt)
{
	pkt->arrivalTime = simtime();
	/* Record packet transmission time in proper table */
	record(pkt->arrivalTime - pkt->creationTime, onuAttrs[onuNum].transmitTimeTable);
}

/* Record stats after packet arrival */
void record_video_packet_stats_finish(int onuNum, sENTITY_PKT *pkt)
{
	pkt->arrivalTime = simtime();
	/* Record packet transmission time in proper table */
	record(pkt->arrivalTime - pkt->creationTime, onuAttrs[onuNum].transmitVideoTimeTable);
}




/* Record stats after packet is dequeued */
void record_packet_stats_dequeue(int onuNum)
{
	onuAttrs[onuNum].packetsHead->transmissionTime = simtime();
	/* Record packet queue time in proper table */
	record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, onuAttrs[onuNum].queueTimeTable);
	/* Record packet queue time in overall queueing delay table */
	record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, overallQueueDelay);
	if(onuNum < simParams.NUM_HEAVY_ONU)
	{
		/* Record packet queue time in Heavy ONU queueing delay table */
		record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, heavyQueueDelay);
	}
	else
	{
		/* Record packet queue time in Heavy ONU queueing delay table */
		record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, lightQueueDelay);
	}
	if(onuNum < simParams.NUM_PREFERRED_ONU)
	{
		/* Record packet queue time in Preferred ONU queueing delay table */
		record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, preferQueueDelay);
	}
	if((simParams.TRAFFIC_TYPE == TRAFFIC_SELF_SIMILAR) && (simInitPhase == 0))
	{
		/* Record confidence interval data for Self-similar traffic */
		if(((onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime) < overallQueueDelayStat.intervalLower)
		   || ((onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime) > overallQueueDelayStat.intervalUpper))
		{
			overallQueueDelayStat.outsideIntervalCount++;
			if(onuNum < simParams.NUM_HEAVY_ONU)
			{
				heavyQueueDelayStat.outsideIntervalCount++;
			}
			else
			{
				lightQueueDelayStat.outsideIntervalCount++;
			}
		}
		else
		{
			overallQueueDelayStat.insideIntervalCount++;
			if(onuNum < simParams.NUM_HEAVY_ONU)
			{
				heavyQueueDelayStat.insideIntervalCount++;
			}
			else
			{
				lightQueueDelayStat.insideIntervalCount++;
			}
			
		}
	}
}



/* Record stats after Video packet is dequeued */
void record_video_packet_stats_dequeue(int onuNum)
{
	onuAttrs[onuNum].packetsVideoHead->transmissionTime = simtime();
	/* Record packet queue time in proper table */
	record(onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime, onuAttrs[onuNum].VideoqueueTimeTable);
	/* Record packet queue time in overall queueing delay table */
	record(onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime, overallVideoQueueDelay);
	record(onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime, overallVideoQueueDelay_MovingAverage);
	/* Update the video queue delay values for the MFAC controls dropping algorithm */
	outputPrev_MFAC = output_MFAC;
	output_MFAC = onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime;
}

/* Record stats after packet is dequeued */
void record_packet_stats_dequeue_tx_time(int onuNum)
{
	onuAttrs[onuNum].packetsHead->transmissionTime = simtime() + onuAttrs[onuNum].packetsHead->size*simParams.TIME_PER_BYTE; // Hack that inclues tx time to check Frank's equation
	/* Record packet queue time in proper table */
	record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, onuAttrs[onuNum].queueTimeTable);
    	/* Record packet queue time in delay table for cycle based observation */
	record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, cycleQueueDelay);
	/* Record packet queue time in overall queueing delay table */
	record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, overallQueueDelay);
	if(onuNum < simParams.NUM_HEAVY_ONU)
	{
		/* Record packet queue time in Heavy ONU queueing delay table */
		record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, heavyQueueDelay);
	}
	else
	{
		/* Record packet queue time in Heavy ONU queueing delay table */
		record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, lightQueueDelay);
	}
	if(onuNum < simParams.NUM_PREFERRED_ONU)
	{
		/* Record packet queue time in Preferred ONU queueing delay table */
		record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, preferQueueDelay);
	}
	if((simParams.TRAFFIC_TYPE == TRAFFIC_SELF_SIMILAR) && (simInitPhase == 0))
	{
		/* Record confidence interval data for Self-similar traffic */
		if(((onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime) < overallQueueDelayStat.intervalLower)
		   || ((onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime) > overallQueueDelayStat.intervalUpper))
		{
			overallQueueDelayStat.outsideIntervalCount++;
			if(onuNum < simParams.NUM_HEAVY_ONU)
			{
				heavyQueueDelayStat.outsideIntervalCount++;
			}
			else
			{
				lightQueueDelayStat.outsideIntervalCount++;
			}
		}
		else
		{
			overallQueueDelayStat.insideIntervalCount++;
			if(onuNum < simParams.NUM_HEAVY_ONU)
			{
				heavyQueueDelayStat.insideIntervalCount++;
			}
			else
			{
				lightQueueDelayStat.insideIntervalCount++;
			}
			
		}
	}
}




/* Record stats after Video packet is dequeued */
void record_video_packet_stats_dequeue_tx_time(int onuNum)
{
	onuAttrs[onuNum].packetsVideoHead->transmissionTime = simtime() + onuAttrs[onuNum].packetsVideoHead->size*simParams.TIME_PER_BYTE; // Hack that inclues tx time to check Frank's equation
	/* Record packet queue time in proper table */
	record(onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime, onuAttrs[onuNum].VideoqueueTimeTable);
    	/* Record packet queue time in delay table for cycle based observation */
	//////record(onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime, cycleVideoQueueDelay);
	/* Record packet queue time in overall queueing delay table */
	record(onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime, overallVideoQueueDelay);
	record(onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime, overallVideoQueueDelay_MovingAverage);
	outputPrev_MFAC = output_MFAC;
	output_MFAC = onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime;
	
	
	
	// Debugging the Cycle length and the queueing delay mismatch
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//	if((onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime) <= 122e-6)
//	{
//		//printf("Before minQueueDelay = [%10.5e]\n",minQueueDelay);
//		if(minSwitch == 1)
//		{
//			printf("Cycle Length Mismatch: [%10.10e] Video Queuing Delay for ONU #%d = [%10.10e]\n",simtime(),onuNum,onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime);
//			minQueueDelay = onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime;
//			minSwitch = 0;
//		}
			
//		else if((onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime) <= minQueueDelay)
//		{	
//			//printf("minQueueDelay = [%10.5e]\n",minQueueDelay);
//    			printf("Cycle Length Mismatch: [%10.10e] Video Queuing Delay for ONU #%d = [%10.10e] & Packet Creation Time = [%10.10e] & Packet Transmission Time = [%10.10e]\n",simtime(),onuNum,onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime, onuAttrs[onuNum].packetsVideoHead->creationTime,onuAttrs[onuNum].packetsVideoHead->transmissionTime);
//    			minQueueDelay = onuAttrs[onuNum].packetsVideoHead->transmissionTime - onuAttrs[onuNum].packetsVideoHead->creationTime;
//   		}
//    			//fflush(NULL);
//    			//terminateSim = 1;
//			//set(SIM_END_EVENT);
//		//printf("After minQueueDelay = [%10.5e]\n",minQueueDelay);
//	}
	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	
	
}


/* Record the unused remainder of the Video Grant */
void record_unused_video_grant(int onuNum)
{
	record(remainingVideoGrantLength, unusedVideoGrant);

}


/* Record the Video Report */
void record_video_report(int onuNum)
{
	record(onuAttrs[onuNum].rptVideoQueueSize, videoReport);

}


/* Record stats after packet is dequeued */
void record_packet_stats_dequeue_minus_prop(int onuNum)
{
	onuAttrs[onuNum].packetsHead->transmissionTime = simtime() - onuAttrs[onuNum].latency;
	/* Record packet queue time in proper table */
	record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, onuAttrs[onuNum].queueTimeTable);
	/* Record packet queue time in overall queueing delay table */
	record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, overallQueueDelay);
	if(onuNum < simParams.NUM_HEAVY_ONU)
	{
		/* Record packet queue time in Heavy ONU queueing delay table */
		record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, heavyQueueDelay);
	}
	else
	{
		/* Record packet queue time in Heavy ONU queueing delay table */
		record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime, lightQueueDelay);
	}
	if(onuNum < simParams.NUM_PREFERRED_ONU)
	{
		/* Record packet queue time in Preferred ONU queueing delay table */
		record(onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime,
			   preferQueueDelay);
	}
	if((simParams.TRAFFIC_TYPE == TRAFFIC_SELF_SIMILAR) && (simInitPhase == 0))
	{
		/* Record confidence interval data for Self-similar traffic */
		if(((onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime) < overallQueueDelayStat.intervalLower)
		   || ((onuAttrs[onuNum].packetsHead->transmissionTime - onuAttrs[onuNum].packetsHead->creationTime) > overallQueueDelayStat.intervalUpper))
		{
			overallQueueDelayStat.outsideIntervalCount++;
			if(onuNum < simParams.NUM_HEAVY_ONU)
			{
				heavyQueueDelayStat.outsideIntervalCount++;
			}
			else
			{
				lightQueueDelayStat.outsideIntervalCount++;
			}
		}
		else
		{
			overallQueueDelayStat.insideIntervalCount++;
			if(onuNum < simParams.NUM_HEAVY_ONU)
			{
				heavyQueueDelayStat.insideIntervalCount++;
			}
			else
			{
				lightQueueDelayStat.insideIntervalCount++;
			}
			
		}
	}
}

/* Record stats for queue length */
void record_stats_queue_length(int onuNum)
{
	/* Record queue length in proper table */
	record(onuAttrs[onuNum].packetQueueSize, onuAttrs[onuNum].queueLengthTable);
	record(onuAttrs[onuNum].packetQueueSize, overallQueueLength);
	if(onuNum < simParams.NUM_HEAVY_ONU)
	{
		record(onuAttrs[onuNum].packetQueueSize, heavyQueueLength);
	}
	else
	{
		record(onuAttrs[onuNum].packetQueueSize, lightQueueLength);
	}
	if(onuNum < simParams.NUM_PREFERRED_ONU)
	{
		record(onuAttrs[onuNum].packetQueueSize, preferQueueLength);
	}
}

void record_grant_stats_begin(int onuNum, sENTITY_GATE_PKT *pendingGATE)
{

    /* Record queue length at instant of grant */
    record_stats_queue_length(onuNum);

    /* Note passage of GATE */
    note_passage(overallGrantRate);
    if(onuNum < simParams.NUM_HEAVY_ONU)
    {
        note_passage(heavyGrantRate);
    }
    else
    {
        note_passage(lightGrantRate);
    }

    /* Note GATE size */
    record((pendingGATE->length/simParams.TIME_PER_BYTE),overallGrantSize);
    if(onuNum < simParams.NUM_HEAVY_ONU)
    {
        record((pendingGATE->length/simParams.TIME_PER_BYTE),heavyGrantSize);
    }
    else
    {
        record((pendingGATE->length/simParams.TIME_PER_BYTE),lightGrantSize);
    }

    /* Record if zero or non-zero GATE */
    if(pendingGATE->length == 0)
    {
        note_passage(onuAttrs[onuNum].zeroReqRate);
        note_passage(overallZeroReqRate);
        if(onuNum < simParams.NUM_HEAVY_ONU)
        {
            note_passage(heavyZeroReqRate);
        }
        else
        {
            note_passage(lightZeroReqRate);
        }
    }
    else
    {
        note_passage(onuAttrs[onuNum].nonzeroReqRate);
        note_passage(overallNonzeroReqRate);
        if(onuNum < simParams.NUM_HEAVY_ONU)
        {
            note_passage(heavyNonzeroReqRate);
        }
        else
        {
            note_passage(lightNonzeroReqRate);
        }
    }
#if 0
    /* Note Cycle time */
    if(onuAttrs[onuNum].prevGateTime > 1e-9)
    {
#ifdef DEBUG_TRC_LO
        printf("ONU #%d, cl = %e\n",onuNum,(simtime()-onuAttrs[onuNum].prevGateTime));
#endif
        record((simtime()-onuAttrs[onuNum].prevGateTime),overallCycleLength);
        if(onuNum < simParams.NUM_HEAVY_ONU)
        {
            record((simtime()-onuAttrs[onuNum].prevGateTime),heavyCycleLength);
        }
        else
        {
            record((simtime()-onuAttrs[onuNum].prevGateTime),lightCycleLength);
        }
        if(onuNum < simParams.NUM_PREFERRED_ONU)
        {
            record((simtime()-onuAttrs[onuNum].prevGateTime),preferCycleLength);
        }
    }
#endif
    /* Record REPORT-to-GATE time */
    if(onuAttrs[onuNum].prevGateTime > 1e-9)
    {
        record((simtime() - onuAttrs[onuNum].rptTime),overallRptToGateTime);
        if(onuNum < simParams.NUM_HEAVY_ONU)
        {
            record((simtime() - onuAttrs[onuNum].rptTime),heavyRptToGateTime);
        }
        else
        {
            record((simtime() - onuAttrs[onuNum].rptTime),lightRptToGateTime);
        }
    }
    /* Update previous GATE time */
    onuAttrs[onuNum].prevGateTime = simtime();

}


/*! 
   Calculate the Raj Jain Fairness index
   based on actual Allocation set, A 
   and optimal allocation set, O
   with the following Fairness calculation:
 
 	Xi = Ai/Oi
 
   Fairness = [  sigma(Xi)^2  ] / [ n*sigma(Xi^2) ]
              
 \author John McAlarney
 \param a Actual Allocation set
 \param o Optimal Allocation set
 \param n Number of Samples
 \return Raj Jain Fairness Index
*/
double rj_fairness_index(double * a, double * o, int n)
{
	//Iterator for array indexing
	int i = 0;
	//accumulating numerator for fairness calculation
	double num = 0;
	//accumulating denominator for fairness calculation
	double denom = 0;
	//relative allocation X[i]=A[i]/O[i]
	double x[n];
	
	for(i=0;i<n;i++)
	{
		//find current relative allocation for set
		x[i] = a[i]/o[i];
		//accumulate numerator and denominator
		num += x[i];
		denom += x[i]*x[i];
	}
	
	return ((num*num)/(n*denom));
}

void reset_throughput_counters()
{
	int i;
	
    /* Clear Throughput Counters */
    for(i=0; i < simParams.NUM_ONU; i++)
    {
        onuAttrs[i].transmitByteCnt = 0;
    }

    reset_throughput_flag = 1;
}

/* 
 * FUNCTION: onu_throughput_calc()
 * DESCRIPTION: polls ONUs for throughput statistics
 *
 */
void onu_throughput_calc()
{
    int loopIndex;
    float const poll_rate = 0.1; //times per second
	create("ONU Throughput Calculator");

	while(!terminateSim)
	{
        hold(1/poll_rate);
        if (reset_throughput_flag == 0)
        {
            for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
            {
                /* record throughput over the last polling period */
                actual_tput[loopIndex] = onuAttrs[loopIndex].transmitByteCnt*8*poll_rate;
                /* Mark in throughput table */
                record(actual_tput[loopIndex], onuAttrs[loopIndex].transmitThroughput);
            }
            /* Compute Raj Jain's fairness index and record in a table */
            record(rj_fairness_index(actual_tput,ideal_tput,simParams.NUM_ONU), throughputFairness);
        }
        else
        {
            reset_throughput_flag = 0;
        }

        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            onuAttrs[loopIndex].transmitByteCnt = 0;
        }
    }	
}
