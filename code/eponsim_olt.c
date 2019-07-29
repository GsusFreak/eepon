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


void check_data_packet_list(int onuNum) {
  sENTITY_PKT *pktPtr;
	int length = 0;
	pktPtr = onuAttrs[onuNum].packetsHead;
	while (pktPtr != NULL) {
		pktPtr = pktPtr->next;
		length++;
	}
	if (length > DATA_QUEUE_MAX_LENGTH) {
		fatalErrorCode = FATAL_CAUSE_LENGTH_DATA_BUFFER_OVR;
		dump_sim_core();
	}
	//while (length > DATA_QUEUE_MAX_LENGTH) {
		//remove_packet(onuNum);
		//length--;
	//}
	
	return;
}

void check_video_packet_list(int onuNum) {
	sENTITY_PKT *pktPtr;
	int length = 0;
	pktPtr = onuAttrs[onuNum].packetsVideoHead;
	while (pktPtr != NULL) {
		pktPtr = pktPtr->next;
		length++;
	}
	if (length > VIDEO_QUEUE_MAX_LENGTH) {
		fatalErrorCode = FATAL_CAUSE_LENGTH_VIDEO_BUFFER_OVR;
		dump_sim_core();
	}
	//while (length > VIDEO_QUEUE_MAX_LENGTH) {
		//remove_video_packet(onuNum);
		//length--;
	//}
	
	return;
}


/* Calculate packet arrival time statistics */
void calc_avg_arrival(int onuNum, double *avgVal)
{
    sENTITY_PKT *tmp;
    double  arrivalSum;

    if(onuAttrs[onuNum].packetsHead != NULL)
    {
        tmp = onuAttrs[onuNum].packetsHead;
        arrivalSum = 0;
        while(tmp != NULL)
        {
            arrivalSum += tmp->creationTime;
            tmp = tmp->next;
        }
        *avgVal = arrivalSum/(double)onuAttrs[onuNum].packetQueueNum;
    }
    else
    {
        *avgVal = 0;
    }
}

/* Report the data queue size*/
void mpcp_report(int onuNum)
{
    onuAttrs[onuNum].rptQueueSize = onuAttrs[onuNum].packetQueueSize + onuAttrs[onuNum].packetQueueNum * PREAMBLE_IPG_BYTES;
    onuAttrs[onuNum].rptQueueNum = onuAttrs[onuNum].packetQueueNum;
    if(onuAttrs[onuNum].packetsHead != NULL)
    {
        onuAttrs[onuNum].minArrivalTime = onuAttrs[onuNum].packetsHead->creationTime;
        switch(simParams.OLT_TYPE)
        {
        case OLT_EAAF:
        case OLT_ONLINE_JIT_EAAF:
        case OLT_ONLINE_INTERVAL_EAAF:
        case OLT_ONLINE_JIT_WBM_EAAF:
            calc_avg_arrival(onuNum,&(onuAttrs[onuNum].avgArrivalTime));
            break;
        }
    }
    else
    {
        onuAttrs[onuNum].minArrivalTime = simtime();
        onuAttrs[onuNum].avgArrivalTime = onuAttrs[onuNum].minArrivalTime;
    }
}



