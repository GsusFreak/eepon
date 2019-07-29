
#include <values.h>
#include <stdlib.h>
#include <string.h>
#include <csim.h>
#include "hungarian.h"
#include "eponsim.h"
#include "eponsim_util.h"
#include "eponsim_onu.h"



/* Excess bandwidth distribution variables */
int excessBW, excessShare;
int numOverloaded;
int i;
unsigned long cycleNumber = 0;
double x;
double preLoad = 0;
sONU_LIST onuEntry;
sONU_LIST *overloadedONUList;

/*
 * OLT Utility Functions
 */
int lambda_search(int onuNum, double idealTime)
{
	int currLambda, optimalLambda, search = 1, lambdaNum;
	double optimalTime;
    int	lambdaList[MAX_LAMBDAS], lambdaListCnt;
	
	if(simParams.OLT_TYPE == OLT_LEAST_ASSIGNED)
	{
		return onuAttrs[onuNum].tunedLambda;
	}
    /* Go through list of supported wavelength and find all that
       are available by idealTime
     */
	lambdaNum = 0;
  lambdaListCnt = 0;
	currLambda = onuAttrs[onuNum].supportedLambdas[lambdaNum++];
    while(search)
    {
      if(lambdaFree[currLambda] <= (idealTime + 1e-10))
      {
        /* Add wavelength to list */
        lambdaList[lambdaListCnt++] = currLambda;
      }

      currLambda = onuAttrs[onuNum].supportedLambdas[lambdaNum++];
      if((currLambda == LAMBDA_NULL) || (lambdaNum > simParams.NUM_LAMBDAS))
      {
        search = 0;
      }

    }
    /* If none are available, select next available */
    if(lambdaListCnt == 0)
    {
        lambdaNum = 0;
        currLambda = onuAttrs[onuNum].supportedLambdas[lambdaNum++];
        optimalTime = 1e20;
        optimalLambda = currLambda;
        search = 1;
        while(search)
        {
            /* If optimalTime is negative, an early wavelength has been selected, set
               optimalTime to a small positive value */
            if(optimalTime < 0)
            {
                optimalTime = 10e-9;
            }
            if((onuAttrs[onuNum].wdmType == WDM_TUNABLE) && (onuAttrs[onuNum].tunedLambda != currLambda))
            {
                if((lambdaFree[currLambda] - idealTime + onuAttrs[onuNum].tuningTime) < optimalTime)
                {
                    optimalTime = (lambdaFree[currLambda] - idealTime + onuAttrs[onuNum].tuningTime);
                    optimalLambda = currLambda;
                }
            }
            else
            {
                if((lambdaFree[currLambda] - idealTime) < optimalTime)
                {
                    optimalTime = (lambdaFree[currLambda] - idealTime);
                    optimalLambda = currLambda;
                    if(optimalTime <= simParams.GUARD_TIME)
                    {
                        search = 0; /* can't find a better lambda */
                    }
                }
            }
            currLambda = onuAttrs[onuNum].supportedLambdas[lambdaNum++];
            if((currLambda == LAMBDA_NULL) || (lambdaNum > simParams.NUM_LAMBDAS))
            {
                search = 0;
            }
        }
        if(simParams.WDM_TYPE == WDM_TUNABLE)
        {
            onuAttrs[onuNum].tunedLambda = optimalLambda;
        }
        return optimalLambda;
    }
    else if (lambdaListCnt > 1) 
    {
        /* Randomly select a wavelength from this list */
        optimalLambda = rand() % lambdaListCnt;
        if(simParams.WDM_TYPE == WDM_TUNABLE)
        {
            onuAttrs[onuNum].tunedLambda = lambdaList[optimalLambda];
        }
        return lambdaList[optimalLambda];
    }
    else
    {
        if(simParams.WDM_TYPE == WDM_TUNABLE)
        {
            onuAttrs[onuNum].tunedLambda = lambdaList[0];
        }
        return lambdaList[0];
    }
}


int lambda_search_temp(int onuNum, double idealTime)
{
	int currLambda, optimalLambda, search, lambdaNum;
	double optimalTime;
  int	lambdaList[MAX_LAMBDAS], lambdaListCnt;
	
	if(simParams.OLT_TYPE == OLT_LEAST_ASSIGNED)
	{
		return onuAttrs[onuNum].tunedLambda;
	}
    /* Go through list of supported wavelength and find all that
       are available by idealTime
     */
	lambdaNum = 0;
  lambdaListCnt = 0;
	currLambda = onuAttrs[onuNum].supportedLambdas[lambdaNum++];
    while(search)
    {
        if(lambdaFreeTemp[currLambda] <= (idealTime + 1e-10))
        {
            /* Add wavelength to list */
            lambdaList[lambdaListCnt++] = currLambda;
        }

        currLambda = onuAttrs[onuNum].supportedLambdas[lambdaNum++];
        if((currLambda == LAMBDA_NULL) || (lambdaNum > simParams.NUM_LAMBDAS))
        {
            search = 0;
        }

    }
    /* If none are available, select next available */
    if(lambdaListCnt == 0)
    {
        lambdaNum = 0;
        currLambda = onuAttrs[onuNum].supportedLambdas[lambdaNum++];
        optimalTime = 1e20;
        optimalLambda = currLambda;
        search = 1;
        while(search)
        {
            /* If optimalTime is negative, an early wavelength has been selected, set
               optimalTime to a small positive value */
            if(optimalTime < 0)
            {
                optimalTime = 10e-9;
            }
            if((onuAttrs[onuNum].wdmType == WDM_TUNABLE) && (onuAttrs[onuNum].tunedLambda != currLambda))
            {
                if((lambdaFreeTemp[currLambda] - idealTime + onuAttrs[onuNum].tuningTime) < optimalTime)
                {
                    optimalTime = (lambdaFreeTemp[currLambda] - idealTime + onuAttrs[onuNum].tuningTime);
                    optimalLambda = currLambda;
                }
            }
            else
            {
                if((lambdaFreeTemp[currLambda] - idealTime) < optimalTime)
                {
                    optimalTime = (lambdaFreeTemp[currLambda] - idealTime);
                    optimalLambda = currLambda;
                    if(optimalTime <= simParams.GUARD_TIME)
                    {
                        search = 0; /* can't find a better lambda */
                    }
                }
            }
            currLambda = onuAttrs[onuNum].supportedLambdas[lambdaNum++];
            if((currLambda == LAMBDA_NULL) || (lambdaNum > simParams.NUM_LAMBDAS))
            {
                search = 0;
            }
        }
        if(simParams.WDM_TYPE == WDM_TUNABLE)
        {
            onuAttrs[onuNum].tunedLambda = optimalLambda;
        }
        return optimalLambda;
    }
    else if (lambdaListCnt > 1) 
    {
        /* Randomly select a wavelength from this list */
        optimalLambda = rand() % lambdaListCnt;
        if(simParams.WDM_TYPE == WDM_TUNABLE)
        {
            onuAttrs[onuNum].tunedLambda = lambdaList[optimalLambda];
        }
        return lambdaList[optimalLambda];
    }
    else
    {
        if(simParams.WDM_TYPE == WDM_TUNABLE)
        {
            onuAttrs[onuNum].tunedLambda = lambdaList[0];
        }
        return lambdaList[0];
    }
}


