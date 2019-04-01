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


/* Report the Video queue size*/
void mpcp_video_report(int onuNum)
{
	// These functions check the ONU for excessively long queues
	check_video_packet_list(onuNum);
	check_data_packet_list(onuNum);
	
	// Drop excess enhancement layers
	drop_scalable_video_packets(onuNum, 0, 0, 0, 1);
	drop_scalable_video_packets(onuNum, 0, LowerBound_scalableDropping, UpperBound_scalableDropping, 3);
	drop_scalable_video_packets(onuNum, 0, LowerBound_scalableDropping, UpperBound_scalableDropping, 4);
	drop_scalable_video_packets(onuNum, 0, LowerBound_scalableDropping, UpperBound_scalableDropping, 5);


    onuAttrs[onuNum].rptVideoQueueSize = onuAttrs[onuNum].packetVideoQueueSize + onuAttrs[onuNum].packetVideoQueueNum * PREAMBLE_IPG_BYTES;
    onuAttrs[onuNum].rptVideoQueueNum = onuAttrs[onuNum].packetVideoQueueNum;
    
    onuAttrs[onuNum].rptForecastVideoQueueSize = onuAttrs[onuNum].packetForecastVideoQueueSize + onuAttrs[onuNum].packetForecastVideoQueueNum * PREAMBLE_IPG_BYTES;
    onuAttrs[onuNum].rptForecastVideoQueueNum = onuAttrs[onuNum].packetForecastVideoQueueNum;
    onuAttrs[onuNum].rptForecastVideoTime = onuAttrs[onuNum].packetForecastVideoTime;

/*##########################################################################################################################################################
   
    if (simtime() >= 0.036 && simtime() <= 0.040222) printf("[%10.5e], ONU[%d] Video Queue = %ld bytes,   Reported Video Queue = %ld bytes\n",simtime(), onuNum, onuAttrs[onuNum].packetVideoQueueSize, onuAttrs[onuNum].rptVideoQueueSize);
    
##########################################################################################################################################################*/

    if(onuAttrs[onuNum].packetsVideoHead != NULL) 
    {
        onuAttrs[onuNum].minArrivalTimeVideo = onuAttrs[onuNum].packetsVideoHead->creationTime;
    }
    else
    {
        onuAttrs[onuNum].minArrivalTimeVideo = simtime();
        onuAttrs[onuNum].avgArrivalTimeVideo = onuAttrs[onuNum].minArrivalTimeVideo;
    }
}


/* 
 * FUNCTION: onu()
 * DESCRIPTION: Process model of an ONU
 *
 * NOTES:
 * Recently updated to remove the REPORT message transmission code, we now have the OLT scheduler
 * expire the time for the REPORT message. This way the utilization statistics for the wavelength
 * facilities only show usage to transmit data packets.
 *
 */
void onu(int onuNum, int lambdaNum)
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
	
#ifdef DEBUG_TRC_HI
	printf("ONU #%d, Lambda #%d\n",onuNum,lambdaNum);
