/*
	TITLE: 		EPON Simulator Traffic Models
	AUTHOR: 	Michael P. McGarry
	DATE:
	VERSION:	1.0
	
	NOTES:
	
*/

#include <stdlib.h>
#include <math.h>
#include <csim.h>
#include "eponsim.h"
#include "eponsim_util.h"
#include "eponsim_traffic.h"
#include <string.h>
#include <stdio.h>

extern EVENT SIM_END_EVENT;

/* Data structures for empirical distribution of packet sizes */
double	EMPIRICAL_PROB[5]  = { 0.60, 0.04, 0.11, 0.25 };
double	EMPIRICAL_VALUE[5] = { 64.0, 300.0, 580.0, 1518.0 };
double	EMPIRICAL_CUTOFF[5];
long	EMPIRICAL_ALIAS[5];
char	type[1] = "I";

/* Pareto distribution random number generator */
double stream_pareto_epon(STREAM randomStream, double shapeParam, double locationParam)
{
    return (locationParam/(pow(stream_uniform(randomStream,0,1),(1/shapeParam))));
}

/* Packet assignment function */
void assign_packet(sENTITY_PKT *pkt, int onuNum)
{
    if(pkt != NULL)
    {
        pkt->creationTime = simtime();
        if(onuAttrs[onuNum].packetsHead == NULL)
        {
            /* this is the only packet */
            onuAttrs[onuNum].packetsHead = pkt;
            onuAttrs[onuNum].packetsTail = pkt;
        }
        else
        {
            /* Go to the end of the packet list for this ONU */
            onuAttrs[onuNum].packetsTail->next = pkt;
            pkt->next = NULL;
            onuAttrs[onuNum].packetsTail = pkt;
        }
        /* Add this packet's size to the queue size */
        onuAttrs[onuNum].packetQueueSize += pkt->size;
        /* Add this packet to the queue packet count */
        onuAttrs[onuNum].packetQueueNum++;
    }
}

/* Video packet assignment function */
void assign_video_packet(sENTITY_PKT *pkt, int onuNum)
{
    if(pkt != NULL)
    {
        pkt->creationTime = simtime();
        if(onuAttrs[onuNum].packetsVideoHead == NULL)
        {
            /* this is the only packet */
            onuAttrs[onuNum].packetsVideoHead = pkt;
            onuAttrs[onuNum].packetsVideoTail = pkt;
        }
        else
        {
            /* Go to the end of the packet list for this ONU */
            onuAttrs[onuNum].packetsVideoTail->next = pkt;
            pkt->next = NULL;
            onuAttrs[onuNum].packetsVideoTail = pkt;
        }
        /* Add this packet's size to the queue size */

//####################################################################################################################   
     
//	if (type[3] == pkt->frameType && onuNum == 3) onuAttrs[onuNum].packetVideoQueueSize += pkt->size - 58;
//	else if(type[3] != pkt->frameType && onuNum == 3)  
//	{
//		onuAttrs[onuNum].packetVideoQueueSize = pkt->size - 58;
//		type[3] = pkt->frameType;
//	}
//	else onuAttrs[onuNum].packetVideoQueueSize += pkt->size;
//
//if (onuNum == 3) printf("[%10.5e], ONU[%d] Video Queue = %ld bytes,   Frame Type = %c\n",simtime(), onuNum, onuAttrs[onuNum].packetVideoQueueSize, pkt->frameType);        
        
//####################################################################################################################        
        	
	onuAttrs[onuNum].packetVideoQueueSize += pkt->size;
	
	onuAttrs[onuNum].packetForecastVideoQueueSize = pkt->forecastSize;
	
	onuAttrs[onuNum].packetForecastVideoQueueNum = pkt->forecastPktNumber;
	
    	onuAttrs[onuNum].packetForecastVideoTime = (pkt->frameTimeStamp/1000) + 1/simParams.FRAME_RATE;
         
        /* Add this packet to the queue packet count */
        onuAttrs[onuNum].packetVideoQueueNum++;
        
/*##########################################################################################################################################################

	if (simtime() >= 0.03998 && simtime() <= 0.040222) printf("[%10.5e], ONU[%d] Video Queue = %ld bytes,   Frame Type = %c\n",simtime(), onuNum, onuAttrs[onuNum].packetVideoQueueSize, pkt->frameType); 

##########################################################################################################################################################*/

    }
}

/*
 * Traffic models
 */