// void forecast_grant_sizing(int onuNum, double start_time)
// {	
// 	if(simParams.OLT_TYPE == OLT_IPACT_PSF)
// 	{
// 		if(preLoad != simParams.DESIRED_LOAD && simType == ACTUAL_RUN)
// 		{
// 			preLoad = simParams.DESIRED_LOAD;
// 			cycleNumber = 0;
// 		}
// 		if(start_time * simParams.FRAME_RATE >= cycleNumber)  // Fix this if statement when we have a shift in transmission (Important)!!!!!!!!!!!!!!!!!!!!
// 		{
// 			for(i=0; i < simParams.NUM_SS_STREAMS; i++)
// 			{
// 				onuAttrs[i].remainPSFgrant = simParams.MAX_GRANT_SIZE; //Limted Data - Limited Video
// 				onuAttrs[i].videoServiced = NOT_SERVICED;
// 			}
// 			cycleNumber = cycleNumber + 1;
// 		}
// 	}
// 	
// 	if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
// 	{
// 		if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED)
// 		{	
// 			if(start_time >= onuAttrs[onuNum].rptForecastVideoTime)
// 			{
// 				onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize + onuAttrs[onuNum].rptForecastVideoQueueSize;
// 			}
// 			else
// 			{
// 				onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize;
// 			}
// 		}
// 		else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_FIXED)
// 		{
// 			onuAttrs[onuNum].videoGrantLen = simParams.FIXED_GRANT_SIZE;
// 		}
// 		else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_LIMITED_GATE)
// 		{
// 			if(start_time >= onuAttrs[onuNum].rptForecastVideoTime)
// 			{
// 				if(onuAttrs[onuNum].rptVideoQueueSize + onuAttrs[onuNum].rptForecastVideoQueueSize <= simParams.MAX_GRANT_SIZE)
// 				{
// 					onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize + onuAttrs[onuNum].rptForecastVideoQueueSize;
// 				}
// 				else
// 				{
// 					onuAttrs[onuNum].videoGrantLen = simParams.MAX_GRANT_SIZE;
// 				}
// 			}
// 			else
// 			{
// 				if(onuAttrs[onuNum].rptVideoQueueSize <= simParams.MAX_GRANT_SIZE)
// 				{
// 					onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize;
// 				}
// 				else
// 				{
// 					onuAttrs[onuNum].videoGrantLen = simParams.MAX_GRANT_SIZE;
// 				}
// 			}
// 		}
// 	}
// 	else
// 	{
// 		onuAttrs[onuNum].videoGrantLen = 0;
// 	}
// 	
// 	x = start_time * simParams.FRAME_RATE;
// 	if(simParams.OLT_TYPE == OLT_IPACT_PSF && onuAttrs[onuNum].videoServiced == NOT_SERVICED)
// 	{
// 		onuAttrs[onuNum].grantLen = onuAttrs[onuNum].videoGrantLen;
// 		onuAttrs[onuNum].remainPSFgrant = onuAttrs[onuNum].remainPSFgrant - onuAttrs[onuNum].grantLen;
// 		onuAttrs[onuNum].dataGrantLen = 0;
// 		onuAttrs[onuNum].videoServiced = SERVICED;
// 	}
// 	else
// 	{	
// 		switch(simParams.DBA_TYPE)
// 		{
// 			case DBA_GATED:
// 				onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
// 				onuAttrs[onuNum].grantLen = onuAttrs[onuNum].dataGrantLen + onuAttrs[onuNum].videoGrantLen;
// 				break;
// 		
// 			case DBA_FIXED:
// 				if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
// 				{
// 					if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED || simParams.VIDEO_DBA_TYPE == VIDEO_DBA_LIMITED_GATE)
// 					{
// 						onuAttrs[onuNum].dataGrantLen = simParams.FIXED_GRANT_SIZE;
// 					}
// 					else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_FIXED)
// 					{
// 						if(start_time >= onuAttrs[onuNum].rptForecastVideoTime)
// 						{
// 							if(onuAttrs[onuNum].rptVideoQueueSize + onuAttrs[onuNum].rptForecastVideoQueueSize < simParams.FIXED_GRANT_SIZE)
// 							{
// 								onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize + onuAttrs[onuNum].rptForecastVideoQueueSize;
// 								onuAttrs[onuNum].dataGrantLen = simParams.FIXED_GRANT_SIZE - onuAttrs[onuNum].videoGrantLen;
// 							}
// 							else
// 							{
// 								onuAttrs[onuNum].videoGrantLen = simParams.FIXED_GRANT_SIZE;
// 								onuAttrs[onuNum].dataGrantLen = 0;
// 							}
// 						}
// 						else
// 						{
// 							if(onuAttrs[onuNum].rptVideoQueueSize < simParams.FIXED_GRANT_SIZE)
// 							{
// 								onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize;
// 								onuAttrs[onuNum].dataGrantLen = simParams.FIXED_GRANT_SIZE - onuAttrs[onuNum].rptVideoQueueSize;
// 							}
// 							else
// 							{
// 								onuAttrs[onuNum].videoGrantLen = simParams.FIXED_GRANT_SIZE;
// 								onuAttrs[onuNum].dataGrantLen = 0;
// 							}
// 						}
// 					}
// 				}
// 				else
// 				{
// 					onuAttrs[onuNum].dataGrantLen = simParams.FIXED_GRANT_SIZE;
// 				}
// 			
// 				onuAttrs[onuNum].grantLen = onuAttrs[onuNum].dataGrantLen + onuAttrs[onuNum].videoGrantLen;
// 				break;
// 		    
// 		    
// 			case DBA_LIMITED_GATE:
// 				if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
// 				{
// 					if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED || simParams.VIDEO_DBA_TYPE == VIDEO_DBA_FIXED)
// 					{
// 						if(onuAttrs[onuNum].rptQueueSize <= simParams.MAX_GRANT_SIZE)
// 						{
// 							onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
// 						}
// 						else
// 						{
// 							onuAttrs[onuNum].dataGrantLen = simParams.MAX_GRANT_SIZE;
// 						}
// 					}
// 					else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_LIMITED_GATE)
// 					{
// 						if(simParams.OLT_TYPE == OLT_IPACT_PSF)
// 						{
// 							if(onuAttrs[onuNum].videoGrantLen < onuAttrs[onuNum].remainPSFgrant)
// 							{
// 								onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].remainPSFgrant - onuAttrs[onuNum].videoGrantLen;
// 								
// 								if(onuAttrs[onuNum].rptQueueSize <  onuAttrs[onuNum].dataGrantLen)
// 								{
// 									onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
// 								}
// 								
// 								onuAttrs[onuNum].remainPSFgrant = onuAttrs[onuNum].remainPSFgrant - onuAttrs[onuNum].dataGrantLen;
// 							}
// 							else
// 							{
// 								onuAttrs[onuNum].dataGrantLen = 0;
// 							}
// 							
// 							if(onuNum != (simParams.NUM_SS_STREAMS - 1))
// 							{
// 								onuAttrs[onuNum + 1].remainPSFgrant = onuAttrs[onuNum + 1].remainPSFgrant + onuAttrs[onuNum].remainPSFgrant;
// 								onuAttrs[onuNum].remainPSFgrant = 0;
// 							}
// 							else
// 							{
// 								onuAttrs[0].remainPSFgrant = onuAttrs[onuNum].remainPSFgrant;
// 								onuAttrs[onuNum].remainPSFgrant = 0;
// 							}
// 						}
// 						else
// 						{
// 						
// 							if(onuAttrs[onuNum].videoGrantLen < simParams.MAX_GRANT_SIZE)
// 							{
// 								onuAttrs[onuNum].dataGrantLen = simParams.MAX_GRANT_SIZE - onuAttrs[onuNum].videoGrantLen;
// 								if(onuAttrs[onuNum].rptQueueSize <  onuAttrs[onuNum].dataGrantLen)
// 								{
// 									onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
// 								}
// 							}
// 							else
// 							{
// 								onuAttrs[onuNum].dataGrantLen = 0;
// 							}
// 						}
// 					}
// 				}
// 				else
// 				{
// 					if(onuAttrs[onuNum].rptQueueSize < simParams.MAX_GRANT_SIZE)
// 					{
// 						onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
// 					}
// 					else
// 					{
// 						onuAttrs[onuNum].dataGrantLen =simParams.MAX_GRANT_SIZE;
// 					}	
// 				}	
// 				
// 				onuAttrs[onuNum].grantLen = onuAttrs[onuNum].dataGrantLen + onuAttrs[onuNum].videoGrantLen;
// 				break;
// 				  
// 			case DBA_EXCESS:
// 				onuAttrs[onuNum].grantLen = onuAttrs[onuNum].rptQueueSize;
// 				if(onuAttrs[onuNum].grantLen > simParams.MAX_GRANT_SIZE)
// 				{
// 					/* Overloaded ONU */
// 					onuAttrs[onuNum].grantLen = simParams.MAX_GRANT_SIZE;
// 					onuEntry.onuNum = onuNum;
// 					onuEntry.posNum = numOverloaded++;
// 					overloadedONUList = onu_list_insert(SORT_ASCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_ONU_NUM, SORT_POS_NUM, &onuEntry, overloadedONUList);
// 				}
// 				else
// 				{
// 					/* Underloaded ONU */
// 					/* Contribute to excess */
// 					excessBW += (simParams.MAX_GRANT_SIZE - onuAttrs[onuNum].grantLen);
// 					record((simParams.MAX_GRANT_SIZE - onuAttrs[onuNum].grantLen), excessBandwidthONU);
// 				}
// 				break;
// 		}
// 	}
// 	
// 	/*##########################################################################################################################################################
// 
// 	if(start_time <= 0.404 && start_time >= 0.304)
// 	{
// 		printf("[%10.5e] ONU#%d, Start Time = %lf, Forecast Video Time = %lf, Video Grant Report = %ld, Video Grant = %ld \n",simtime() ,onuNum, start_time, onuAttrs[onuNum].rptForecastVideoTime, onuAttrs[onuNum].rptVideoQueueSize, onuAttrs[onuNum].videoGrantLen);
// 	}
// 
// 	##########################################################################################################################################################*/
// 	
// 	/*//##########################################################################################################################################################
// 
// 	printf("[%10.5e] ONU#%d, Grant Length = %ld \n",simtime() ,onuNum, onuAttrs[onuNum].grantLen);
// 	
// 	if(simtime() <= 9172.806350 && simtime() >= 9172.806100)
// 	{
// 		printf("[%10.5e] ONU#%d, Grant Length = %ld \n",simtime() ,onuNum, onuAttrs[onuNum].grantLen);
// 	}
// 
// 	//##########################################################################################################################################################*/
// 		
// 	
// 	return;
// }