#endif

	//~ TSprint("ONU #%d, Lambda #%d\n",onuNum,lambdaNum);
	
	/* Upstream transmission gate functionality of ONU */
	while(!terminateSim) /* permanent behavior of the ONU process model */
	{
	
		//~ TSprint("[%10.5e] ONU:Waiting for GATE on ONU %d,L#%d\n",simtime(),onuNum,lambdaNum);

		//~ if (onuNum == 1 && simtime() >= 7200)	TSPrint("[%10.5e] ONU:Waiting for GATE on ONU %d,L#%d\n",simtime(),onuNum,lambdaNum);

		/* Wait for a TX GATE from the OLT */
		
		receive(onuAttrs[onuNum].grantMailbox[lambdaNum], (long *) &pendingGATE);
		
		//~ TSprint("[%10.5e] ONU:Received GATE on ONU %d,L#%d [current queue size = %ld]\n",simtime(),onuNum, lambdaNum,onuAttrs[onuNum].packetQueueSize);
		//~ TSprint("  MPCP GATE:: start = %f, length = %f, lambda = %d\n", pendingGATE->start, pendingGATE->length, pendingGATE->lambda);
		
		//~ TSprint("[%10.5e] ONU:Received GATE on ONU %d,L#%d [current queue size = %ld]\n",simtime(),onuNum, lambdaNum,onuAttrs[onuNum].packetQueueSize);

		//~ if (onuNum == 1 && simtime() >= 7200)	TSprint("[%10.5e] ONU:Received GATE on ONU %d,L#%d [current data queue size = %ld] [current video queue size = %.0f]\n",simtime(),onuNum,
					//~ lambdaNum,onuAttrs[onuNum].packetQueueSize, onuAttrs[onuNum].packetVideoQueueSize);
		//~ if (onuNum == 1 && simtime() >= 7200)	TSprint("  MPCP GATE:: start = %f, length = %lf, D-length = %lf, V-length = %lf, lambda = %d\n", pendingGATE->start, pendingGATE->grant, pendingGATE->data_grant, pendingGATE->video_grant, pendingGATE->lambda);
		//~ if (onuNum == 1 && simtime() >= 7200)	TSprint(" V-Report = %.0f, D-Report = %ld\n", onuAttrs[onuNum].rptVideoQueueSize,onuAttrs[onuNum].rptQueueSize);
		

		/* Check that this wavelength is supported */
		if(onuAttrs[onuNum].supportedLambdasMap[lambdaNum] == LAMBDA_FALSE)
		{
			printf("\n\n\n[%10.5e] Wavelength #%d not supported on ONU #%d!!!!!\n\n\n",simtime(),lambdaNum,onuNum);
			TSprint("\n\n\n[%10.5e] Wavelength #%d not supported on ONU #%d!!!!!\n\n\n",simtime(),lambdaNum,onuNum);
			/* Fill out some context information */
			dump_msg_buf[0] = '\0';
			sprintf(dump_msg_buf,"Detected by ONU #%d\n",onuNum);
			sprintf(dump_msg_buf,"%sReceived GATE message:\n",dump_msg_buf);
			sprintf(dump_msg_buf,"%slambda=%d,start=%e,length=%e\n",dump_msg_buf,pendingGATE->lambda,pendingGATE->start,pendingGATE->length);
			sprintf(dump_msg_buf,"%swavelength %d not supported\n",dump_msg_buf,pendingGATE->lambda);
			fatalErrorCode = FATAL_CAUSE_INV_WA;
			dump_sim_core();
		}

		/* Check the size of the GATE */
//		if(pendingGATE->length < onuAttrs[onuNum].rptQueueSize*simParams.TIME_PER_BYTE)
//		{
//			/* Add an insignificant amount of time, this is to deal with floating point number issues */
//			pendingGATE->length += 1e-15;
//		}

		/* Add an insignificant amount of time, this is to deal with floating point number issues */
//		pendingGATE->length += 5e-15;
//		pendingGATE->video_length += 4e-15;
//		pendingGATE->data_length += 1e-15;
		
		if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
		{
			lengthCompare = pendingGATE->data_grant;
		}
		else
		{
			lengthCompare = pendingGATE->grant;
		}
		
		if((lengthCompare < onuAttrs[onuNum].rptQueueSize) && (simParams.DBA_TYPE == DBA_GATED))
		{
			/* If still too small, this is an error, record it */
			printf("\n\n\n[%10.5e] GATE too small!!!, Data_REPORT=%.0f bytes, Data_Grant=%lf bytes, byte_diff=%e, ONU# %d\n\n\n",simtime(),onuAttrs[onuNum].rptQueueSize, lengthCompare, onuAttrs[onuNum].rptQueueSize-lengthCompare, onuNum);
			TSprint("\n\n\n[%10.5e] GATE too small!!!, Data_REPORT=%.0f bytes, Data_Grant=%lf bytes, byte_diff=%e, ONU# %d\n\n\n",simtime(),onuAttrs[onuNum].rptQueueSize, lengthCompare, onuAttrs[onuNum].rptQueueSize-lengthCompare, onuNum);
			/* Fill out some context information */
			dump_msg_buf[0] = '\0';
			sprintf(dump_msg_buf,"Detected by ONU #%d\n",onuNum);
			sprintf(dump_msg_buf,"%sReceived GATE message:\n",dump_msg_buf);
			sprintf(dump_msg_buf,"%slambda=%d,start=%e,data_length=%lf\n",dump_msg_buf,pendingGATE->lambda,pendingGATE->start,lengthCompare);
			sprintf(dump_msg_buf,"%sGATE too small, REPORT=%e\n",dump_msg_buf,((double)(onuAttrs[onuNum].rptQueueSize)*simParams.TIME_PER_BYTE));
			fatalErrorCode = FATAL_CAUSE_GATE_TOO_SMALL;
			dump_sim_core();
		}
		
		/* Now wait for beginning of gate */
		if(pendingGATE->start > simtime())
		{
			//~ TSprint("[%10.5e] ONU:Waiting for start of GATE on ONU %d\n",simtime(),onuNum);
			hold(pendingGATE->start - simtime());
			//~ TSprint("[%10.5e] ONU:Start of GATE reached on ONU %d\n",simtime(),onuNum);
		}
		
		/* Calculate the maximum grant cycle for all the ONUs */
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
		 * Service the video queue 
		 *########################################################################################################################
		 */
		 
		//~ if (onuNum == 1 && /*(remainingVideoGrantLength != 0 || remainingDataGrantLength != 0) &&*/ simtime() >= 7200 )
		//~ {
			//~ TSprint("V-RPT = %.0f, D-RPT = %ld, V-GATE = %lf, D-GATE = %lf, GATE = %lf, onuNum = %d\n",onuAttrs[onuNum].rptVideoQueueSize, onuAttrs[onuNum].rptQueueSize, pendingGATE->video_grant, pendingGATE->data_grant, pendingGATE->grant,onuNum);		
		//~ }	
		
		if (pendingGATE->video_grant > 0)
		{
			/* If there are no packets in Video queue, then just transmit a report */
			if(onuAttrs[onuNum].packetsVideoHead == NULL)
			{
				transmitVideoPkt = 0;
				mpcp_video_report(onuNum);
				record_video_report(onuNum);
				
				// Debugging the Cycle length and the queueing delay mismatch
				///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		
				// printf("ERROR:[%10.5e] Video Grant isn't zero while the video message is\n",simtime());

				//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////	
				
				if (pendingGATE->data_grant == 0)
				{
					mpcp_report(onuNum);
				}
				record_unused_video_grant(onuNum);
			}
			else
			{
				/* If gate length is shorter than first packet size, then just prepare REPORT (rptVideoQueueSize) */
				if(remainingVideoGrantLength < (onuAttrs[onuNum].packetsVideoHead->size + PREAMBLE_IPG_BYTES))
				{
					if((remainingVideoGrantLength < 0) && (simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED))
					{
						printf("ERROR: Video GATE message did is Negative\n");
						printf("REPORT = %.0f, GATE = %lf\n",onuAttrs[onuNum].rptVideoQueueSize,(pendingGATE->video_grant));
						fflush(NULL);
						terminateSim = 1;
						set(SIM_END_EVENT);
					}
					
					record_unused_video_grant(onuNum);
					
					//if (onuNum == 3)
					//{
					//	printf("remainingVideoGrantLength = %ld\n", (unsigned long)(remainingVideoGrantLength*(1/simParams.TIME_PER_BYTE)));
					//}
					
					transmitVideoPkt = 0;
					mpcp_video_report(onuNum);
					record_video_report(onuNum);
					
					if (pendingGATE->data_grant == 0)
					{
						mpcp_report(onuNum);
					}
				}
			}
			
			
			/* Check for excessive buffer size */
			if(onuAttrs[onuNum].packetVideoQueueSize > MAX_PKT_BUF_SIZE)
			{
				fatalErrorCode = FATAL_CAUSE_BUFFER_OVR;
				dump_sim_core();
				transmitVideoPkt = 0;
			}
			
			/* Reset transmitted packet counter */
			txVideoPktCount = 0;
			while(transmitVideoPkt)
			{
				/* transmit a packet */
				/* collect statistics on this packet */
				record_video_packet_stats_dequeue_tx_time(onuNum);
			
				/* Copy packet to temporary data structure */
				currPkt.creationTime = onuAttrs[onuNum].packetsVideoHead->creationTime;
				currPkt.transmissionTime = onuAttrs[onuNum].packetsVideoHead->transmissionTime;
				currPkt.arrivalTime = onuAttrs[onuNum].packetsVideoHead->arrivalTime;
				currPkt.size = onuAttrs[onuNum].packetsVideoHead->size;
				
				/* remove packet */
				remove_video_packet(onuNum);
				packet_transmission_time = (currPkt.size)*simParams.TIME_PER_BYTE;

#ifdef DEBUG_ONU			
if (onuNum == 1 && simtime() >= 7200)	printf("[%10.5e] ONU:Transmitting a video packet from ONU %d [%d bytes]\n",simtime(), onuNum, currPkt.size);
#endif
				
#ifdef DEBUG_TRC
printf("[%10.5e] ONU:Transmitting a video packet from ONU %d [%d bytes]\n",simtime(), onuNum, currPkt.size);
#endif

				//#######################################################################################################################
				
				if(simtime() <= 9172.806350 && simtime() >= 9172.805875)
				{
					printf("[%10.10e] ONU:Transmitting a video packet from ONU %d [%d bytes]\n",simtime(), onuNum, currPkt.size);
				}
				
				//########################################################################################################################

				result = timed_reserve(lambda[pendingGATE->lambda], 0.0);
				if(result != TIMED_OUT)
				{
					hold(packet_transmission_time);

#ifdef DEBUG_ONU
if (onuNum == 1 && simtime() >= 7200)	printf("[%10.5e] ONU:Releasing lambda #%d on ONU %d [tx packet]\n",simtime(),pendingGATE->lambda,onuNum);
#endif
					
#ifdef DEBUG_TRC
printf("[%10.5e] ONU:Releasing lambda #%d on ONU %d [tx packet]\n",simtime(),pendingGATE->lambda,onuNum);
#endif
					release(lambda[pendingGATE->lambda]);
				}
				else
				{
					printf("[%10.5e] FATAL ERROR: MAC failed, there was a video contention for lambda #%d [ONU #%d]\n", simtime(), pendingGATE->lambda,onuNum);
					fatalErrorCode = FATAL_CAUSE_MAC_CONTENTION;
					/* Fill out some context information */
					dump_msg_buf[0] = '\0';
					sprintf(dump_msg_buf,"Detected by ONU #%d\n",onuNum);
					sprintf(dump_msg_buf,"%sReceived Video GATE message:\n",dump_msg_buf);
					sprintf(dump_msg_buf,"%slambda=%d,start=%e,length=%e\n",dump_msg_buf,pendingGATE->lambda,pendingGATE->start,pendingGATE->length);
					dump_sim_core();
					transmitVideoPkt = 0;
					transmitPkt = 0;
				}
				
				/* Increment transmitted packet counter */
				txVideoPktCount++;
				
				/* Incremement throughput statistics per ONU*/
				onuAttrs[onuNum].transmitVideoByteCnt += currPkt.size;

				/* Remove this transmitted frame from previous report queue size */
	//			onuAttrs[onuNum].rptVideoQueueSize -= (currPkt.size + PREAMBLE_IPG_BYTES);

				/* collect statistics on this packet */
				record_video_packet_stats_finish(onuNum, &currPkt);
				remainingGrantLength -= currPkt.size + PREAMBLE_IPG_BYTES;
				remainingVideoGrantLength -= currPkt.size + PREAMBLE_IPG_BYTES;
				/* Expire the inter-packet time (Preamble + IPG, 20 bytes total) */
				hold(simParams.PREAMBLE_IPG_TIME);

#ifdef DEBUG_ONU			
if (onuNum == 1 && (pendingGATE->grant) != 0 && simtime() >= 7200)
{
	printf("Remaining V-Gate = %lf, Remaining D-Gate = %lf, Remaining GATE = %lf, onuNum = %d\n",(remainingVideoGrantLength),(remainingDataGrantLength),(remainingGrantLength),onuNum);
}
#endif
				
					/* check if next packet can be transmitted in this gate */
				if(onuAttrs[onuNum].packetsVideoHead == NULL)
				{
					transmitVideoPkt = 0;
					mpcp_video_report(onuNum);
					record_video_report(onuNum);
					
					if (pendingGATE->data_grant == 0)
					{
						mpcp_report(onuNum);
					}
					record_unused_video_grant(onuNum);
				}
				else
				{
					if(remainingVideoGrantLength < ((onuAttrs[onuNum].packetsVideoHead->size  + PREAMBLE_IPG_BYTES)))
					{
						if(((remainingVideoGrantLength) < 0) && (simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED))
						{
							printf("ERROR: Video GATE message is Negative\n");
							printf("Remaining V GATE = %lf, onuNum = %d\n",(remainingVideoGrantLength),onuNum);
							fflush(NULL);
							terminateSim = 1;
							set(SIM_END_EVENT);
						}

						record_unused_video_grant(onuNum);
						
						transmitVideoPkt = 0;
						mpcp_video_report(onuNum);
						record_video_report(onuNum);
						
						if (pendingGATE->data_grant == 0)
						{
							mpcp_report(onuNum);
						}
					}
					else
					{
						transmitVideoPkt = 1;
					}
				}
						
			}

		}
		else
		{
			mpcp_video_report(onuNum);
			record_video_report(onuNum);
		}
			
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
				mpcp_video_report(onuNum);
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
					mpcp_video_report(onuNum);
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
				
#ifdef DEBUG_ONU			
if (onuNum == 1 && simtime() >= 7200)	printf("[%10.5e] ONU:Transmitting a Data packet from ONU %d [%d bytes]\n",simtime(), onuNum, currPkt.size);
#endif				
				
#ifdef DEBUG_TRC
printf("[%10.5e] ONU:Transmitting a packet from ONU %d [%d bytes]\n",simtime(), onuNum, currPkt.size);
#endif

				//#######################################################################################################################
				
				if(simtime() <= 9172.806350 && simtime() >= 9172.805875)
				{
					printf("[%10.10e] ONU:Transmitting a data packet from ONU %d [%d bytes]\n",simtime(), onuNum, currPkt.size);
				}
				
				//########################################################################################################################

				result = timed_reserve(lambda[pendingGATE->lambda], 0.0);
				if(result != TIMED_OUT)
				{
					hold(packet_transmission_time);
					
#ifdef DEBUG_ONU
if (onuNum == 1 && simtime() >= 7200)	printf("[%10.5e] ONU:Releasing lambda #%d on ONU %d [tx packet]\n",simtime(),pendingGATE->lambda,onuNum);
#endif
					
#ifdef DEBUG_TRC
printf("[%10.5e] ONU:Releasing lambda #%d on ONU %d [tx packet]\n",simtime(),pendingGATE->lambda,onuNum);
#endif
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
				
				/* collect statistics on this packet */
				record_packet_stats_finish(onuNum, &currPkt);
				remainingGrantLength -= currPkt.size + PREAMBLE_IPG_BYTES;
				remainingDataGrantLength -= currPkt.size + PREAMBLE_IPG_BYTES;
				/* Expire the inter-packet time (Preamble + IPG, 20 bytes total) */
				hold(simParams.PREAMBLE_IPG_TIME);
				/* check if next packet can be transmitted in this gate */
				if(onuAttrs[onuNum].packetsHead == NULL)
				{
					transmitPkt = 0;
					mpcp_report(onuNum);
					mpcp_video_report(onuNum);
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
						mpcp_video_report(onuNum);
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

        /* Record number of video packets transmitted */
//fix	record(txVideoPktCount,overallGrantSizePkt);

        /* Record number of packets transmitted */
        record(txPktCount,overallGrantSizePkt);

		/* Free GATE packet */
		remove_gate(pendingGATE);

	}

}