/* Process model of a Poisson traffic generator */
void traffic_src_poisson(int onuNum)
{
	int pktSize;
	sENTITY_PKT *pktPtr;
	
	/* Initialize the process */
	procId[0] = '\0';
	sprintf(procId,"TrafficGen #%d",onuNum);
	create(procId);
#ifdef DEBUG_TRC_HI
	printf("Traffic #%d\n",onuNum);
#endif

	while(!terminateSim)
	{
		/* Generate packets according to a particular distribution */
		if(onuNum < simParams.NUM_HEAVY_ONU)
		{
			hold(stream_exponential(onuAttrs[onuNum].pktInterArrivalStream, simParams.AVG_PKT_INTER_ARVL_TIME_HEAVY));
		}
		else
		{
			hold(stream_exponential(onuAttrs[onuNum].pktInterArrivalStream, simParams.AVG_PKT_INTER_ARVL_TIME));
		}
			pktSize = (int) stream_empirical(onuAttrs[onuNum].pktSizeStream, EMPIRICAL_SIZE, EMPIRICAL_CUTOFF, EMPIRICAL_ALIAS, EMPIRICAL_VALUE);
			pktPtr = create_a_packet(pktSize, onuNum);
        	assign_packet(pktPtr, onuNum);
#ifdef DEBUG_TRC
        printf("[%10.5e] ---> Generating Packet with %d bytes for ONU #%d\n",simtime(),pktSize,onuNum);
#endif
	}
	
}

/* 
 * FUNCTION: traffic_src_self_similar()
 * DESCRIPTION: Process model of a Self-similar traffic generator
 *
 * Individual self-similar packet stream generator
 *
 * Pareto distributed burst size and Pareto distributed OFF time
 *
 */
void traffic_src_self_similar(int onuNum, int streamNum)
{
	int pktSize;
	sENTITY_PKT *pktPtr;
	double currPktTime, offTime;
	int burstSize, loop;
    //double lastPktTxTime;
	
	/* Initialize the process */
	procId[0] = '\0';
	sprintf(procId,"TrafficGen %d:%d",onuNum, streamNum);
	create(procId);	
#ifdef DEBUG_TRC_HI
	printf("Traffic #%d for ONU #%d\n",streamNum, onuNum);
#endif

	/* Sleep until first ON period */
	if(onuNum < simParams.NUM_HEAVY_ONU)
	{
        /* Obtain a Pareto distributed random number (using Uniform distribution transformation) */
		offTime = stream_pareto_epon(onuAttrs[onuNum].pktInterArrivalStream,simParams.SS_PARETO_SHAPE_PARAM,simParams.SS_OFF_LOC_PARAM_HEAVY);
	}
	else
	{
        /* Obtain a Pareto distributed random number (using Uniform distribution transformation) */
		offTime = stream_pareto_epon(onuAttrs[onuNum].pktInterArrivalStream,simParams.SS_PARETO_SHAPE_PARAM,simParams.SS_OFF_LOC_PARAM);
	}
	hold(offTime);

	while(!terminateSim)
	{
        /* Pareto distributed burst size */
		burstSize = (int) rintf(stream_pareto_epon(onuAttrs[onuNum].pktInterArrivalStream,simParams.SS_PARETO_SHAPE_PARAM,simParams.SS_PARETO_LOC_PARAM));
		/* Transmit Burst */
		for(loop=0; loop < burstSize; loop++)
		{
			pktSize = (int) stream_empirical(onuAttrs[onuNum].pktSizeStream, EMPIRICAL_SIZE, EMPIRICAL_CUTOFF, EMPIRICAL_ALIAS, EMPIRICAL_VALUE);
			pktPtr = create_a_packet(pktSize, onuNum);
			/* assign_packet(pktPtr, onuNum); */
			/* Send packet to packet arrival mailbox */
			send(onuAttrs[onuNum].pktMailbox, (long) pktPtr);
			currPktTime = pktSize * simParams.TIME_PER_BYTE + simParams.PREAMBLE_IPG_TIME;
			hold(currPktTime);
		}
		/* Sleep until next ON period */
		if(onuNum < simParams.NUM_HEAVY_ONU)
		{
            /* Obtain a Pareto distributed random number (using Uniform distribution transformation) */
            offTime = stream_pareto_epon(onuAttrs[onuNum].pktInterArrivalStream,simParams.SS_PARETO_SHAPE_PARAM,simParams.SS_OFF_LOC_PARAM_HEAVY);
		}
		else
		{
            /* Obtain a Pareto distributed random number (using Uniform distribution transformation) */
            offTime = stream_pareto_epon(onuAttrs[onuNum].pktInterArrivalStream,simParams.SS_PARETO_SHAPE_PARAM,simParams.SS_OFF_LOC_PARAM);
		}
		/*
		 * Factor out Preamble and IPG time from OFF time period
		 * This way the load is computed as Ethernet frames only
		 */
		if(offTime >= (simParams.PREAMBLE_IPG_TIME*burstSize))
		{
			offTime -= (simParams.PREAMBLE_IPG_TIME*burstSize);
		}
		else
		{
			offTime = 0;
		}
		hold(offTime);
	}
	
}