// void grant_sizing_PSF(int onuNum, double start_time)
// {
// 	if(simParams.OLT_TYPE == OLT_IPACT_PSF || simType != ACTUAL_RUN)
// 	{
// 		if(preLoad != simParams.DESIRED_LOAD && simType == ACTUAL_RUN)
// 		{
// 			preLoad = simParams.DESIRED_LOAD;
// 			cycleNumber = 0;
// 		}
// 		if(start_time * simParams.FRAME_RATE >= cycleNumber)  // Fix this if statement when we have a shift in transmission (Important)!!!!!!!!!!!!!!!!!!!!
// 		{
// 			for(i=0; i < simParams.NUM_SS_STREAMS; i++)
// 			{
// 				onuAttrs[i].remainPSFgrant = simParams.MAX_GRANT_SIZE; //Limted Data - Limited Video
// 				if(simType == ACTUAL_RUN)
// 				{
// 					onuAttrs[i].videoServiced = NOT_SERVICED;
// 				}
// 				else
// 				{
// 					onuAttrs[i].videoServiced = SERVICED;
// 				}
// 			}
// 			cycleNumber = cycleNumber + 1;
// 		}
// 	}
// 
// 	if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON && simType == ACTUAL_RUN)
// 	{
// 		if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED)
// 		{
// 			onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize;
// 		}
// 		else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_FIXED)
// 		{
// 			onuAttrs[onuNum].videoGrantLen = simParams.FIXED_GRANT_SIZE;
// 		}
// 		else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_LIMITED_GATE)
// 		{
// 			if(onuAttrs[onuNum].rptVideoQueueSize <= simParams.MAX_GRANT_SIZE)
// 			{
// 				onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize;
// 			}
// 			else
// 			{
// 				onuAttrs[onuNum].videoGrantLen = simParams.MAX_GRANT_SIZE;
// 			}
// 		}
// 	}
// 	else
// 	{
// 		onuAttrs[onuNum].videoGrantLen = 0;
// 	}
// 	
// 	if(simParams.OLT_TYPE == OLT_IPACT_PSF && onuAttrs[onuNum].videoServiced == NOT_SERVICED /*&& simParams.VIDEO_PREDICTION == VIDEO_PREDICTION_OFF*/)
// 	{
// 		onuAttrs[onuNum].grantLen = onuAttrs[onuNum].videoGrantLen;
// 		onuAttrs[onuNum].videoServiced = SERVICED;
// 		onuAttrs[onuNum].dataGrantLen = 0;
// 		onuAttrs[onuNum].remainPSFgrant = onuAttrs[onuNum].remainPSFgrant - onuAttrs[onuNum].grantLen;
// 		//printf("ONU# %d Serviced, txf=%lf, cycle#=%ld\n",onuNum,x,cycleNumber);
// 	}
// 	else
// 	{
// 		switch(simParams.DBA_TYPE)
// 		{
// 			case DBA_GATED:
// 				onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
// 				onuAttrs[onuNum].grantLen = onuAttrs[onuNum].dataGrantLen + onuAttrs[onuNum].videoGrantLen;
// 				break;
// 		
// 			case DBA_FIXED:
// 				if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
// 				{
// 					if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED || simParams.VIDEO_DBA_TYPE == VIDEO_DBA_LIMITED_GATE)
// 					{
// 						onuAttrs[onuNum].dataGrantLen = simParams.FIXED_GRANT_SIZE;
// 					}
// 					else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_FIXED)
// 					{
// 						if(onuAttrs[onuNum].rptVideoQueueSize < simParams.FIXED_GRANT_SIZE)
// 						{
// 							onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize;
// 							onuAttrs[onuNum].dataGrantLen = simParams.FIXED_GRANT_SIZE - onuAttrs[onuNum].rptVideoQueueSize;
// 						}
// 						else
// 						{
// 							onuAttrs[onuNum].videoGrantLen = simParams.FIXED_GRANT_SIZE;
// 							onuAttrs[onuNum].dataGrantLen = 0;
// 						}
// 					}
// 				}
// 				else
// 				{
// 					onuAttrs[onuNum].dataGrantLen = simParams.FIXED_GRANT_SIZE;
// 				}
// 			
// 				onuAttrs[onuNum].grantLen = onuAttrs[onuNum].dataGrantLen + onuAttrs[onuNum].videoGrantLen;
// 				break;
// 		    
// 		    
// 			case DBA_LIMITED_GATE:
// 				if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON && simType == ACTUAL_RUN)
// 				{
// 					if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED || simParams.VIDEO_DBA_TYPE == VIDEO_DBA_FIXED)
// 					{
// 						if(onuAttrs[onuNum].rptQueueSize <= simParams.MAX_GRANT_SIZE)
// 						{
// 							onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
// 						}
// 						else
// 						{
// 							onuAttrs[onuNum].dataGrantLen = simParams.MAX_GRANT_SIZE;
// 						}
// 					}
// 					else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_LIMITED_GATE)
// 					{
// 						if(simParams.OLT_TYPE == OLT_IPACT_PSF)
// 						{
// 							if(onuAttrs[onuNum].videoGrantLen < onuAttrs[onuNum].remainPSFgrant)
// 							{
// 								onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].remainPSFgrant - onuAttrs[onuNum].videoGrantLen;
// 								
// 								if(onuAttrs[onuNum].rptQueueSize <  onuAttrs[onuNum].dataGrantLen)
// 								{
// 									onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
// 								}
// 								
// 								onuAttrs[onuNum].remainPSFgrant = onuAttrs[onuNum].remainPSFgrant - onuAttrs[onuNum].dataGrantLen;
// 							}
// 							else
// 							{
// 								onuAttrs[onuNum].dataGrantLen = 0;
// 							}
// 							
// 							if(onuNum != (simParams.NUM_SS_STREAMS - 1))
// 							{
// 								onuAttrs[onuNum + 1].remainPSFgrant = onuAttrs[onuNum + 1].remainPSFgrant + onuAttrs[onuNum].remainPSFgrant;
// 								onuAttrs[onuNum].remainPSFgrant = 0;
// 							}
// 							else
// 							{
// 								onuAttrs[0].remainPSFgrant = onuAttrs[onuNum].remainPSFgrant;
// 								onuAttrs[onuNum].remainPSFgrant = 0;
// 							}
// 						}
// 					
// 						else
// 						{
// 							if(onuAttrs[onuNum].videoGrantLen < simParams.MAX_GRANT_SIZE)
// 							{
// 								onuAttrs[onuNum].dataGrantLen = simParams.MAX_GRANT_SIZE - onuAttrs[onuNum].videoGrantLen;
// 								if(onuAttrs[onuNum].rptQueueSize <  onuAttrs[onuNum].dataGrantLen)
// 								{
// 									onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
// 								}
// 							}
// 							else
// 							{
// 								onuAttrs[onuNum].dataGrantLen = 0;
// 							}
// 						}
// 					}
// 				}
// 				else
// 				{
// 					if(onuAttrs[onuNum].rptQueueSize < simParams.MAX_GRANT_SIZE)
// 					{
// 						onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
// 					}
// 					else
// 					{
// 						onuAttrs[onuNum].dataGrantLen = simParams.MAX_GRANT_SIZE;
// 					}
// 				}	
// 				
// 				onuAttrs[onuNum].grantLen = onuAttrs[onuNum].dataGrantLen + onuAttrs[onuNum].videoGrantLen;
// 				break;
// 				  
// 			case DBA_EXCESS:
// 				onuAttrs[onuNum].grantLen = onuAttrs[onuNum].rptQueueSize;
// 				if(onuAttrs[onuNum].grantLen > simParams.MAX_GRANT_SIZE)
// 				{
// 					/* Overloaded ONU */
// 					onuAttrs[onuNum].grantLen = simParams.MAX_GRANT_SIZE;
// 					onuEntry.onuNum = onuNum;
// 					onuEntry.posNum = numOverloaded++;
// 					overloadedONUList = onu_list_insert(SORT_ASCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_ONU_NUM, SORT_POS_NUM, &onuEntry, overloadedONUList);
// 				}
// 				else
// 				{
// 					/* Underloaded ONU */
// 					/* Contribute to excess */
// 					excessBW += (simParams.MAX_GRANT_SIZE - onuAttrs[onuNum].grantLen);
// 					record((simParams.MAX_GRANT_SIZE - onuAttrs[onuNum].grantLen), excessBandwidthONU);
// 				}
// 				break;
// 		}
// 	}
// 	
// 	//// Drop packets to match the availability of bandwidth
// 	//if (simParams.SCALABLE_VIDEO_TRAFFIC == SCALABLE_VIDEO_ON)
// 		//drop_scalable_video_packets(onuNum, onuAttrs[onuNum].videoGrantLen);	
// 	
// 	return;
// }