void olt(int onuNum, int lambdaNum)
{
	int transmitPkt, transmitVideoPkt;
	long result,txPktCount = 0, txVideoPktCount;
	double packet_transmission_time;
	double remainingGrantLength, remainingDataGrantLength, lengthCompare, grantCycle;
	sENTITY_GATE_PKT *pendingGATE;
	sENTITY_PKT currPkt;

	/* Initialize the process */
	procId[0] = '\0';
	sprintf(procId,"ONU #%d,L#%d",onuNum,lambdaNum);
	create(procId);
	
	// Test Variables
	status_processes_print();
	
	/* Upstream transmission gate functionality of ONU */
	while(!terminateSim) /* permanent behavior of the ONU process model */
	{
	
		/* Wait for a TX GATE from the OLT */
		receive(onuAttrs[onuNum].grantMailbox[lambdaNum], (long *) &pendingGATE);

		/* Check that this wavelength is supported */
		if(onuAttrs[onuNum].supportedLambdasMap[lambdaNum] == LAMBDA_FALSE)
		{
			printf("\n\n\n[%10.5e] Wavelength #%d not supported on ONU #%d!!!!!\n\n\n",simtime(),lambdaNum,onuNum);
			/* Fill out some context information */
			dump_msg_buf[0] = '\0';
			sprintf(dump_msg_buf,"Detected by ONU #%d\n",onuNum);
			sprintf(dump_msg_buf,"%sReceived GATE message:\n",dump_msg_buf);
			sprintf(dump_msg_buf,"%slambda=%d,start=%e,length=%e\n",dump_msg_buf,pendingGATE->lambda,pendingGATE->start,pendingGATE->length);
			sprintf(dump_msg_buf,"%swavelength %d not supported\n",dump_msg_buf,pendingGATE->lambda);
			fatalErrorCode = FATAL_CAUSE_INV_WA;
			dump_sim_core();
		}

		lengthCompare = pendingGATE->grant;
		
		/* Now wait for beginning of gate */
		if(pendingGATE->start > simtime())
		{
			hold(pendingGATE->start - simtime());
		}
		
		/*Calculate the maximum grant cycle for all the ONUs*/
		grantCycle = pendingGATE->start - onuAttrs[onuNum].previousGrantStartTime;
		onuAttrs[onuNum].previousGrantStartTime = pendingGATE->start;
		
		onuAttrs[onuNum].totalNumberGrantCycle += 1;
		onuAttrs[onuNum].averageGrantCycle = grantCycle/onuAttrs[onuNum].totalNumberGrantCycle + (onuAttrs[onuNum].totalNumberGrantCycle - 1)/onuAttrs[onuNum].totalNumberGrantCycle * onuAttrs[onuNum].averageGrantCycle;
		
	
		if(onuAttrs[onuNum].maximumGrantCycle < grantCycle)
		{
			onuAttrs[onuNum].maximumGrantCycle = grantCycle;
			onuAttrs[onuNum].maximumGrantCycleTime = simtime();
		}
		
		/* Record grant level statistics at beginning of grant */
		record_grant_stats_begin(onuNum, pendingGATE);		

		/* We are now gated for transmission, check length of transmission */
		remainingGrantLength = pendingGATE->grant;
		remainingVideoGrantLength = pendingGATE->video_grant;
		remainingDataGrantLength = pendingGATE->data_grant;
		transmitPkt = 1;
		transmitVideoPkt = 1;
		
			
		/* 
		 * Service the data queue
		 *######################################################################################################################
		 */
			 
		if (pendingGATE->data_grant > 0)
		{
			/* If there are no packets in data queue, then just transmit a report */
			if(onuAttrs[onuNum].packetsHead == NULL)
			{
				transmitPkt = 0;
				mpcp_report(onuNum);
			}
			else
			{
				/* If gate length is shorter than first packet size, then just prepare REPORT (rptQueueSize) */
				if(remainingDataGrantLength < (onuAttrs[onuNum].packetsHead->size + PREAMBLE_IPG_BYTES))
				{
					if((onuAttrs[onuNum].rptQueueSize != 0) && (simParams.DBA_TYPE == DBA_GATED))
					{
						printf("ERROR: Data GATE message did not accomodate all frames in REPORT Before Beginning Transmission\n");
						printf("D-REPORT = %.0f, D-GATE = %lf\n",onuAttrs[onuNum].rptQueueSize,(pendingGATE->data_grant));
						fflush(NULL);
						terminateSim = 1;
					}
					transmitPkt = 0;
					mpcp_report(onuNum);
				}
			}

			/* Check for excessive buffer size */
			if(onuAttrs[onuNum].packetQueueSize > MAX_PKT_BUF_SIZE)
			{
				fatalErrorCode = FATAL_CAUSE_BUFFER_OVR;
				dump_sim_core();
				transmitPkt = 0;
			}

			/* Reset transmitted packet counter */
			txPktCount = 0;
			while(transmitPkt)
			{
				/* transmit a packet */
				/* collect statistics on this packet */
				record_packet_stats_dequeue_tx_time(onuNum);
				/* Copy packet to temporary data structure */
				currPkt.creationTime = onuAttrs[onuNum].packetsHead->creationTime;
				currPkt.transmissionTime = onuAttrs[onuNum].packetsHead->transmissionTime;
				currPkt.arrivalTime = onuAttrs[onuNum].packetsHead->arrivalTime;
				currPkt.size = onuAttrs[onuNum].packetsHead->size;
				/* remove packet */
				remove_packet(onuNum);		
				
				packet_transmission_time = (currPkt.size)*simParams.TIME_PER_BYTE;
				
				if(simtime() <= 9172.806350 && simtime() >= 9172.805875)
				{
					printf("[%10.10e] ONU:Transmitting a data packet from ONU %d [%d bytes]\n",simtime(), onuNum, currPkt.size);
				}
			  	
				//########################################################################################################################

				result = timed_reserve(lambda[pendingGATE->lambda], 0.0);
				if(result != TIMED_OUT)
				{
					hold(packet_transmission_time);
					release(lambda[pendingGATE->lambda]);
				}
				else
				{
					printf("[%10.5e] FATAL ERROR: MAC failed, there was contention for lambda #%d [ONU #%d]\n", simtime(), 
					pendingGATE->lambda,onuNum);
					fatalErrorCode = FATAL_CAUSE_MAC_CONTENTION;
					/* Fill out some context information */
					dump_msg_buf[0] = '\0';
					sprintf(dump_msg_buf,"Detected by ONU #%d\n",onuNum);
					sprintf(dump_msg_buf,"%sReceived Data GATE message:\n",dump_msg_buf);
					sprintf(dump_msg_buf,"%slambda=%d,start=%e,length=%e\n",dump_msg_buf,pendingGATE->lambda,pendingGATE->start,pendingGATE->length);
					dump_sim_core();
					transmitPkt = 0;
				}

				/* Increment transmitted packet counter */
				txPktCount++;

				/* Incremement throughput statistics per ONU*/
				onuAttrs[onuNum].transmitByteCnt += currPkt.size;

				/* Remove this transmitted frame from previous report queue size */
				onuAttrs[onuNum].rptQueueSize -= (currPkt.size + PREAMBLE_IPG_BYTES);
				
				/* Collect statistics on this packet */
				record_packet_stats_finish(onuNum, &currPkt);
				remainingGrantLength -= currPkt.size + PREAMBLE_IPG_BYTES;
				remainingDataGrantLength -= currPkt.size + PREAMBLE_IPG_BYTES;
				/* Expire the inter-packet time (Preamble + IPG, 20 bytes total) */
				hold(simParams.PREAMBLE_IPG_TIME);
				/* Check if next packet can be transmitted in this gate */
				if(onuAttrs[onuNum].packetsHead == NULL)
				{
					transmitPkt = 0;
					mpcp_report(onuNum);
				}
				else
				{
					if(remainingDataGrantLength < (onuAttrs[onuNum].packetsHead->size + PREAMBLE_IPG_BYTES))
					{
						if((onuAttrs[onuNum].rptQueueSize != 0) && (simParams.DBA_TYPE == DBA_GATED))
						{
							printf("ERROR: Data GATE message did not accomodate all Data frames in REPORT\n");
							printf("Remaining D REPORT = %.0f, Remaining D GATE = %lf, ONU#:%d\n",onuAttrs[onuNum].rptQueueSize,remainingDataGrantLength, onuNum);
							fflush(NULL);
							terminateSim = 1;
						}
						transmitPkt = 0;
						mpcp_report(onuNum);
					}
					else
					{
						transmitPkt = 1;
					}
				}
						
			}

		}
		else
		{
			mpcp_report(onuNum);
		}


    /* Note REPORT time */
    onuAttrs[onuNum].rptTime = simtime() + (64*simParams.TIME_PER_BYTE);

    /* Record number of packets transmitted */
    record(txPktCount,overallGrantSizePkt);

		/* Free GATE packet */
		remove_gate(pendingGATE);

	}

}