/* 
 * FUNCTION: traffic_a#define LINE_LENGTH	128gg_self_similar()
 * DESCRIPTION: Process model of a Self-similar traffic aggregator
 *
 * Aggregates the packets generated by the individual traffic streams
 *
 */
void traffic_agg_self_similar(int onuNum)
{
	sENTITY_PKT *pktPtr;
	double currPktTime;

	/* Initialize the process */
	procId[0] = '\0';
	sprintf(procId,"TrafficAgg %d",onuNum);
	create(procId);	
#ifdef DEBUG_TRC_HI
	printf("Traffic Agg for ONU #%d\n",onuNum);
#endif

	while(!terminateSim)
	{
		/* Get packet from mailbox */
		receive(onuAttrs[onuNum].pktMailbox, (long *) &pktPtr);

		/* Place in Queue for ONU */
		assign_packet(pktPtr, onuNum);

        /* Check for excessive buffer size */
		if(onuAttrs[onuNum].packetQueueSize > MAX_PKT_BUF_SIZE)
		{
			fatalErrorCode = FATAL_CAUSE_BUFFER_OVR;
			dump_sim_core();
		}

#ifdef DEBUG_TRC
			printf("[%10.5e] ---> Generating Packet with %d bytes for ONU #%d\n",simtime(),pktPtr->size,onuNum);
#endif

		/* expire time to transmit packet */
		currPktTime = pktPtr->size * simParams.TIME_PER_BYTE + simParams.PREAMBLE_IPG_TIME;
		hold(currPktTime);
	}
}