void grant_sizing(int onuNum)
{
	
	//// These functions check the ONU for excessively long queues
	//check_video_packet_list(onuNum);
	//check_data_packet_list(onuNum);
	
	
	// This code is used to evaluate the prospective grant size for the 
	// scalable video dynamic dropping alogorithm
	double temp_videoGrantLen,
			temp_dataGrantLen,
			temp_grantLen;
	
	if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
	{
		if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED)
		{
			temp_videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize;
		}
		else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_FIXED)
		{
			temp_videoGrantLen = simParams.FIXED_GRANT_SIZE;
		}
		else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_LIMITED_GATE)
		{
			if(onuAttrs[onuNum].rptVideoQueueSize <= (int)(simParams.MAX_GRANT_SIZE*simParams.SV_DROP_SENSITIVITY))
			{
				temp_videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize;
			}
			else
			{
				temp_videoGrantLen = (int)(simParams.MAX_GRANT_SIZE*simParams.SV_DROP_SENSITIVITY);
			}
		}
	}
	else
	{
		temp_videoGrantLen = 0;
	}
	
	switch(simParams.DBA_TYPE)
	{
		case DBA_GATED:
			temp_dataGrantLen = onuAttrs[onuNum].rptQueueSize;
			temp_grantLen = temp_dataGrantLen + temp_videoGrantLen;
			
			//#ifdef DEBUG_ONU
			//if (onuNum == 1 && simtime() >= 7200)	printf("OLT: D-Gate=%ld, Gate=%ld, ONU# %d\n",onuAttrs[onuNum].dataGrantLen, onuAttrs[onuNum].grantLen, onuNum);
			//#endif
			
			break;
	
		case DBA_FIXED:
			if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
			{
				if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED || simParams.VIDEO_DBA_TYPE == VIDEO_DBA_LIMITED_GATE)
				{
					temp_dataGrantLen = simParams.FIXED_GRANT_SIZE;
				}
				else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_FIXED)
				{
					if(onuAttrs[onuNum].rptVideoQueueSize < simParams.FIXED_GRANT_SIZE)
					{
						temp_videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize;
						temp_dataGrantLen = simParams.FIXED_GRANT_SIZE - onuAttrs[onuNum].rptVideoQueueSize;
					}
					else
					{
						temp_videoGrantLen = simParams.FIXED_GRANT_SIZE;
						temp_dataGrantLen = 0;
					}
				}
			}
			else
			{
				temp_dataGrantLen = simParams.FIXED_GRANT_SIZE;
			}
		
			temp_grantLen = temp_dataGrantLen + temp_videoGrantLen;
			break;
	    
	    
		case DBA_LIMITED_GATE:
			if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
			{
				if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED || simParams.VIDEO_DBA_TYPE == VIDEO_DBA_FIXED)
				{
					if(onuAttrs[onuNum].rptQueueSize <= (int)(simParams.MAX_GRANT_SIZE*simParams.SV_DROP_SENSITIVITY))
					{
						temp_dataGrantLen = onuAttrs[onuNum].rptQueueSize;
					}
					else
					{
						temp_dataGrantLen = (int)(simParams.MAX_GRANT_SIZE*simParams.SV_DROP_SENSITIVITY);
					}
				}
				else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_LIMITED_GATE)
				{
					if(temp_videoGrantLen < (int)(simParams.MAX_GRANT_SIZE*simParams.SV_DROP_SENSITIVITY))
					{
						temp_dataGrantLen = (int)(simParams.MAX_GRANT_SIZE*simParams.SV_DROP_SENSITIVITY) - temp_videoGrantLen;
						if(onuAttrs[onuNum].rptQueueSize <  temp_dataGrantLen)
						{
							temp_dataGrantLen = onuAttrs[onuNum].rptQueueSize;
						}
					}
					else
					{
						temp_dataGrantLen = 0;
					}
				}
			}
			else
			{
				if(onuAttrs[onuNum].rptQueueSize < (int)(simParams.MAX_GRANT_SIZE*simParams.SV_DROP_SENSITIVITY))
				{
					temp_dataGrantLen = onuAttrs[onuNum].rptQueueSize;
				}
				else
				{
					temp_dataGrantLen = (int)(simParams.MAX_GRANT_SIZE*simParams.SV_DROP_SENSITIVITY);
				}
			}	
			
			temp_grantLen = temp_dataGrantLen + temp_videoGrantLen;
			break;
			  
		case DBA_EXCESS:
			temp_grantLen = onuAttrs[onuNum].rptQueueSize;
			if(temp_grantLen > (int)(simParams.MAX_GRANT_SIZE*simParams.SV_DROP_SENSITIVITY))
			{
				/* Overloaded ONU */
				temp_grantLen = (int)(simParams.MAX_GRANT_SIZE*simParams.SV_DROP_SENSITIVITY);
				//onuEntry.onuNum = onuNum;
				//onuEntry.posNum = numOverloaded++;
				//overloadedONUList = onu_list_insert(SORT_ASCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_ONU_NUM, SORT_POS_NUM, &onuEntry, overloadedONUList);
			}
			else
			{
				/* Underloaded ONU */
				/* Contribute to excess */
				//excessBW += (simParams.MAX_GRANT_SIZE - temp_grantLen);
				//record((simParams.MAX_GRANT_SIZE - temp_grantLen), excessBandwidthONU);
			}
			break;
	}
	
	// Drop particular packets to smooth out the playback
	drop_scalable_video_packets(onuNum, temp_grantLen, 0, 0, 2);
	
	
	
	
	
	
	
	
	
	
	
	// This code assigns the ONU its grant
	if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
	{
		if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED)
		{
			onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize;
		}
		else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_FIXED)
		{
			onuAttrs[onuNum].videoGrantLen = simParams.FIXED_GRANT_SIZE;
		}
		else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_LIMITED_GATE)
		{
			if(onuAttrs[onuNum].rptVideoQueueSize <= simParams.MAX_GRANT_SIZE)
			{
				onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize;
			}
			else
			{
				onuAttrs[onuNum].videoGrantLen = simParams.MAX_GRANT_SIZE;
			}
		}
	}
	else
	{
		onuAttrs[onuNum].videoGrantLen = 0;
	}
	
	switch(simParams.DBA_TYPE)
	{
		case DBA_GATED:
			onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
			onuAttrs[onuNum].grantLen = onuAttrs[onuNum].dataGrantLen + onuAttrs[onuNum].videoGrantLen;
			
			// if (onuNum == 1 && simtime() >= 7200)	printf("OLT: D-Gate=%ld, Gate=%ld, ONU# %d\n",onuAttrs[onuNum].dataGrantLen, onuAttrs[onuNum].grantLen, onuNum);
			
			break;
	
		case DBA_FIXED:
			if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
			{
				if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED || simParams.VIDEO_DBA_TYPE == VIDEO_DBA_LIMITED_GATE)
				{
					onuAttrs[onuNum].dataGrantLen = simParams.FIXED_GRANT_SIZE;
				}
				else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_FIXED)
				{
					if(onuAttrs[onuNum].rptVideoQueueSize < simParams.FIXED_GRANT_SIZE)
					{
						onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize;
						onuAttrs[onuNum].dataGrantLen = simParams.FIXED_GRANT_SIZE - onuAttrs[onuNum].rptVideoQueueSize;
					}
					else
					{
						onuAttrs[onuNum].videoGrantLen = simParams.FIXED_GRANT_SIZE;
						onuAttrs[onuNum].dataGrantLen = 0;
					}
				}
			}
			else
			{
				onuAttrs[onuNum].dataGrantLen = simParams.FIXED_GRANT_SIZE;
			}
		
			onuAttrs[onuNum].grantLen = onuAttrs[onuNum].dataGrantLen + onuAttrs[onuNum].videoGrantLen;
			break;
	    
	    
		case DBA_LIMITED_GATE:
			if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
			{
				if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED || simParams.VIDEO_DBA_TYPE == VIDEO_DBA_FIXED)
				{
					if(onuAttrs[onuNum].rptQueueSize <= simParams.MAX_GRANT_SIZE)
					{
						onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
					}
					else
					{
						onuAttrs[onuNum].dataGrantLen = simParams.MAX_GRANT_SIZE;
					}
				}
				else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_LIMITED_GATE)
				{
					if(onuAttrs[onuNum].videoGrantLen < simParams.MAX_GRANT_SIZE)
					{
						onuAttrs[onuNum].dataGrantLen = simParams.MAX_GRANT_SIZE - onuAttrs[onuNum].videoGrantLen;
						if(onuAttrs[onuNum].rptQueueSize <  onuAttrs[onuNum].dataGrantLen)
						{
							onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
						}
					}
					else
					{
						onuAttrs[onuNum].dataGrantLen = 0;
					}
				}
			}
			else
			{
				if(onuAttrs[onuNum].rptQueueSize < simParams.MAX_GRANT_SIZE)
				{
					onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
				}
				else
				{
					onuAttrs[onuNum].dataGrantLen =simParams.MAX_GRANT_SIZE;
				}
			}	
			
			onuAttrs[onuNum].grantLen = onuAttrs[onuNum].dataGrantLen + onuAttrs[onuNum].videoGrantLen;
			break;
			  
		case DBA_EXCESS:
			onuAttrs[onuNum].grantLen = onuAttrs[onuNum].rptQueueSize;
			if(onuAttrs[onuNum].grantLen > simParams.MAX_GRANT_SIZE)
			{
				/* Overloaded ONU */
				onuAttrs[onuNum].grantLen = simParams.MAX_GRANT_SIZE;
				onuEntry.onuNum = onuNum;
				onuEntry.posNum = numOverloaded++;
				overloadedONUList = onu_list_insert(SORT_ASCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_ONU_NUM, SORT_POS_NUM, &onuEntry, overloadedONUList);
			}
			else
			{
				/* Underloaded ONU */
				/* Contribute to excess */
				excessBW += (simParams.MAX_GRANT_SIZE - onuAttrs[onuNum].grantLen);
				record((simParams.MAX_GRANT_SIZE - onuAttrs[onuNum].grantLen), excessBandwidthONU);
			}
			break;
	}
		
	if(simtime() <= 9172.806350 && simtime() >= 9172.805875)
	{
		printf("[%10.10e] ONU#%d, Grant Length = %.0f \n",simtime() ,onuNum, onuAttrs[onuNum].grantLen);
	}
	
	return;	
}





void grant_excess()
{	
	/* Divide the total excess by the number of overloaded ONUs (i.e., Equitable Division) */
	excessShare = (int)((double)excessBW)/((double)numOverloaded);
	//excessShare = 0;
	
	/* For each overloaded ONU, allocate the excess up to what is requested by the overloaded ONU (i.e., Controlled Allocation) */
	while(overloadedONUList != NULL)
	{
		overloadedONUList = onu_list_pop(overloadedONUList, &onuEntry);
		if(onuAttrs[onuEntry.onuNum].rptQueueSize <= (simParams.MAX_GRANT_SIZE + excessShare))
		{
			onuAttrs[onuEntry.onuNum].grantLen = onuAttrs[onuEntry.onuNum].rptQueueSize;
		}
		else
		{
			onuAttrs[onuEntry.onuNum].grantLen = simParams.MAX_GRANT_SIZE + excessShare;			
		}
	}
}

/* Grant Trace function */
void grant_trace(int onuNum, sENTITY_GATE_PKT *currentGATE)
{
    FILE *grantTrcFile;
    int loopIndex;
    char filename_str[100];

    /* Store Grant information in array */
    grantTrace[currentGATE->lambda][grantTracePtr[currentGATE->lambda]].onuNum = onuNum;
    grantTrace[currentGATE->lambda][grantTracePtr[currentGATE->lambda]].startTime = currentGATE->start;
    grantTrace[currentGATE->lambda][grantTracePtr[currentGATE->lambda]++].length = currentGATE->length;
    /* When array is full, flush it to a file */
    if(grantTracePtr[currentGATE->lambda] >= GRANT_TRACE_SIZE)
    {
        /* Flush array to a file */
        filename_str[0] = '\0';
        strcat(filename_str, "grant_trc_");
        sprintf(filename_str, "%s%dw", filename_str, currentGATE->lambda+1);
        grantTrcFile = fopen(filename_str,"a");
        for(loopIndex=0; loopIndex < GRANT_TRACE_SIZE; loopIndex++)
        {
            fprintf(grantTrcFile,"%d %e %e\n",grantTrace[currentGATE->lambda][loopIndex].onuNum,
                    grantTrace[currentGATE->lambda][loopIndex].startTime,grantTrace[currentGATE->lambda][loopIndex].length);
        }
        fclose(grantTrcFile);
        /* Reset Pointer */
        grantTracePtr[currentGATE->lambda] = 0;
    }
}