/* Process model of a Video traffic generator */
void traffic_src_video(int onuNum, char fileName[])
{
	int pktSize, pktTemp, flag, wordCount = 0;
	//int icount = 0;
	char mystring[LINE_LENGTH];
	sENTITY_PKT *pktPtr;
	double frameTimeStamp;
	char frameType;
	//char dummy[LINE_LENGTH], c;
	int forecastSize, forecastPktNumber;
	
	/* Initialize the process */
	procId[0] = '\0';
	sprintf(procId,"TrafficGen #%d",onuNum);
	create(procId);
	
	// Test Variables traffic_src_video_start
	test_vars.traffic_src_video_start[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
	test_var_print();
	
#ifdef DEBUG_TRC_HI
	printf("Video Traffic #%d\n",onuNum);
#endif
	
	FILE* inFilePtrWrap = fopen(fileName,"r");
	FILE* inFilePtr = fopen(fileName,"r");
	if (inFilePtr == NULL)
	{
		printf("Error opening %s \n",fileName);
	}
	
	///////////////////////////////////////////////////////////////// Read Frame Size
	
	//while ((c = fgetc(inFilePtr)) != '\n')
	//{
		//if( c != '#' && c != ' ' && c != '\0') 
		//{
			//dummy[icount] = c;
			//icount = icount + 1;
		//}
	//}
	
	//onuAttrs[onuNum].maxFrameCount = atof(dummy);
	
	/////////////////////////////////////////////////////////////////	
	
	
	hold(simParams.TIME_SHIFT/1000.0 * onuNum);
	
	srand ( onuNum*10 );
	
	//Random Start for video traffic in each ONU
	
	onuAttrs[onuNum].startoffset = rand()/(RAND_MAX + 1.00);  //Disabled Temp. to test the Random Frame Start
	//onuAttrs[onuNum].startoffset = 0;
	
	printf("[%10.5e] ---> %lf sec is the start offset for ONU #%d\n",simtime(),onuAttrs[onuNum].startoffset,onuNum);
	hold(onuAttrs[onuNum].startoffset);
	printf("[%10.5e] Start Video stream for ONU #%d\n",simtime(),onuNum);
	
/////////////////////////////////////////////////////////////////////////////////////////////////////NEW Code
	
	//onuAttrs[onuNum].maxFrameCount = 256273;		//The max # of frames in the video trace file
	
	onuAttrs[onuNum].currentFrame = 1;
	
	onuAttrs[onuNum].sentFrameCount = 0;
	
	onuAttrs[onuNum].wrapAround = 0;
	
	onuAttrs[onuNum].startFrame = 1 + floor(onuAttrs[onuNum].maxFrameCount*rand()/(RAND_MAX + 1.00));
	// This was the problem code: (Why was this algorithm used?)
	// onuAttrs[onuNum].startFrame = 1 + ceil((50000.00-1.00)*rand()/(RAND_MAX + 1.00));  //Generate a random value between 1 and max # of Frames
	
////	printf("[%10.5e] ---> %lf sec is the start offset for ONU #%d\n",simtime(),onuAttrs[onuNum].startoffset, onuNum);
	printf("[%10.5e] ---> %lf is the starting frame for ONU #%d\n",simtime(),onuAttrs[onuNum].startFrame, onuNum);
		

///////////////////////////////////////////////////////////////////////////////////////////////////
	
	onuAttrs[onuNum].previousGrantStartTime = 0;
	onuAttrs[onuNum].maximumGrantCycle = 0;
	onuAttrs[onuNum].totalNumberGrantCycle = 0;
	onuAttrs[onuNum].averageGrantCycle = 0;
		
	flag = get_line(inFilePtr, mystring, LINE_LENGTH);
	
	while(!terminateSim && flag > 0)
	{
		if (flag == 500 || flag == 1000 || flag == 2000)
		{
			wordCount += 1;
				
			if (wordCount == 2) frameTimeStamp = atof(mystring) + simParams.TIME_SHIFT/1000.0 * onuNum + onuAttrs[onuNum].startoffset*1000;
			else if (wordCount == 3) frameType = mystring[0];
			else if (wordCount == 4) pktSize = atoi(mystring);
			else if (wordCount == 5) pktSize = pktSize + atoi(mystring);
			else if (wordCount == 6) 
			{
				pktTemp = atoi(mystring);
				
				if (flag == 1000)
				{
					wordCount = 0;
					
					if(onuAttrs[onuNum].currentFrame >= onuAttrs[onuNum].startFrame)
					{
						if(onuAttrs[onuNum].wrapAround == 0)
						{
							frameTimeStamp = frameTimeStamp-(onuAttrs[onuNum].startFrame-1)*1000/simParams.FRAME_RATE;
						}
						else
						{
							frameTimeStamp = frameTimeStamp+(onuAttrs[onuNum].sentFrameCount)*1000/simParams.FRAME_RATE;
						}
						
						if (pktTemp == 1)
						{
							// Test Variables traffic_src_video_process
							test_vars.traffic_src_video_process[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
							
							forecastSize = 0;
							forecastPktNumber = 0;
							pktPtr = create_a_video_packet(pktSize, frameType, frameTimeStamp, forecastSize, forecastPktNumber, 1, onuNum);
							assign_video_packet(pktPtr, onuNum);
							onuAttrs[onuNum].sentFrameCount = onuAttrs[onuNum].sentFrameCount + pktTemp;
							hold(1.0/simParams.FRAME_RATE);
						}
						else
						{
							// Test Variables traffic_src_video_process
							test_vars.traffic_src_video_process[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
							
							forecastSize = 0;
							forecastPktNumber = 0;
							pktPtr = create_a_video_packet(pktSize, frameType, frameTimeStamp, forecastSize, forecastPktNumber, 1, onuNum);
							assign_video_packet(pktPtr, onuNum);
						}
					}
				
					onuAttrs[onuNum].currentFrame = onuAttrs[onuNum].currentFrame + pktTemp;
				}
			}
			else if (wordCount == 7) forecastSize = pktTemp + atoi(mystring);
			else if (wordCount == 8) forecastPktNumber = atoi(mystring);
			else if (wordCount == 9)
			{
				pktTemp = atoi(mystring);

				wordCount = 0;

				if(onuAttrs[onuNum].currentFrame >= onuAttrs[onuNum].startFrame)
				{
					if(onuAttrs[onuNum].wrapAround == 0)
					{
						frameTimeStamp = frameTimeStamp-(onuAttrs[onuNum].startFrame-1)*1000/simParams.FRAME_RATE;
					}
					else
					{
						frameTimeStamp = frameTimeStamp+(onuAttrs[onuNum].sentFrameCount)*1000/simParams.FRAME_RATE;
					}

					if (pktTemp == 1)
					{
						// Test Variables traffic_src_video_process
						test_vars.traffic_src_video_process[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
							
						pktPtr = create_a_video_packet(pktSize, frameType, frameTimeStamp, forecastSize, forecastPktNumber, 1, onuNum);
						assign_video_packet(pktPtr, onuNum);
						onuAttrs[onuNum].sentFrameCount = onuAttrs[onuNum].sentFrameCount + pktTemp;
						hold(1.0/simParams.FRAME_RATE);
					}
					else
					{
						// Test Variables traffic_src_video_process
						test_vars.traffic_src_video_process[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
							
						pktPtr = create_a_video_packet(pktSize, frameType, frameTimeStamp, forecastSize, forecastPktNumber, 1, onuNum);
						assign_video_packet(pktPtr, onuNum);
					}
					
					//if(onuNum == 1) printf("[%10.5e] ---> PacketSize = %d,  FrameType = %c,  TimeStamp = %lf  for (ONU #%d)\n", simtime(), pktSize, frameType, frameTimeStamp, onuNum);
					
				}
				
				onuAttrs[onuNum].currentFrame = onuAttrs[onuNum].currentFrame + pktTemp;
			}
		}
		
		// The packet analysis code will go here.
		
			
		if (flag != 2000 && onuAttrs[onuNum].sentFrameCount != onuAttrs[onuNum].maxFrameCount) 
		{
			if(onuAttrs[onuNum].wrapAround == 0)
			{
				flag = get_line(inFilePtr, mystring, LINE_LENGTH);
			}
			else
			{
				flag = get_line(inFilePtrWrap, mystring, LINE_LENGTH);
			}
		}
			
		else 
		{
						
//			FILE* maxFile = fopen("sim_maxCycle","a");
//			fprintf(maxFile, "Time: %lf,  ONU# %d,  Maximum Cycle is %lf\n",onuAttrs[onuNum].maximumGrantCycleTime, onuNum, onuAttrs[onuNum].maximumGrantCycle);
//			fclose(maxFile);
			
//			FILE* avgFile = fopen("sim_avgCycle","a");
//			fprintf(maxFile, "ONU# %d,  Average Cycle Length is: %lf,   Total # of Cycles is: %lf\n", onuNum, onuAttrs[onuNum].averageGrantCycle,onuAttrs[onuNum].totalNumberGrantCycle);
//			fclose(avgFile);
			
			if(onuAttrs[onuNum].sentFrameCount == onuAttrs[onuNum].maxFrameCount)
			{
				flag = 0;
				printf("ONU# %d has read the Video Trace File Successfully\n",onuNum);
				// Test Variables traffic_src_video_finish
				test_vars.traffic_src_video_finish[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
				test_var_print();
				
				hold(1 - onuAttrs[onuNum].startoffset);
			
				set(SIM_END_EVENT);
			}
			else
			{
				wordCount = 0;
				flag = get_line(inFilePtrWrap, mystring, LINE_LENGTH);
				//printf("\nFlag = %d",flag);
			}
			
			
			if(onuAttrs[onuNum].wrapAround == 0)
			{
				fclose(inFilePtr);
				onuAttrs[onuNum].wrapAround = 1;
			}
			else
			{
				fclose(inFilePtrWrap);
			}
				
		}

                
#ifdef DEBUG_TRC
        printf("[%10.5e] ---> Generating Packet with %d bytes for ONU #%d\n", simtime(), pktSize, onuNum);
#endif
	}

}

//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////

/* Process model of a Scalable Video traffic generator */
void traffic_scalable_video(int onuNum, char fileName[])
{
	int pktSize, pktTemp, flag, wordCount = 0;
	//int icount = 0;
	char mystring[LINE_LENGTH];
	sENTITY_PKT *pktPtr;
	double frameTimeStamp;
	char 	frameType;
			//dummy[LINE_LENGTH], 
			//c;
	int	forecastSize, 
		forecastPktNumber;
	int	layer;
	
	/* Initialize the process */
	procId[0] = '\0';
	sprintf(procId,"TrafficGen #%d",onuNum);
	create(procId);
	
	// TSprint("The scalable video traffic generator was used.\n");
	
	// Test Variables traffic_scalable_video_start
	test_vars.traffic_scalable_video_start[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
	test_var_print();
	
#ifdef DEBUG_TRC_HI
	printf("Video Traffic #%d\n",onuNum);
#endif
	
	FILE* inFilePtrWrap = fopen(fileName,"r");
	FILE* inFilePtr = fopen(fileName,"r");
	if (inFilePtr == NULL)
	{
		printf("Error opening %s \n",fileName);
		TSprint("Error opening %s \n",fileName);
	}
	
	///////////////////////////////////////////////////////////////// Read Frame Size
	
	//while ((c = fgetc(inFilePtr)) != '\n')
	//{
		//if( c != '#' && c != ' ' && c != '\0') 
		//{
			//dummy[icount] = c;
			//icount = icount + 1;
		//}
	//}
	
	//onuAttrs[onuNum].maxFrameCount = atof(dummy);
	// TSprint("maxFrameCount = %f for ONU #%d\n", onuAttrs[onuNum].maxFrameCount, onuNum);
	
	/////////////////////////////////////////////////////////////////	
	
	
	hold(simParams.TIME_SHIFT/1000.0 * onuNum);
	
	srand ( onuNum*10 );
	
	//Random Start for video traffic in each ONU
	
	onuAttrs[onuNum].startoffset = rand()/(RAND_MAX + 1.00);  //Disabled Temp. to test the Random Frame Start
	//onuAttrs[onuNum].startoffset = 0;
	
	printf("[%10.5e] ---> %lf sec is the start offset for ONU #%d\n",simtime(),onuAttrs[onuNum].startoffset,onuNum);
	hold(onuAttrs[onuNum].startoffset);
	printf("[%10.5e] Start Video stream for ONU #%d\n",simtime(), onuNum);
	
/////////////////////////////////////////////////////////////////////////////////////////////////////NEW Code
	
	//onuAttrs[onuNum].maxFrameCount = 256273;		//The max # of frames in the video trace file
	
	onuAttrs[onuNum].currentFrame = 1;
	
	onuAttrs[onuNum].sentFrameCount = 0;
	
	onuAttrs[onuNum].wrapAround = 0;
	
	onuAttrs[onuNum].startFrame = 1 + floor(onuAttrs[onuNum].maxFrameCount*rand()/(RAND_MAX + 1.00));
	// onuAttrs[onuNum].startFrame = 1 + onuNum;
	// This was the problem code: (Why was this algorithm used?)
	// onuAttrs[onuNum].startFrame = 1 + ceil((50000.00-1.00)*rand()/(RAND_MAX + 1.00));  //Generate a random value between 1 and max # of Frames
	
////	printf("[%10.5e] ---> %lf sec is the start offset for ONU #%d\n",simtime(),onuAttrs[onuNum].startoffset, onuNum);
	printf("[%10.5e] ---> %lf is the starting frame for ONU #%d\n", simtime(), onuAttrs[onuNum].startFrame, onuNum);
	// ONUprint(onuNum, "[%10.5e] ---> %lf is the starting frame\n", simtime(), onuAttrs[onuNum].startFrame);
	// ONUprint(onuNum, "\t-------> %d is the number of enhancement layers\n", simParams.SCALABLE_VIDEO_NUM_LAYERS - 1);
	// TSprint("%lf is the starting frame for ONU #%d\n", onuAttrs[onuNum].startFrame, onuNum);
		

///////////////////////////////////////////////////////////////////////////////////////////////////
	
	onuAttrs[onuNum].previousGrantStartTime = 0;
	onuAttrs[onuNum].maximumGrantCycle = 0;
	onuAttrs[onuNum].totalNumberGrantCycle = 0;
	onuAttrs[onuNum].averageGrantCycle = 0;
		
	flag = get_line(inFilePtr, mystring, LINE_LENGTH);
	
	while(!terminateSim && flag > 0)
	{
		if (flag == 500 || flag == 1000 || flag == 2000)
		{
			wordCount += 1;
				
			if (wordCount == 2) {
				frameTimeStamp = atof(mystring) + simParams.TIME_SHIFT/1000.0 * onuNum + onuAttrs[onuNum].startoffset*1000;
				// ONUprint(onuNum, "%.0f\t", atof(mystring));
			}
			else if (wordCount == 3) {
				frameType = mystring[0];
				// ONUprint(onuNum, "%c\t", frameType);
			}
			else if (wordCount == 4) {
				pktSize = atoi(mystring);
				// ONUprint(onuNum, "%d\t", pktSize);
			}
			else if (wordCount == 5) {
				pktSize = pktSize + atoi(mystring);
				// ONUprint(onuNum, "%d\t", atoi(mystring));
			}
			else if (wordCount == 6) {
				pktTemp = atoi(mystring);
				// ONUprint(onuNum, "%d\t", pktTemp);
			}
			else if (wordCount == 7) 
			{
				
				layer = atoi(mystring); /*David (I rearanged a few things in this section.)*/
				// ONUprint(onuNum, "%d\t\t\t", layer);
				
				
				
				
				if (flag == 1000)
				{
					wordCount = 0;
					
					if(onuAttrs[onuNum].currentFrame >= onuAttrs[onuNum].startFrame)
					{
						if(onuAttrs[onuNum].wrapAround == 0)
						{
							frameTimeStamp = frameTimeStamp-(onuAttrs[onuNum].startFrame-1)*1000/simParams.FRAME_RATE;
						}
						else
						{
							frameTimeStamp = frameTimeStamp+(onuAttrs[onuNum].sentFrameCount)*1000/simParams.FRAME_RATE;
						}
						
						if (pktTemp == 1 && layer == (simParams.SCALABLE_VIDEO_NUM_LAYERS - 1))
						{
							// Test Variables traffic_scalable_video_process
							test_vars.traffic_scalable_video_process[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
							
							forecastSize = 0;
							forecastPktNumber = 0;
							pktPtr = create_a_video_packet(pktSize, frameType, frameTimeStamp, forecastSize, forecastPktNumber, layer + 1, onuNum);
							assign_video_packet(pktPtr, onuNum);
							onuAttrs[onuNum].sentFrameCount = onuAttrs[onuNum].sentFrameCount + pktTemp;
							hold(1.0/simParams.FRAME_RATE);
						}
						else
						{
							// Test Variables traffic_scalable_video_process
							test_vars.traffic_scalable_video_process[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
							
							forecastSize = 0;
							forecastPktNumber = 0;
							pktPtr = create_a_video_packet(pktSize, frameType, frameTimeStamp, forecastSize, forecastPktNumber, layer + 1, onuNum);
							assign_video_packet(pktPtr, onuNum);
						}
					}
				
					if (layer == (simParams.SCALABLE_VIDEO_NUM_LAYERS - 1)) {
						onuAttrs[onuNum].currentFrame = onuAttrs[onuNum].currentFrame + pktTemp;
					}
				}
			}
			else if (wordCount == 8) {
				forecastSize = pktTemp + atoi(mystring);
			}
			else if (wordCount == 9) {
				forecastPktNumber = atoi(mystring);
			}
			else if (wordCount == 10)
			{
				pktTemp = atoi(mystring);

				wordCount = 0;
				
				if(onuAttrs[onuNum].currentFrame >= onuAttrs[onuNum].startFrame)
				{
					if(onuAttrs[onuNum].wrapAround == 0)
					{
						frameTimeStamp = frameTimeStamp-(onuAttrs[onuNum].startFrame-1)*1000/simParams.FRAME_RATE;
					}
					else
					{
						frameTimeStamp = frameTimeStamp+(onuAttrs[onuNum].sentFrameCount)*1000/simParams.FRAME_RATE;
					}

					if (pktTemp == 1 && layer == (simParams.SCALABLE_VIDEO_NUM_LAYERS - 1))
					{
						// Test Variables traffic_scalable_video_process
						test_vars.traffic_scalable_video_process[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
							
						pktPtr = create_a_video_packet(pktSize, frameType, frameTimeStamp, forecastSize, forecastPktNumber, layer + 1, onuNum);
						assign_video_packet(pktPtr, onuNum);
						onuAttrs[onuNum].sentFrameCount = onuAttrs[onuNum].sentFrameCount + pktTemp;
						hold(1.0/simParams.FRAME_RATE);
					}
					else
					{
						// Test Variables traffic_scalable_video_process
						test_vars.traffic_scalable_video_process[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
							
						pktPtr = create_a_video_packet(pktSize, frameType, frameTimeStamp, forecastSize, forecastPktNumber, layer + 1, onuNum);
						assign_video_packet(pktPtr, onuNum);
					}
					
					//if(onuNum == 1) printf("[%10.5e] ---> PacketSize = %d,  FrameType = %c,  TimeStamp = %lf  for (ONU #%d)\n", simtime(), pktSize, frameType, frameTimeStamp, onuNum);
					
				}
				
				if (layer == (simParams.SCALABLE_VIDEO_NUM_LAYERS - 1))
					onuAttrs[onuNum].currentFrame = onuAttrs[onuNum].currentFrame + pktTemp;
			}
		}
		
		// The packet analysis code will go here.
		
		int wrapAroundCheck = 0;
		if (flag != 2000 && onuAttrs[onuNum].sentFrameCount != onuAttrs[onuNum].maxFrameCount)
		{
			if(onuAttrs[onuNum].wrapAround == 0)
			{
				flag = get_line(inFilePtr, mystring, LINE_LENGTH);
			}
			else
			{
				if (wrapAroundCheck == 0) {
					wrapAroundCheck = 1;
				}
				flag = get_line(inFilePtrWrap, mystring, LINE_LENGTH);
			}
		}
		else 
		{
						
//			FILE* maxFile = fopen("sim_maxCycle","a");
//			fprintf(maxFile, "Time: %lf,  ONU# %d,  Maximum Cycle is %lf\n",onuAttrs[onuNum].maximumGrantCycleTime, onuNum, onuAttrs[onuNum].maximumGrantCycle);
//			fclose(maxFile);
			
//			FILE* avgFile = fopen("sim_avgCycle","a");
//			fprintf(maxFile, "ONU# %d,  Average Cycle Length is: %lf,   Total # of Cycles is: %lf\n", onuNum, onuAttrs[onuNum].averageGrantCycle,onuAttrs[onuNum].totalNumberGrantCycle);
//			fclose(avgFile);
			
			if(onuAttrs[onuNum].sentFrameCount == onuAttrs[onuNum].maxFrameCount)
			{
				flag = 0;
				printf("ONU# %d has read the Video Trace File Successfully\n", onuNum);
				// ONUprint(onuNum, "ONU# %d has read the Video Trace File Successfully\n", onuNum);
				// Test Variables traffic_scalable_video_finish
				test_vars.traffic_scalable_video_finish[test_vars.runNum][test_vars.loadOrderCounter][onuNum]++;
				test_var_print();
				//TSprint("ONU #%d reached the last frame.\n", onuNum);
				
				hold(1 - onuAttrs[onuNum].startoffset);
			
				set(SIM_END_EVENT);
			}
			else
			{
				wordCount = 0;
				flag = get_line(inFilePtrWrap, mystring, LINE_LENGTH);
				//printf("\nFlag = %d",flag);
			}
			
			
			if(onuAttrs[onuNum].wrapAround == 0)
			{
				fclose(inFilePtr);
				onuAttrs[onuNum].wrapAround = 1;
			}
			else
			{
				fclose(inFilePtrWrap);
			}
				
		}

                
#ifdef DEBUG_TRC
        printf("[%10.5e] ---> Generating Packet with %d bytes for ONU #%d\n", simtime(), pktSize, onuNum);
#endif
	}

}



/*
   get_line() takes and array and maximum length, reads in one line of input 
   - but not to exceed the maximum length - places the input in the array 
   and returns the length of the input.
*/
   
int get_line(FILE *input, char *line, int maxlen)
{
	char c;              /* c is the next character, i the loop counter */
    int i;               /* read in until we have EOF or end-of-line    */
    int exit_iaa = 1;
	
	for (i = 0; (i < maxlen-1) && ((c = fgetc(input)) != EOF) && (c != '\n'); i++)
    {
		if (c == '#') {
			while (exit_iaa) {
				c = fgetc(input);
				if (c == '\n') {
					c = fgetc(input);
					if (c != '#') exit_iaa = 0;
				}
			}
		}
		if (c != '\t')	line[i] = c;
		else if (c == '\t' && i != 0)
		{
			line[i] = '\0';          /* terminate the string */
			return 500;
		}
		else 
		{
			line[i] = '\0';	
			return 250;
		}
    }
	if ((c == '\n') && (line[0] != '#')) 
	{
		line[i] = '\0';          /* terminate the string */
		return 1000;
	}
	
	if (c == EOF)
	{
		line[i] = '\0';          /* terminate the string */
		return 2000;
	}
        

    return i;                /* return the length */
}

int get_line2(FILE *input, char *line, int maxlen)
{
	char c;              /* c is the next character, i the loop counter */
    int i;               /* read in until we have EOF or end-of-line    */
    int exit_iaa = 1;
	
	for (i = 0; (i < maxlen-1) && ((c = fgetc(input)) != EOF) && (c != '\n'); i++)
    {
		if (c == '#') {
			while (exit_iaa) {
				c = fgetc(input);
				if (c == '\n') {
					c = fgetc(input);
					if (c != '#') exit_iaa = 0;
				}
			}
		}
		if (c != '\t')	line[i] = c;
		else if (c == '\t' && i != 0)
		{
			line[i] = '\0';          /* terminate the string */
			return 500;
		}
		else 
		{
			line[i] = '\0';	
			return 250;
		}
    }
	if ((c == '\n') && (line[0] != '#')) 
	{
		line[i] = '\0';          /* terminate the string */
		return 1000;
	}
	
	if ((c == '\n') && (line[0] == '#')) 
	{
		line[i] = '\0';          /* terminate the string */
		return -1;
	}	
	
	if (c == EOF)
	{
		line[i] = '\0';          /* terminate the string */
		return 2000;
	}
        

    return i;                /* return the length */
}