/* 
 * FUNCTION: schedule_onu()
 * DESCRIPTION: Utility function used to schedule an ONU with or without
 *              pre-selected lambda
 *
 */
int schedule_onu(int onuNum, int lambdaNum)
{
  double earliest_schedule_time, earliest_gate_tx_time;
  int loopIndex;
	sENTITY_GATE_PKT *currentGATE;

  /* Record REPORT to Schedule times for this ONU */
  if(onuAttrs[onuNum].prevGateTime != 0)
  {
    record((simtime() - onuAttrs[onuNum].rptTime),overallRptToSchedTime);
    if(onuNum < simParams.NUM_HEAVY_ONU)
    {
      record((simtime() - onuAttrs[onuNum].rptTime),heavyRptToSchedTime);
    }
    else
    {
      record((simtime() - onuAttrs[onuNum].rptTime),lightRptToSchedTime);
    }
  }

  /* Update all Lambdas with available time < current time to current time */
  for(loopIndex = 0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
  {
    if(lambdaFree[loopIndex] < simtime())
    {
      lambdaFree[loopIndex] = simtime();
    }
  }

  /* setup a new GATE message */
  currentGATE = create_a_gate();

  /* calculate earliest possible time to schedule this ONU */
  earliest_gate_tx_time = (downstreamFree > simtime()) ? downstreamFree : simtime();
  earliest_schedule_time = earliest_gate_tx_time + (64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME + onuAttrs[onuNum].rtt;
  downstreamFree = earliest_gate_tx_time + (64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME;

  // printf("[%10.5e] earliest_schedule_time for ONU #%d = [%10.5e]\n",simtime(),onuNum,earliest_schedule_time);
  // fflush(NULL);

  if(lambdaNum == LAMBDA_NULL)
  {
    /* find best candidate lambda to issue grant */
    currentGATE->lambda = lambda_search(onuNum, earliest_schedule_time);
  }
  else
  {
    /* Use pre-selected lambda */
    currentGATE->lambda = lambdaNum;
  }

  /* Setup the GATE */
  /* setup grant start time and length (does not include REPORT, REPORT kept separate to keep out of utilization calc) */

  if((onuAttrs[onuNum].wdmType == WDM_TUNABLE) && 
    (onuAttrs[onuNum].tunedLambda != currentGATE->lambda) && 
    (lambdaFree[currentGATE->lambda] < (simtime() + onuAttrs[onuNum].tuningTime)))
  {
    /* need to account for some or all of the tuning time */
    currentGATE->start = MAX(lambdaFree[currentGATE->lambda], (earliest_schedule_time + onuAttrs[onuNum].tuningTime));
  }
  else
  {
    currentGATE->start = MAX(lambdaFree[currentGATE->lambda], (earliest_schedule_time));
  }
  
 	
  currentGATE->data_length = onuAttrs[onuNum].dataGrantLen*simParams.TIME_PER_BYTE;
  currentGATE->video_length = onuAttrs[onuNum].videoGrantLen*simParams.TIME_PER_BYTE;
  currentGATE->length = onuAttrs[onuNum].grantLen*simParams.TIME_PER_BYTE;
  
  currentGATE->data_grant = onuAttrs[onuNum].dataGrantLen;
  currentGATE->video_grant = onuAttrs[onuNum].videoGrantLen;
  currentGATE->grant = onuAttrs[onuNum].grantLen;
  
  
  /* update lambda free time */
  lambdaFree[currentGATE->lambda] = currentGATE->start + currentGATE->length + 64*simParams.TIME_PER_BYTE + simParams.PREAMBLE_IPG_TIME /* now we account for REPORT frame */ 
      + simParams.GUARD_TIME; /* As well as GUARD time */
  // printf("[%10.5e] OLT:Transmit an MPCP GATE packet to ONU #%d [lambda: %d, start: %e, length: %e, %d + 64 bytes]\n",simtime(),onuNum, currentGATE->lambda, currentGATE->start, currentGATE->length,(int)(currentGATE->length/simParams.TIME_PER_BYTE)/* - 64*/);
  // fflush(NULL);
  /* Send the GATE */
  send(onuAttrs[onuNum].grantMailbox[currentGATE->lambda], (long) currentGATE);

  if(simParams.GRANT_TRACE == GRANT_TRACE_ON)
  {
    grant_trace(onuNum,currentGATE);
  }
  /* Update ONU's ready time */
  onuAttrs[onuNum].readyTime = lambdaFree[currentGATE->lambda] - simParams.GUARD_TIME;

  // printf("ONU #%d ready at [%10.5e]\n",onuNum,onuAttrs[onuNum].readyTime);
  // printf("Lambda #%d free at [%10.5e]\n",currentGATE->lambda,lambdaFree[currentGATE->lambda]);
  // fflush(NULL);

  return 0;
}

/* 
 * FUNCTION: tentatively_schedule_onu()
 * DESCRIPTION: Utility function used to tentatively schedule an ONU with or without
 *              pre-selected lambda
 *
 */
int tentatively_schedule_onu(int onuNum, int lambdaNum)
{
  double earliest_schedule_time, earliest_gate_tx_time;

  /* 
   * Schedule this ONU for transmission
   */
  /* calculate earliest possible time to schedule this ONU */
  earliest_gate_tx_time = (downstreamFree > simtime()) ? downstreamFree : simtime();
  earliest_schedule_time = earliest_gate_tx_time + (64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME + onuAttrs[onuNum].rtt;
  downstreamFree = earliest_gate_tx_time + (64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME;

  // printf("[%10.5e] earliest_schedule_time for ONU #%d = [%10.5e]\n",simtime(),onuNum,earliest_schedule_time);
  // fflush(NULL);

  // printf("find best candidate lambda to issue grant for ONU #%d\n",onuNum);
  // fflush(NULL);
  if(lambdaNum == LAMBDA_NULL)
  {
    /* find best candidate lambda to issue grant */
    schedPool[onuNum].gateLambda = lambda_search_temp(onuNum, earliest_schedule_time);
  }
  else
  {
    /* Use pre-selected lambda */
    schedPool[onuNum].gateLambda = lambdaNum;
  }
  /* Setup the GATE */
  /* setup grant start time and length (does not include REPORT, REPORT kept seperate to keep out of utilization calc) */
  schedPool[onuNum].gateLength = onuAttrs[onuNum].grantLen*simParams.TIME_PER_BYTE;
  if((onuAttrs[onuNum].wdmType == WDM_TUNABLE) && (onuAttrs[onuNum].tunedLambda != schedPool[onuNum].gateLambda) && (lambdaFreeTemp[schedPool[onuNum].gateLambda] < (simtime() + onuAttrs[onuNum].tuningTime)))
  {
    /* need to account for some or all of the tuning time */
    schedPool[onuNum].gateStart = MAX(lambdaFreeTemp[schedPool[onuNum].gateLambda], (earliest_schedule_time + onuAttrs[onuNum].tuningTime));
  }
  else
  {
    schedPool[onuNum].gateStart = MAX(lambdaFreeTemp[schedPool[onuNum].gateLambda], (earliest_schedule_time));
  }
  /* update lambda free time */
  lambdaFreeTemp[schedPool[onuNum].gateLambda] = schedPool[onuNum].gateStart + schedPool[onuNum].gateLength + 64*simParams.TIME_PER_BYTE + simParams.PREAMBLE_IPG_TIME + simParams.GUARD_TIME; /* We account for REPORT frame as well as GUARD time */
  return 0;
}

/* 
 * FUNCTION: firmly_schedule_onu()
 * DESCRIPTION: Utility function used to firmly schedule an ONU after it has first been
 *              tentatively scheduled
 *
 */
int firmly_schedule_onu(int onuNum)
{
	sENTITY_GATE_PKT *currentGATE;

  /* 
     Send GATE message 
   */
  /* setup a new GATE message */
  currentGATE = create_a_gate();
  currentGATE->lambda = schedPool[onuNum].gateLambda;
  currentGATE->length = schedPool[onuNum].gateLength;
  currentGATE->start  = schedPool[onuNum].gateStart;

  /* update lambda free time */
  lambdaFree[currentGATE->lambda] = currentGATE->start + currentGATE->length + 64*simParams.TIME_PER_BYTE + simParams.PREAMBLE_IPG_TIME + simParams.GUARD_TIME; /* As well as GUARD time */

  // printf("[%10.5e] OLT:Transmit an MPCP GATE packet to ONU #%d [lambda: %d, start: %e, length: %e, %d + 64 bytes]\n",simtime(),onuNum, 
  //        currentGATE->lambda, currentGATE->start, currentGATE->length,
  //        (int)(currentGATE->length/simParams.TIME_PER_BYTE)/* - 64*/);

  /* Record REPORT to Schedule time for this ONU */
  if(onuAttrs[onuNum].prevGateTime != 0)
  {
    record((simtime() - onuAttrs[onuNum].rptTime),overallRptToSchedTime);
    if(onuNum < simParams.NUM_HEAVY_ONU)
    {
      record((simtime() - onuAttrs[onuNum].rptTime),heavyRptToSchedTime);
    }
    else
    {
      record((simtime() - onuAttrs[onuNum].rptTime),lightRptToSchedTime);
    }
  }

  /* Send the GATE */
  send(onuAttrs[onuNum].grantMailbox[currentGATE->lambda], (long) currentGATE);
  if(simParams.GRANT_TRACE == GRANT_TRACE_ON)
  {
    grant_trace(onuNum,currentGATE);
  }
  /* Update ONU's ready time */
  onuAttrs[onuNum].readyTime = lambdaFree[currentGATE->lambda] - simParams.GUARD_TIME;
  /* Remove ONU from scheduling pool */
  schedPool[onuNum].state = ONU_SCHED_INACTIVE;
  if(schedPoolCount > 0)
  {
    schedPoolCount--;
  }

//  printf("ONU #%d ready at [%10.5e]\n",onuNum,onuAttrs[onuNum].readyTime);
//  printf("Lambda #%d free at [%10.5e]\n",currentGATE->lambda,lambdaFree[currentGATE->lambda]);
//  fflush(NULL);

  return 0;
}


/* 
 * FUNCTION: online()
 * DESCRIPTION: Online OLT scheduler that employs Online (or one ONU at a time) scheduling
 *              Follows "scheduling jobs one at a time" online classification
 *
 */

void online()
{
	int pollONU;
	int loopIndex;
 	int grantCnt;
	double min_lambda_time,max_lambda_time;

  grantCnt = 0;
	
	/* issue GRANTs to ONUs as they need them */
	while(!terminateSim)
	{
    /*
     * After a total number of ONUs worth of grants record load balance measure
     */
    grantCnt++;
    if(grantCnt == simParams.NUM_ONU)
    {
      grantCnt = 0;
      min_lambda_time = 1e200;
      max_lambda_time = 0;
      for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
      {
        if(min_lambda_time > lambdaFree[loopIndex])
        {
          min_lambda_time = lambdaFree[loopIndex];
        }
        if(max_lambda_time < lambdaFree[loopIndex])
        {
          max_lambda_time = lambdaFree[loopIndex];
        }
      }
      /*
       * Record the the difference between minimum and maximum lambda available 
       * times as load balancing measure
       */
      record(max_lambda_time-min_lambda_time,loadBalanceMeasure);
    }

		/* 
		 * Determine which ONU to service 
		 */
		pollONU = 0;
		for(loopIndex = 0; loopIndex < simParams.NUM_ONU; loopIndex++)
		{
			if(onuAttrs[loopIndex].readyTime < onuAttrs[pollONU].readyTime)
			{
				pollONU = loopIndex;
			}
		}
		if(onuAttrs[pollONU].readyTime > simtime())
		{
			hold(onuAttrs[pollONU].readyTime-simtime());
		}
    // printf("[%10.5e] Scheduling ONU #%d\n",simtime(),pollONU);
    // fflush(NULL);
		
		
		/* 
		 * Determine grant size for this ONU
		 */
		if(simParams.OLT_TYPE != OLT_IPACT_PSF)
		{
     	grant_sizing(pollONU);
    }
        
    /* Record the number of ONUs available for scheduling */
    record(1,numONUSched);
    /* 
		 * Schedule this ONU for transmission
		 */
    schedule_onu(pollONU, LAMBDA_NULL);
	}
}


void onu()
{
	create("OLT");
	// printf("OLT started\n");

	// Test Variables
	status_processes_print();
	
	/* Permanent OLT behavior */
	while(!terminateSim)
	{
    online();
	}	
}


