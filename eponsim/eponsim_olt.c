/*
	TITLE: 		EPON Simulator OLT Models
	AUTHOR: 	Michael P. McGarry
	DATE:
	VERSION:	1.0
	
	NOTES:
	
*/

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
/* 
 * FUNCTION: lambda_search(onuNum,idealTime)
 * ARGUMENTS:
 *          int onuNum          - ONU Number to find wavelength for
 *          double idealTime    - ideal wavelength avail time
 * DESCRIPTION: Finds the wavelength supported by ONU onuNum that is available the closest to
 *              idealTime
 *
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


void forecast_grant_sizing(int onuNum, double start_time)
{	
	if(simParams.OLT_TYPE == OLT_IPACT_PSF)
	{
		if(preLoad != simParams.DESIRED_LOAD && simType == ACTUAL_RUN)
		{
			preLoad = simParams.DESIRED_LOAD;
			cycleNumber = 0;
		}
		if(start_time * simParams.FRAME_RATE >= cycleNumber)  // Fix this if statement when we have a shift in transmission (Important)!!!!!!!!!!!!!!!!!!!!
		{
			for(i=0; i < simParams.NUM_SS_STREAMS; i++)
			{
				onuAttrs[i].remainPSFgrant = simParams.MAX_GRANT_SIZE; //Limted Data - Limited Video
				onuAttrs[i].videoServiced = NOT_SERVICED;
			}
			cycleNumber = cycleNumber + 1;
		}
	}
	
	if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON)
	{
		if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_GATED)
		{	
			if(start_time >= onuAttrs[onuNum].rptForecastVideoTime)
			{
				onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize + onuAttrs[onuNum].rptForecastVideoQueueSize;
			}
			else
			{
				onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize;
			}
		}
		else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_FIXED)
		{
			onuAttrs[onuNum].videoGrantLen = simParams.FIXED_GRANT_SIZE;
		}
		else if(simParams.VIDEO_DBA_TYPE == VIDEO_DBA_LIMITED_GATE)
		{
			if(start_time >= onuAttrs[onuNum].rptForecastVideoTime)
			{
				if(onuAttrs[onuNum].rptVideoQueueSize + onuAttrs[onuNum].rptForecastVideoQueueSize <= simParams.MAX_GRANT_SIZE)
				{
					onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize + onuAttrs[onuNum].rptForecastVideoQueueSize;
				}
				else
				{
					onuAttrs[onuNum].videoGrantLen = simParams.MAX_GRANT_SIZE;
				}
			}
			else
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
	}
	else
	{
		onuAttrs[onuNum].videoGrantLen = 0;
	}
	
	x = start_time * simParams.FRAME_RATE;
	if(simParams.OLT_TYPE == OLT_IPACT_PSF && onuAttrs[onuNum].videoServiced == NOT_SERVICED)
	{
		onuAttrs[onuNum].grantLen = onuAttrs[onuNum].videoGrantLen;
		onuAttrs[onuNum].remainPSFgrant = onuAttrs[onuNum].remainPSFgrant - onuAttrs[onuNum].grantLen;
		onuAttrs[onuNum].dataGrantLen = 0;
		onuAttrs[onuNum].videoServiced = SERVICED;
	}
	else
	{	
		switch(simParams.DBA_TYPE)
		{
			case DBA_GATED:
				onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
				onuAttrs[onuNum].grantLen = onuAttrs[onuNum].dataGrantLen + onuAttrs[onuNum].videoGrantLen;
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
						if(start_time >= onuAttrs[onuNum].rptForecastVideoTime)
						{
							if(onuAttrs[onuNum].rptVideoQueueSize + onuAttrs[onuNum].rptForecastVideoQueueSize < simParams.FIXED_GRANT_SIZE)
							{
								onuAttrs[onuNum].videoGrantLen = onuAttrs[onuNum].rptVideoQueueSize + onuAttrs[onuNum].rptForecastVideoQueueSize;
								onuAttrs[onuNum].dataGrantLen = simParams.FIXED_GRANT_SIZE - onuAttrs[onuNum].videoGrantLen;
							}
							else
							{
								onuAttrs[onuNum].videoGrantLen = simParams.FIXED_GRANT_SIZE;
								onuAttrs[onuNum].dataGrantLen = 0;
							}
						}
						else
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
						if(simParams.OLT_TYPE == OLT_IPACT_PSF)
						{
							if(onuAttrs[onuNum].videoGrantLen < onuAttrs[onuNum].remainPSFgrant)
							{
								onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].remainPSFgrant - onuAttrs[onuNum].videoGrantLen;
								
								if(onuAttrs[onuNum].rptQueueSize <  onuAttrs[onuNum].dataGrantLen)
								{
									onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
								}
								
								onuAttrs[onuNum].remainPSFgrant = onuAttrs[onuNum].remainPSFgrant - onuAttrs[onuNum].dataGrantLen;
							}
							else
							{
								onuAttrs[onuNum].dataGrantLen = 0;
							}
							
							if(onuNum != (simParams.NUM_SS_STREAMS - 1))
							{
								onuAttrs[onuNum + 1].remainPSFgrant = onuAttrs[onuNum + 1].remainPSFgrant + onuAttrs[onuNum].remainPSFgrant;
								onuAttrs[onuNum].remainPSFgrant = 0;
							}
							else
							{
								onuAttrs[0].remainPSFgrant = onuAttrs[onuNum].remainPSFgrant;
								onuAttrs[onuNum].remainPSFgrant = 0;
							}
						}
						else
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
	}
	
	/*##########################################################################################################################################################

	if(start_time <= 0.404 && start_time >= 0.304)
	{
		printf("[%10.5e] ONU#%d, Start Time = %lf, Forecast Video Time = %lf, Video Grant Report = %ld, Video Grant = %ld \n",simtime() ,onuNum, start_time, onuAttrs[onuNum].rptForecastVideoTime, onuAttrs[onuNum].rptVideoQueueSize, onuAttrs[onuNum].videoGrantLen);
	}

	##########################################################################################################################################################*/
	
	/*//##########################################################################################################################################################

	printf("[%10.5e] ONU#%d, Grant Length = %ld \n",simtime() ,onuNum, onuAttrs[onuNum].grantLen);
	
	if(simtime() <= 9172.806350 && simtime() >= 9172.806100)
	{
		printf("[%10.5e] ONU#%d, Grant Length = %ld \n",simtime() ,onuNum, onuAttrs[onuNum].grantLen);
	}

	//##########################################################################################################################################################*/
		
	
	return;
}


void grant_sizing_PSF(int onuNum, double start_time)
{
	if(simParams.OLT_TYPE == OLT_IPACT_PSF || simType != ACTUAL_RUN)
	{
		if(preLoad != simParams.DESIRED_LOAD && simType == ACTUAL_RUN)
		{
			preLoad = simParams.DESIRED_LOAD;
			cycleNumber = 0;
		}
		if(start_time * simParams.FRAME_RATE >= cycleNumber)  // Fix this if statement when we have a shift in transmission (Important)!!!!!!!!!!!!!!!!!!!!
		{
			for(i=0; i < simParams.NUM_SS_STREAMS; i++)
			{
				onuAttrs[i].remainPSFgrant = simParams.MAX_GRANT_SIZE; //Limted Data - Limited Video
				if(simType == ACTUAL_RUN)
				{
					onuAttrs[i].videoServiced = NOT_SERVICED;
				}
				else
				{
					onuAttrs[i].videoServiced = SERVICED;
				}
			}
			cycleNumber = cycleNumber + 1;
		}
	}

	if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON && simType == ACTUAL_RUN)
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
	
	if(simParams.OLT_TYPE == OLT_IPACT_PSF && onuAttrs[onuNum].videoServiced == NOT_SERVICED /*&& simParams.VIDEO_PREDICTION == VIDEO_PREDICTION_OFF*/)
	{
		onuAttrs[onuNum].grantLen = onuAttrs[onuNum].videoGrantLen;
		onuAttrs[onuNum].videoServiced = SERVICED;
		onuAttrs[onuNum].dataGrantLen = 0;
		onuAttrs[onuNum].remainPSFgrant = onuAttrs[onuNum].remainPSFgrant - onuAttrs[onuNum].grantLen;
		//printf("ONU# %d Serviced, txf=%lf, cycle#=%ld\n",onuNum,x,cycleNumber);
	}
	else
	{
		switch(simParams.DBA_TYPE)
		{
			case DBA_GATED:
				onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
				onuAttrs[onuNum].grantLen = onuAttrs[onuNum].dataGrantLen + onuAttrs[onuNum].videoGrantLen;
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
				if(simParams.VIDEO_TRAFFIC == VIDEO_TRAFFIC_ON && simType == ACTUAL_RUN)
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
						if(simParams.OLT_TYPE == OLT_IPACT_PSF)
						{
							if(onuAttrs[onuNum].videoGrantLen < onuAttrs[onuNum].remainPSFgrant)
							{
								onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].remainPSFgrant - onuAttrs[onuNum].videoGrantLen;
								
								if(onuAttrs[onuNum].rptQueueSize <  onuAttrs[onuNum].dataGrantLen)
								{
									onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
								}
								
								onuAttrs[onuNum].remainPSFgrant = onuAttrs[onuNum].remainPSFgrant - onuAttrs[onuNum].dataGrantLen;
							}
							else
							{
								onuAttrs[onuNum].dataGrantLen = 0;
							}
							
							if(onuNum != (simParams.NUM_SS_STREAMS - 1))
							{
								onuAttrs[onuNum + 1].remainPSFgrant = onuAttrs[onuNum + 1].remainPSFgrant + onuAttrs[onuNum].remainPSFgrant;
								onuAttrs[onuNum].remainPSFgrant = 0;
							}
							else
							{
								onuAttrs[0].remainPSFgrant = onuAttrs[onuNum].remainPSFgrant;
								onuAttrs[onuNum].remainPSFgrant = 0;
							}
						}
					
						else
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
				}
				else
				{
					if(onuAttrs[onuNum].rptQueueSize < simParams.MAX_GRANT_SIZE)
					{
						onuAttrs[onuNum].dataGrantLen = onuAttrs[onuNum].rptQueueSize;
					}
					else
					{
						onuAttrs[onuNum].dataGrantLen = simParams.MAX_GRANT_SIZE;
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
	}
	
	//// Drop packets to match the availability of bandwidth
	//if (simParams.SCALABLE_VIDEO_TRAFFIC == SCALABLE_VIDEO_ON)
		//drop_scalable_video_packets(onuNum, onuAttrs[onuNum].videoGrantLen);	
	
	return;
}


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
			
			#ifdef DEBUG_ONU
			if (onuNum == 1 && simtime() >= 7200)	printf("OLT: D-Gate=%ld, Gate=%ld, ONU# %d\n",onuAttrs[onuNum].dataGrantLen, onuAttrs[onuNum].grantLen, onuNum);
			#endif
			
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
		
		
	//##########################################################################################################################################################
	
	if(simtime() <= 9172.806350 && simtime() >= 9172.805875)
	{
		printf("[%10.10e] ONU#%d, Grant Length = %.0f \n",simtime() ,onuNum, onuAttrs[onuNum].grantLen);
	}
	
	//##########################################################################################################################################################	
	
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

#ifdef DEBUG_TRC_LO
    printf("[%10.5e] earliest_schedule_time for ONU #%d = [%10.5e]\n",simtime(),onuNum,earliest_schedule_time);
    fflush(NULL);
#endif

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
        currentGATE->start = MAX(lambdaFree[currentGATE->lambda], 
                                                    (earliest_schedule_time + onuAttrs[onuNum].tuningTime));
    }
    else
    {
        currentGATE->start = MAX(lambdaFree[currentGATE->lambda], 
                                                        (earliest_schedule_time));
    }
    
    if(simParams.VIDEO_PREDICTION == VIDEO_PREDICTION_ON && simType == ACTUAL_RUN)
    {
    	forecast_grant_sizing(onuNum, currentGATE->start);
    }
    else if(simParams.OLT_TYPE == OLT_IPACT_PSF)
    {
    	grant_sizing_PSF(onuNum, currentGATE->start);
    }
    //else
    //{
    //	grant_sizing(onuNum);
    //}
    
    	
    currentGATE->data_length = onuAttrs[onuNum].dataGrantLen*simParams.TIME_PER_BYTE;
    currentGATE->video_length = onuAttrs[onuNum].videoGrantLen*simParams.TIME_PER_BYTE;
    currentGATE->length = onuAttrs[onuNum].grantLen*simParams.TIME_PER_BYTE;
    
    currentGATE->data_grant = onuAttrs[onuNum].dataGrantLen;
    currentGATE->video_grant = onuAttrs[onuNum].videoGrantLen;
    currentGATE->grant = onuAttrs[onuNum].grantLen;
    
    
    /* update lambda free time */
    lambdaFree[currentGATE->lambda] = currentGATE->start + currentGATE->length + 64*simParams.TIME_PER_BYTE + simParams.PREAMBLE_IPG_TIME /* now we account for REPORT frame */ 
        + simParams.GUARD_TIME; /* As well as GUARD time */
#ifdef DEBUG_TRC_LO
    printf("[%10.5e] OLT:Transmit an MPCP GATE packet to ONU #%d [lambda: %d, start: %e, length: %e, %d + 64 bytes]\n",simtime(),onuNum, 
           currentGATE->lambda, currentGATE->start, currentGATE->length,
           (int)(currentGATE->length/simParams.TIME_PER_BYTE)/* - 64*/);
    fflush(NULL);
#endif
    /* Send the GATE */
    send(onuAttrs[onuNum].grantMailbox[currentGATE->lambda], (long) currentGATE);

    if(simParams.GRANT_TRACE == GRANT_TRACE_ON)
    {
        grant_trace(onuNum,currentGATE);
    }
    /* Update ONU's ready time */
    onuAttrs[onuNum].readyTime = lambdaFree[currentGATE->lambda] - simParams.GUARD_TIME;

#ifdef DEBUG_TRC_HI
    printf("ONU #%d ready at [%10.5e]\n",onuNum,onuAttrs[onuNum].readyTime);
    printf("Lambda #%d free at [%10.5e]\n",currentGATE->lambda,lambdaFree[currentGATE->lambda]);
    fflush(NULL);
#endif

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

#ifdef DEBUG_TRC_LO
    printf("[%10.5e] earliest_schedule_time for ONU #%d = [%10.5e]\n",simtime(),onuNum,earliest_schedule_time);
    fflush(NULL);
#endif

#ifdef DEBUG_TRC_HI
    printf("find best candidate lambda to issue grant for ONU #%d\n",onuNum);
    fflush(NULL);
#endif
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
    if((onuAttrs[onuNum].wdmType == WDM_TUNABLE) && 
            (onuAttrs[onuNum].tunedLambda != schedPool[onuNum].gateLambda) && 
            (lambdaFreeTemp[schedPool[onuNum].gateLambda] < (simtime() + onuAttrs[onuNum].tuningTime)))
    {
        /* need to account for some or all of the tuning time */
        schedPool[onuNum].gateStart = MAX(lambdaFreeTemp[schedPool[onuNum].gateLambda], 
                                                    (earliest_schedule_time + onuAttrs[onuNum].tuningTime));
    }
    else
    {
        schedPool[onuNum].gateStart = MAX(lambdaFreeTemp[schedPool[onuNum].gateLambda], 
                                                        (earliest_schedule_time));
    }
    /* update lambda free time */
    lambdaFreeTemp[schedPool[onuNum].gateLambda] = schedPool[onuNum].gateStart + schedPool[onuNum].gateLength + 64*simParams.TIME_PER_BYTE + simParams.PREAMBLE_IPG_TIME /* now we account for REPORT frame */ 
        + simParams.GUARD_TIME; /* As well as GUARD time */
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
    lambdaFree[currentGATE->lambda] = currentGATE->start + currentGATE->length + 64*simParams.TIME_PER_BYTE + simParams.PREAMBLE_IPG_TIME /* now we account for REPORT frame */ 
        + simParams.GUARD_TIME; /* As well as GUARD time */

#ifdef DEBUG_TRC_LO
    printf("[%10.5e] OLT:Transmit an MPCP GATE packet to ONU #%d [lambda: %d, start: %e, length: %e, %d + 64 bytes]\n",simtime(),onuNum, 
           currentGATE->lambda, currentGATE->start, currentGATE->length,
           (int)(currentGATE->length/simParams.TIME_PER_BYTE)/* - 64*/);
#endif

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

#ifdef DEBUG_TRC_HI
    printf("ONU #%d ready at [%10.5e]\n",onuNum,onuAttrs[onuNum].readyTime);
    printf("Lambda #%d free at [%10.5e]\n",currentGATE->lambda,lambdaFree[currentGATE->lambda]);
    fflush(NULL);
#endif

    return 0;
}

/* 
 * FUNCTION: offline()
 * DESCRIPTION: Offline OLT scheduler
 *
 */
void offline()
{
	int pollONU;
	int loopIndex,loopIndex2,lambdaNum,posNum;
	double latestTime;	/* used to keep track of latest ONU transmission */
	double earliest_sched_time;
	// double	latest_gate_time[MAX_ONU];
    int	onuList[MAX_ONU], onuListCnt;
    sONU_LIST onuListItem;
	int **cost_matrix;
    hungarian_problem_t myHungProb;
    int avail_cost;
	int colNum;
    int lambdaPerm[MAX_LAMBDAS];
    /* For new cycle time calculation */
    static double prevScheduleTime = 0.0;
    /* For competitive ratio calculation */
    unsigned long grantSum;
    double grantSumTime, propGrant, maxPropGrant, cr1, cr2, mcr;

    /* Initialize data structures, should possibly move these to global area */
    for(loopIndex = 0; loopIndex < MAX_ONU; loopIndex++)
    {
        // latest_gate_time[loopIndex] = 0;
        onuList[loopIndex] = -1;
    }

    /* 
    * Determine dimensions of cost matrix given ONUs 
    */
    for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        onuList[loopIndex] = loopIndex;
    }
    for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
    {
        lambdaPerm[loopIndex] = loopIndex;
    }
    /* Initialize the cost matrix */
    cost_matrix = (int**)calloc(simParams.NUM_ONU,sizeof(int*));
    for(loopIndex = 0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        cost_matrix[loopIndex] = (int*)calloc(simParams.NUM_ONU*simParams.NUM_LAMBDAS,sizeof(int));
    }
	
	/* issue GRANTs to ONUs as they need them */
	while(!terminateSim)
	{
#ifdef DEBUG_TRC_LO
        printf("\n\n[%10.5e] Offline OLT scheduler\n",simtime());
        fflush(NULL);
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            printf("ONU #%d ready at [%10.5e]\n",loopIndex,onuAttrs[loopIndex].readyTime);
        }
        fflush(NULL);
        for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
        {
            printf("Lambda #%d free at [%10.5e]\n",loopIndex,lambdaFree[loopIndex]);
        }
        fflush(NULL);
#endif
        /* Record cycle length */
        if(prevScheduleTime > 0.0)
        {
            record(simtime() - prevScheduleTime, overallCycleLength);
        }
        prevScheduleTime = simtime();
        /* Record queue delay from cycle observation */
        if(table_cnt(cycleQueueDelay) > 0)
        {
            record(table_mean(cycleQueueDelay),queueDelay);
        }
        reset_table(cycleQueueDelay);

        /* 
         * Grant Sizing
         */
        
        /* Code added for competitive ratio calculation */
        grantSum = 0; maxPropGrant = 0;
        /* Initialize excess bandwidth distribution variables */
        excessBW = 0; numOverloaded = 0; overloadedONUList = NULL;
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            /* 
             * Determine Bandwidth grant for this ONU
             */
            grant_sizing(loopIndex);
            grantSum += onuAttrs[loopIndex].grantLen + 64 + PREAMBLE_IPG_BYTES;
            propGrant = (2*(onuAttrs[loopIndex].latency) + ((onuAttrs[loopIndex].grantLen + 64 + PREAMBLE_IPG_BYTES)*simParams.TIME_PER_BYTE));
            if(propGrant > maxPropGrant)
            {
                maxPropGrant = propGrant;
            }
        }
        
        /* Record excess bandwidth */
        record(excessBW,excessBandwidth);
        
        /* Divide and allocate excess bandwidth */
        if(simParams.DBA_TYPE == DBA_EXCESS)
    	{
        	grant_excess();
        }
        
        /*
         * Completed Grant Sizing
         */
        
        /* Record competitive ratios */
        grantSumTime = grantSum*simParams.TIME_PER_BYTE;
        cr1 = (0.672e-6 + 2*simParams.ACTUAL_MAX_PROP_DELAY + grantSumTime)/(0.672e-6 + 2*6.68e-6 + grantSumTime);
        record(cr1,compRatio1);
        cr2 = (0.672e-6 + 2*simParams.ACTUAL_MAX_PROP_DELAY + grantSumTime)/(0.672e-6 + maxPropGrant);
        record(cr2,compRatio2);
        mcr = MIN(cr1, cr2);
        record(mcr,minCompRatio);
        
        /*
         * Grant Scheduling
         */
        
        /* Immediately schedule ONUs that deserve preferrential treatment */
        if(simParams.NUM_PREFERRED_ONU > 0)
        {
            for(loopIndex=0; loopIndex < simParams.NUM_PREFERRED_ONU; loopIndex++)
            {
                /* schedule this ONU's transmission */
                schedule_onu(loopIndex,LAMBDA_NULL);
            }
        }
        
        switch(simParams.OLT_TYPE)
        {
        case OLT_LFJ:
        case OLT_LFJ_SPT:
        case OLT_LFJ_LPT:
        case OLT_LFJ_LNF:
        case OLT_EAF:
        case OLT_EAAF:
        case OLT_SPD:
        case OLT_LPD:
            /* Sort the ONUs */
            for(loopIndex=0; loopIndex < MAX_LAMBDAS; loopIndex++)
            {
                scheduleList[loopIndex] = NULL;
            }
            for(loopIndex=simParams.NUM_PREFERRED_ONU; loopIndex < simParams.NUM_ONU; loopIndex++)
            {
                pollONU = loopIndex;
                /* Fill out ONU list record for ONU */
                onuListItem.onuNum = pollONU;
                onuListItem.grantLen = onuAttrs[loopIndex].grantLen;
                onuListItem.poolTime = 0;
                onuListItem.numFrames = onuAttrs[loopIndex].rptQueueNum;
                onuListItem.minArrivalTime = onuAttrs[loopIndex].minArrivalTime;
                onuListItem.avgArrivalTime = onuAttrs[loopIndex].avgArrivalTime;
                onuListItem.latency = onuAttrs[loopIndex].latency;
                /* Place ONU into sorted list */
                if(simParams.OLT_TYPE == OLT_LFJ)
                {
                    scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                        onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_ONU_NUM,SORT_ONU_NUM,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
                }
                if(simParams.OLT_TYPE == OLT_LFJ_SPT)
                {
                    scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                        onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_LEN,SORT_GRANT_LEN,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
                }
                if(simParams.OLT_TYPE == OLT_LFJ_LPT)
                {
                    scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                        onu_list_insert(SORT_DESCENDING_ORDER,SORT_DESCENDING_ORDER,SORT_GRANT_LEN,SORT_GRANT_LEN,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
                }
                if(simParams.OLT_TYPE == OLT_LFJ_LNF)
                {
                    scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                        onu_list_insert(SORT_DESCENDING_ORDER,SORT_DESCENDING_ORDER,SORT_NUM_FRAMES,SORT_NUM_FRAMES,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
                }
                if(simParams.OLT_TYPE == OLT_EAF)
                {
                    scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                        onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_MIN_ARRIVAL,SORT_MIN_ARRIVAL,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
                }
                if(simParams.OLT_TYPE == OLT_EAAF)
                {
                    scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                        onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_AVG_ARRIVAL,SORT_AVG_ARRIVAL,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
                }
                if(simParams.OLT_TYPE == OLT_SPD)
                {
                    scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                        onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_PROP_DELAY,SORT_ONU_NUM,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
                }
                if(simParams.OLT_TYPE == OLT_LPD)
                {
                    scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                        onu_list_insert(SORT_DESCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_PROP_DELAY,SORT_ONU_NUM,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
                }
                if(simParams.OLT_TYPE == OLT_SPD_LPT)
                {
                    scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                        onu_list_insert(SORT_ASCENDING_ORDER,SORT_DESCENDING_ORDER,SORT_PROP_DELAY,SORT_GRANT_LEN,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
                }
            }
    
#ifdef DEBUG_TRC
        printf("Ordered List of ONUs in Scheduling Pool\n");
        for(loopIndex = 0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
        {
            printf("Supported Lambdas = %d\n",loopIndex);
            onu_list_print(scheduleList[loopIndex]);
            fflush(NULL);
        }
#endif
    
#ifdef DEBUG_TRC
                    printf("Build linear schedule list\n");
                    fflush(NULL);
#endif
            /* Build linear schedule list */
            onuListCnt = 0;
            for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
            {
                while(scheduleList[loopIndex] != NULL)
                {
                    scheduleList[loopIndex] = onu_list_pop(scheduleList[loopIndex],&onuListItem);
                    onuList[onuListCnt++] = onuListItem.onuNum;
#ifdef DEBUG_TRC
                    printf("ONU #%d, Grant Len = %ld\n", onuListItem.onuNum, onuAttrs[onuListItem.onuNum].grantLen);
                    fflush(NULL);
#endif
                }
            }
    
            /* Record the number of ONUs available for scheduling */
            record(simParams.NUM_ONU,numONUSched);
    
#ifdef DEBUG_TRC_HI
                printf("%d ONUs to be dispatched\n", onuListCnt);
                fflush(NULL);
#endif

            /* Schedule the available ONUs according to generated list */
            for(loopIndex=0; loopIndex < onuListCnt; loopIndex++)
            {
                pollONU = onuList[loopIndex];
                /* 
                 * Schedule this ONU for transmission
                 */
                /* Update all Lambdas with available time < current time to current time */
                for(loopIndex2 = 0; loopIndex2 < simParams.NUM_LAMBDAS; loopIndex2++)
                {
                    if(lambdaFree[loopIndex2] < simtime())
                    {
                        lambdaFree[loopIndex2] = simtime();
                    }
                }
#ifdef DEBUG_TRC_HI
                    printf("Dispatch ONU #%d for first fit wavelength assignment\n", pollONU);
                    fflush(NULL);
#endif

                /* Schedule the ONU */
                schedule_onu(pollONU, LAMBDA_NULL);
            }
            break;

        case OLT_WBM:
            /*
             * Randomly Permute the ONU list and a list of lambdas
             */
            rand_permute_int(onuList, (simParams.NUM_ONU - simParams.NUM_PREFERRED_ONU));
            rand_permute_int(lambdaPerm, simParams.NUM_LAMBDAS);
            /* 
             * Construct cost matrix
             */
            for(loopIndex=simParams.NUM_PREFERRED_ONU; loopIndex < simParams.NUM_ONU; loopIndex++)
            {
                /* calculate earliest schedule time for this ONU */
                earliest_sched_time = simtime()+onuAttrs[onuList[(loopIndex-simParams.NUM_PREFERRED_ONU)]].rtt +(64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME;
                /* Prepare WBM matrix for this ONU */
                for(lambdaNum=0;lambdaNum < simParams.NUM_LAMBDAS; lambdaNum++)
                {
                    /* Compute availability cost for this lambda and ONU combination */
                    avail_cost = abs((lambdaFree[lambdaPerm[lambdaNum]]*(1/simParams.TIME_PER_BYTE)) - 
                                     (earliest_sched_time*(1/simParams.TIME_PER_BYTE)));
                    /* Normalize the avail_cost by 64 */
                    avail_cost = simParams.AVAIL_COST_WEIGHT*(avail_cost/64);
#ifdef DEBUG_TRC_HI
                    printf("avail_cost:ONU#%2d:Lam#%d = [%10.5e]:%d\n",loopIndex,lambdaNum,(lambdaFree[lambdaPerm[lambdaNum]] - earliest_sched_time),avail_cost);
                    fflush(NULL);
#endif
                    for(posNum=0;posNum < onuListCnt; posNum++)
                    {
                        if(onuAttrs[onuList[(loopIndex-simParams.NUM_PREFERRED_ONU)]].supportedLambdasMap[lambdaPerm[lambdaNum]] == LAMBDA_FALSE)
                        {
                            /* lambda not supported */
                            cost_matrix[(loopIndex-simParams.NUM_PREFERRED_ONU)][lambdaNum*simParams.NUM_ONU+posNum] = COST_INFINITE;
                        }
                        else
                        {
                            /* lambda supported */
                            if(onuAttrs[onuList[(loopIndex-simParams.NUM_PREFERRED_ONU)]].grantLen > 0)
                            {
                                /* never allow the cost to be higher than a number slightly smaller than COST_INFINITE */
                                /* also, normalize the cost by 64 */
                                cost_matrix[(loopIndex-simParams.NUM_PREFERRED_ONU)][lambdaNum*simParams.NUM_ONU+posNum] = int_min(((simParams.NUM_ONU-(posNum))*(onuAttrs[onuList[(loopIndex-simParams.NUM_PREFERRED_ONU)]].grantLen/64+1) 
                                    + avail_cost),(COST_INFINITE-100));
                            }
                            else
                            {
                                cost_matrix[(loopIndex-simParams.NUM_PREFERRED_ONU)][lambdaNum*simParams.NUM_ONU+posNum] = (simParams.NUM_ONU-(posNum)) + 
                                    avail_cost /* priority cost */;
                            }
                        }
                    }
                }
            }
    
#ifdef DEBUG_TRC
            debug_print_cost_matrix(cost_matrix,(simParams.NUM_ONU-simParams.NUM_PREFERRED_ONU),(simParams.NUM_ONU-simParams.NUM_PREFERRED_ONU)*simParams.NUM_LAMBDAS);
#endif
            /* dump_cost_matrix(); */
            /* Solve weighted bipartite matching */
#ifdef DEBUG_TRC
            printf("Solve Matching\n");
    		fflush(NULL);
#endif
    
            hungarian_init(&myHungProb,cost_matrix,(simParams.NUM_ONU-simParams.NUM_PREFERRED_ONU),(simParams.NUM_ONU-simParams.NUM_PREFERRED_ONU)*simParams.NUM_LAMBDAS,HUNGARIAN_MODE_MINIMIZE_COST);
    		hungarian_solve(&myHungProb);
    
#ifdef DEBUG_TRC_HI
            printf("WBM Solved\n");
    		fflush(NULL);
#endif
    
            /* 
             * Turn WBM solution into a tentative schedule 
             */
            /* Turn hungarian_solution structure into scheduleList structure */
            for(loopIndex=simParams.NUM_PREFERRED_ONU;loopIndex < simParams.NUM_ONU;loopIndex++)
            {
                colNum    = hungarian_assignment(&myHungProb,(loopIndex-simParams.NUM_PREFERRED_ONU));
                lambdaNum = lambdaPerm[colNum / (simParams.NUM_ONU-simParams.NUM_PREFERRED_ONU)];
                posNum    = colNum % (simParams.NUM_ONU-simParams.NUM_PREFERRED_ONU);
    
#ifdef DEBUG_TRC_HI
    			printf("Assigning Pos %d on Lambda %d to ONU %d:%ld\n", posNum, lambdaNum, onuList[(loopIndex-simParams.NUM_PREFERRED_ONU)], onuAttrs[onuList[(loopIndex-simParams.NUM_PREFERRED_ONU)]].grantLen);
    			fflush(NULL);
#endif
                onuListItem.onuNum = onuList[(loopIndex-simParams.NUM_PREFERRED_ONU)];
                onuListItem.posNum = posNum;
                onuListItem.grantLen = onuAttrs[onuListItem.onuNum].grantLen;
                onuListItem.numFrames = onuAttrs[onuListItem.onuNum].rptQueueNum;
                onuListItem.grantTime = 0;
                onuListItem.poolTime = schedPool[onuListItem.onuNum].poolTime;
                switch(simParams.OLT_TYPE)
                {
                case OLT_WBM:
                    scheduleList[lambdaNum] = onu_list_insert(SORT_ASCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_GRANT_LEN, SORT_POOL_TIME, &onuListItem, scheduleList[lambdaNum]);
                    break;
                case OLT_WBM_LPT:
                    scheduleList[lambdaNum] = onu_list_insert(SORT_DESCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_GRANT_LEN, SORT_POOL_TIME, &onuListItem, scheduleList[lambdaNum]);
                    break;
                case OLT_WBM_LNF:
                    scheduleList[lambdaNum] = onu_list_insert(SORT_DESCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_NUM_FRAMES, SORT_POOL_TIME, &onuListItem, scheduleList[lambdaNum]);
                    break;
                case OLT_WBM_EAAF:
                    scheduleList[lambdaNum] = onu_list_insert(SORT_ASCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_AVG_ARRIVAL, SORT_POOL_TIME, &onuListItem, scheduleList[lambdaNum]);
                    break;
    
                }
            }
#ifdef DEBUG_TRC_LO
    		printf("WBM Schedule\n");
    		for(loopIndex=0;loopIndex<simParams.NUM_LAMBDAS;loopIndex++)
    		{
    			printf("Lambda #%d\n",loopIndex);
    			fflush(NULL);
    			onu_list_print(scheduleList[loopIndex]);
    			fflush(NULL);
    		}
#endif
            /* Schedule the ONUs */
            for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
            {
                while(scheduleList[loopIndex] != NULL)
                {
                    scheduleList[loopIndex] = onu_list_pop(scheduleList[loopIndex],&onuListItem);
                    /* Check if the wavelength assignment is valid */
                    if(onuAttrs[onuListItem.onuNum].supportedLambdasMap[loopIndex] == LAMBDA_TRUE)
                    {
                        /* schedule this ONU on the selected wavelength */
                        schedule_onu(onuListItem.onuNum,loopIndex);
                    }
                    else
                    {
                        /* schedule this ONU on supported channel */
                        schedule_onu(onuListItem.onuNum,LAMBDA_NULL);
                    }
                }
            }
            hungarian_free(&myHungProb);
            break;

        default:
            break;
        }

        /* Wait for last ONU to finish transmission */
        latestTime = simtime();
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            if(latestTime < onuAttrs[loopIndex].readyTime)
            {
                latestTime = onuAttrs[loopIndex].readyTime;
            }
        }
        if(latestTime > simtime())
        {
            hold((latestTime - simtime()));
        }
	}
    /* Free cost_matrix */
    for(loopIndex = 0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        free(cost_matrix[loopIndex]);
    }
    free(cost_matrix);
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
#ifdef DEBUG_TRC_LO
        printf("[%10.5e] Scheduling ONU #%d\n",simtime(),pollONU);
        fflush(NULL);
#endif
		//~ TSprint("[%10.5e] Scheduling ONU #%d\n",simtime(),pollONU);
		
		//// New!     4/30/2017
		//int onuNum_tempvar;
		//for(onuNum_tempvar = 0; onuNum_tempvar < simParams.NUM_ONU; onuNum_tempvar++) {
			//// Check queue lengths for excessive packet numbers
			//check_video_packet_list(onuNum_tempvar);
			//check_data_packet_list(onuNum_tempvar);

			////// Drop Scalable Video Packets as necessary
			////if (simParams.SCALABLE_VIDEO_TRAFFIC == SCALABLE_VIDEO_ON) {
				////drop_scalable_video_packets(onuNum_tempvar, 0);
			////}
		//}
		
		
		
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

#if 0
/* 
 * FUNCTION: online_offline_excess()
 * DESCRIPTION: 
 *
 */
void online_offline_excess()
{
	int pollONU;
	int loopIndex;
	double min_lambda_time,max_lambda_time;
    unsigned long grantExcess, individualExcess;

    onuListCnt = 0;
    reportsReceived = 0;
	
	/* issue GRANTs to ONUs as they need them */
	while(!terminateSim)
	{
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
#ifdef DEBUG_TRC_LO
        printf("[%10.5e] Scheduling ONU #%d\n",simtime(),pollONU);
        fflush(NULL);
#endif

        reportsReceived |= 0x1 << pollONU;
		/* 
		 * Classify this ONU underloaded or overloaded
		 */
        if(onuAttrs[pollONU].rptQueueSize > simParams.MAX_GRANT_SIZE)
        {
            /* Overloaded ONU */
            /* Place in a list of overloaded ONUs waiting for a grant */
            onuList[onuListCnt++] = pollONU;
            onuAttrs[pollONU].grantLen = simParams.MAX_GRANT_SIZE;
        }
        else
        {
            /* Underloaded ONU */
            onuAttrs[pollONU].grantLen = onuAttrs[pollONU].rptQueueSize;
            /* Contribute to excess */
            grantExcess = simParams.MAX_GRANT_SIZE - onuAttrs[pollONU].grantLen;
            /* Record the number of ONUs available for scheduling */
            record(1,numONUSched);
            /* 
             * Schedule this ONU for transmission
             */
            schedule_onu(pollONU, LAMBDA_NULL);
        }
        /* Determine if all ONU REPORTs have been received */
        if(reportsReceived == ((0x1 << simParams.NUM_ONU)-1))
        {
            /* Record the number of ONUs available for scheduling */
            record(onuListCnt,numONUSched);
            /* Distribute the excess among the overloaded ONUs */
            individualExcess = grantExcess/onuListCnt;
            /* Schedule these ONUs */
            for(loopIndex=0; loopIndex < onuListCnt; loopIndex++)
            {
                onuAttrs[onuList[loopIndex]].grantLen += individualExcess;
                /* 
                 * Schedule this ONU for transmission
                 */
                schedule_onu(onuList[loopIndex], LAMBDA_NULL);
            }
            onuListCnt = 0;
            reportsReceived = 0;
        }
	}
}
#endif


/* 
 * FUNCTION: online_jit2()
 * DESCRIPTION: Online JIT 2-step scheduler OLT scheduler
 *
 */
void online_jit2()
{
	int pollONU;
	int loopIndex,loopIndex2;
	double latestTime;	/* used to keep track of latest ONU transmission */
	// double latest_gate_time[MAX_ONU];
    double next_lambda_time; /* used to determine the available time of the next available wavelength */
    int	onuList[MAX_ONU], onuListCnt;
    sONU_LIST onuListItem;
    /* For new cycle time calculation */
    static double prevScheduleTime = 0.0;

    /* Initialize data structures, should possibly move these to global area */
    for(loopIndex = 0; loopIndex < MAX_ONU; loopIndex++)
    {
        // latest_gate_time[loopIndex] = 0;
        onuList[loopIndex] = -1;
    }
	
	/* issue GRANTs to ONUs as they need them */
	while(!terminateSim)
	{
#ifdef DEBUG_TRC_LO
        printf("\n\n[%10.5e] Offline OLT scheduler\n",simtime());
        fflush(NULL);
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            printf("ONU #%d ready at [%10.5e]\n",loopIndex,onuAttrs[loopIndex].readyTime);
        }
        fflush(NULL);
        for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
        {
            printf("Lambda #%d free at [%10.5e]\n",loopIndex,lambdaFree[loopIndex]);
        }
        fflush(NULL);
#endif
        /* Record cycle length */
        if(prevScheduleTime > 0.0)
        {
            record(simtime() - prevScheduleTime, overallCycleLength);
        }
        prevScheduleTime = simtime();
        /* Record queue delay from cycle observation */
        if(table_cnt(cycleQueueDelay) > 0)
        {
            record(table_mean(cycleQueueDelay),queueDelay);
        }
        reset_table(cycleQueueDelay);

        /*
         *
         * Step 1: Schedule ONUs whose REPORTs have been received, prior to wavelength availability
         *
         *
         */

        /* 
         * Grant Sizing
         */
        
        /* Initialize excess bandwidth distribution variables */
        excessBW = 0; numOverloaded = 0; overloadedONUList = NULL;
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            /* Skip over ONUs that are not ready and mark them for step 2 */
            if(onuAttrs[loopIndex].readyTime > simtime())
            {
                /* Scheduled in step 2 */
                onuAttrs[loopIndex].step2Sched = 1;
                continue;
            }
            else
            {
                /* Scheduled in step 1 */
                onuAttrs[loopIndex].step2Sched = 0;
            }
            /* 
             * Determine Bandwidth grant for this ONU
             */
            grant_sizing(loopIndex);
        }
        
        /* Record excess bandwidth */
        record(excessBW,excessBandwidth);
        
        /* Divide and allocate excess bandwidth */
        if(simParams.DBA_TYPE == DBA_EXCESS)
    	{
        	grant_excess();
        }
        
        /*
         * Completed Grant Sizing
         */
                
        /*
         * Grant Scheduling
         */
        
        /* Immediately schedule ONUs that deserve preferrential treatment */
        if(simParams.NUM_PREFERRED_ONU > 0)
        {
            for(loopIndex=0; loopIndex < simParams.NUM_PREFERRED_ONU; loopIndex++)
            {
                /* Skip over ONUs that are not ready */
                if(onuAttrs[loopIndex].readyTime > simtime())
                {
                    continue;
                }
                /* schedule this ONU's transmission */
                schedule_onu(loopIndex,LAMBDA_NULL);
            }
        }
        
        /* Sort the ONUs */
        for(loopIndex=0; loopIndex < MAX_LAMBDAS; loopIndex++)
        {
            scheduleList[loopIndex] = NULL;
        }
        for(loopIndex=simParams.NUM_PREFERRED_ONU; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            /* Skip over ONUs that are not ready */
            if(onuAttrs[loopIndex].readyTime > simtime())
            {
                continue;
            }
            pollONU = loopIndex;
            /* Fill out ONU list record for ONU */
            onuListItem.onuNum = pollONU;
            onuListItem.grantLen = onuAttrs[loopIndex].grantLen;
            onuListItem.poolTime = 0;
            onuListItem.numFrames = onuAttrs[loopIndex].rptQueueNum;
            onuListItem.minArrivalTime = onuAttrs[loopIndex].minArrivalTime;
            onuListItem.avgArrivalTime = onuAttrs[loopIndex].avgArrivalTime;
            onuListItem.latency = onuAttrs[loopIndex].latency;
            /* Place ONU into sorted list */
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_LFJ_SPT)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_LEN,SORT_GRANT_LEN,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_LFJ_LPT)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_DESCENDING_ORDER,SORT_DESCENDING_ORDER,SORT_GRANT_LEN,SORT_GRANT_LEN,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_LFJ_LNF)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_DESCENDING_ORDER,SORT_DESCENDING_ORDER,SORT_NUM_FRAMES,SORT_NUM_FRAMES,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_EAF)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_MIN_ARRIVAL,SORT_MIN_ARRIVAL,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_EAAF)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_AVG_ARRIVAL,SORT_AVG_ARRIVAL,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_SPD)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_PROP_DELAY,SORT_ONU_NUM,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_LPD)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_DESCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_PROP_DELAY,SORT_ONU_NUM,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
        }
    
#ifdef DEBUG_TRC
        printf("Ordered List of ONUs in Scheduling Pool\n");
        for(loopIndex = 0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
        {
            printf("Supported Lambdas = %d\n",loopIndex);
            onu_list_print(scheduleList[loopIndex]);
            fflush(NULL);
        }
#endif
    
#ifdef DEBUG_TRC
        printf("Build linear schedule list\n");
        fflush(NULL);
#endif
        /* Build linear schedule list */
        onuListCnt = 0;
        for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
        {
            while(scheduleList[loopIndex] != NULL)
            {
                scheduleList[loopIndex] = onu_list_pop(scheduleList[loopIndex],&onuListItem);
                onuList[onuListCnt++] = onuListItem.onuNum;
#ifdef DEBUG_TRC
                printf("ONU #%d, Grant Len = %ld\n", onuListItem.onuNum, onuAttrs[onuListItem.onuNum].grantLen);
                fflush(NULL);
#endif
            }
        }

        /* Record the number of ONUs available for scheduling */
        record(onuListCnt,numONUSched);
    
#ifdef DEBUG_TRC_HI
        printf("%d ONUs to be dispatched\n", onuListCnt);
        fflush(NULL);
#endif

        /* Schedule the available ONUs according to generated list */
        for(loopIndex=0; loopIndex < onuListCnt; loopIndex++)
        {
            pollONU = onuList[loopIndex];
            /* 
             * Schedule this ONU for transmission
             */
            /* Update all Lambdas with available time < current time to current time */
            for(loopIndex2 = 0; loopIndex2 < simParams.NUM_LAMBDAS; loopIndex2++)
            {
                if(lambdaFree[loopIndex2] < simtime())
                {
                    lambdaFree[loopIndex2] = simtime();
                }
            }
#ifdef DEBUG_TRC_HI
            printf("Dispatch ONU #%d for first fit wavelength assignment\n", pollONU);
            fflush(NULL);
#endif
            /* Schedule the ONU */
            schedule_onu(pollONU, LAMBDA_NULL);
        }



        /*
         *
         * Step 2: Schedule ONUs whose REPORTs were NOT received prior to wavelength availability
         *
         *
         */

        /*
         * Wait for last step 2 ONU to become ready
         *
         */
        latestTime = simtime();
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            /* Skip over ONUs that were part of step 1 */
            if(onuAttrs[loopIndex].step2Sched == 0)
            {
                continue;
            }
            if(onuAttrs[loopIndex].readyTime > latestTime)
            {
                latestTime = onuAttrs[loopIndex].readyTime;
            }
        }
        if(latestTime > simtime())
        {
            hold(latestTime - simtime());
        }

        /* 
         * Grant Sizing
         */
        
        /* Initialize excess bandwidth distribution variables */
        excessBW = 0; numOverloaded = 0; overloadedONUList = NULL;
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            /* Skip over ONUs that were part of step 1 */
            if(onuAttrs[loopIndex].step2Sched == 0)
            {
                continue;
            }

            /* 
             * Determine Bandwidth grant for this ONU
             */
            grant_sizing(loopIndex);
        }
        
        /* Record excess bandwidth */
        record(excessBW,excessBandwidth);
        
        /* Divide and allocate excess bandwidth */
        if(simParams.DBA_TYPE == DBA_EXCESS)
    	{
        	grant_excess();
        }
        
        /*
         * Completed Grant Sizing
         */
        
        /*
         * Grant Scheduling
         */
        
        /* Immediately schedule ONUs that deserve preferrential treatment */
        if(simParams.NUM_PREFERRED_ONU > 0)
        {
            for(loopIndex=0; loopIndex < simParams.NUM_PREFERRED_ONU; loopIndex++)
            {
                /* Skip over ONUs that were part of step 1 */
                if(onuAttrs[loopIndex].step2Sched == 0)
                {
                    continue;
                }
                /* schedule this ONU's transmission */
                schedule_onu(loopIndex,LAMBDA_NULL);
            }
        }
        
        /* Sort the ONUs */
        for(loopIndex=0; loopIndex < MAX_LAMBDAS; loopIndex++)
        {
            scheduleList[loopIndex] = NULL;
        }
        for(loopIndex=simParams.NUM_PREFERRED_ONU; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            /* Skip over ONUs that were part of step 1 */
            if(onuAttrs[loopIndex].step2Sched == 0)
            {
                continue;
            }
            pollONU = loopIndex;
            /* Fill out ONU list record for ONU */
            onuListItem.onuNum = pollONU;
            onuListItem.grantLen = onuAttrs[loopIndex].grantLen;
            onuListItem.poolTime = 0;
            onuListItem.numFrames = onuAttrs[loopIndex].rptQueueNum;
            onuListItem.minArrivalTime = onuAttrs[loopIndex].minArrivalTime;
            onuListItem.avgArrivalTime = onuAttrs[loopIndex].avgArrivalTime;
            onuListItem.latency = onuAttrs[loopIndex].latency;
            /* Place ONU into sorted list */
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_LFJ_SPT)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_LEN,SORT_GRANT_LEN,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_LFJ_LPT)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_DESCENDING_ORDER,SORT_DESCENDING_ORDER,SORT_GRANT_LEN,SORT_GRANT_LEN,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_LFJ_LNF)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_DESCENDING_ORDER,SORT_DESCENDING_ORDER,SORT_NUM_FRAMES,SORT_NUM_FRAMES,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_EAF)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_MIN_ARRIVAL,SORT_MIN_ARRIVAL,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_EAAF)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_AVG_ARRIVAL,SORT_AVG_ARRIVAL,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_SPD)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_PROP_DELAY,SORT_ONU_NUM,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_LPD)
            {
                scheduleList[onuAttrs[pollONU].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_DESCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_PROP_DELAY,SORT_ONU_NUM,&onuListItem,scheduleList[onuAttrs[pollONU].numSupportedLambdas-1]);
            }
        }
    
#ifdef DEBUG_TRC
        printf("Ordered List of ONUs in Scheduling Pool\n");
        for(loopIndex = 0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
        {
            printf("Supported Lambdas = %d\n",loopIndex);
            onu_list_print(scheduleList[loopIndex]);
            fflush(NULL);
        }
#endif
    
#ifdef DEBUG_TRC
        printf("Build linear schedule list\n");
        fflush(NULL);
#endif
        /* Build linear schedule list */
        onuListCnt = 0;
        for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
        {
            while(scheduleList[loopIndex] != NULL)
            {
                scheduleList[loopIndex] = onu_list_pop(scheduleList[loopIndex],&onuListItem);
                onuList[onuListCnt++] = onuListItem.onuNum;
#ifdef DEBUG_TRC
                printf("ONU #%d, Grant Len = %ld\n", onuListItem.onuNum, onuAttrs[onuListItem.onuNum].grantLen);
                fflush(NULL);
#endif
            }
        }

#ifdef DEBUG_TRC_HI
        printf("%d ONUs to be dispatched\n", onuListCnt);
        fflush(NULL);
#endif

        /* Schedule the available ONUs according to generated list */
        for(loopIndex=0; loopIndex < onuListCnt; loopIndex++)
        {
            pollONU = onuList[loopIndex];
            /* 
             * Schedule this ONU for transmission
             */
            /* Update all Lambdas with available time < current time to current time */
            for(loopIndex2 = 0; loopIndex2 < simParams.NUM_LAMBDAS; loopIndex2++)
            {
                if(lambdaFree[loopIndex2] < simtime())
                {
                    lambdaFree[loopIndex2] = simtime();
                }
            }
#ifdef DEBUG_TRC_HI
            printf("Dispatch ONU #%d for first fit wavelength assignment\n", pollONU);
            fflush(NULL);
#endif
            /* Schedule the ONU */
            schedule_onu(pollONU, LAMBDA_NULL);
        }

        /* 
         * Wait for a polling time prior to channel availability
         *
         */
        /* Determine when next available wavelength is free */
        next_lambda_time = 1e200;
        for(loopIndex = 0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
        {
            if(lambdaFree[loopIndex] < next_lambda_time)
            {
                next_lambda_time = lambdaFree[loopIndex];
            }
        }
        if((next_lambda_time - simParams.SHORTEST_POLL_TIME) > simtime())
        {
            hold((next_lambda_time - simParams.SHORTEST_POLL_TIME - 100e-9) - simtime());
        }
	}
}

/* 
 * FUNCTION: online_interval()
 * DESCRIPTION: Online OLT scheduler that employs just in time scheduling policy
 *              different heuristics can be used for the actual scheduling
 *
 */
void online_interval()
{
    int lambdaNum,posNum;
    int loopIndex;
    double next_lambda_time, next_sched_time, next_avail_onu, max_rtt;
    double latest_lambda_time;
    double earliest_sched_time;
    int	onuList[MAX_ONU], onuListCnt;
    int avail_cost;
    sONU_LIST onuListItem;
	int **cost_matrix;
    hungarian_problem_t myHungProb;
	int colNum;
    int lambdaPerm[MAX_LAMBDAS];

#ifdef DEBUG_TRC_LO
    int trcIdx, trcFlag;
#endif
#ifdef DEBUG_TRC_HI
    double trcSum;
#endif

#ifdef DEBUG_TRC_LO
    printf("\n\n[%10.5e] Online Interval OLT scheduler\n",simtime());
    fflush(NULL);
    for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        printf("ONU #%d ready at [%10.5e]\n",loopIndex,onuAttrs[loopIndex].readyTime);
    }
    fflush(NULL);
    for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
    {
        printf("Lambda #%d free at [%10.5e]\n",loopIndex,lambdaFree[loopIndex]);
    }
    fflush(NULL);
#endif

    /* Initialize data structures, should possibly move these to global area */
    for(loopIndex = 0; loopIndex < MAX_ONU; loopIndex++)
    {
        onuList[loopIndex] = -1;
    }
	
    /*
       Determine when next available wavelength is free
     */
    next_lambda_time = 1e200;
    latest_lambda_time = 0;
    for(loopIndex = 0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
    {
        if(lambdaFree[loopIndex] < next_lambda_time)
        {
            next_lambda_time = lambdaFree[loopIndex];
        }
        if(lambdaFree[loopIndex] > latest_lambda_time)
        {
            latest_lambda_time = lambdaFree[loopIndex];
        }
    }
    /* Determine maximum RTT */
    max_rtt = 0;
    if(schedPoolCount > 0)
    {
        for(loopIndex = 0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            if((schedPool[loopIndex].state == ONU_SCHED_ACTIVE) && (onuAttrs[loopIndex].rtt > max_rtt))
            {
                max_rtt = onuAttrs[loopIndex].rtt;
            }
        }
    }
    else
    {
        max_rtt = simParams.ACTUAL_MAX_PROP_DELAY*2;
    }

    /* Determine schedule time based on wavelength availability and maximum RTT */
    next_sched_time = next_lambda_time - (max_rtt +(64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME);
#ifdef DEBUG_TRC_LO
    printf("[%10.5e] Wait for next scheduling time = [%10.5e], next_lambda_time=[%10.5e], max_rtt=[%10.5e]\n",simtime(),
           next_sched_time,next_lambda_time,max_rtt);
    fflush(NULL);
#endif
    /* Wait for next scheduling time */
    if(next_sched_time > simtime())
    {
        hold(next_sched_time-simtime());
    }

    /*
       Determine new ONUs that are ready for scheduling and add them to the scheduling pool
     */
    next_avail_onu = 1e200;
    for(loopIndex = 0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        if(onuAttrs[loopIndex].readyTime < (next_avail_onu + 1e-9))
        {
            next_avail_onu = onuAttrs[loopIndex].readyTime;
        }
        /* If ONU is ready and not already in pool, add it */
        if((onuAttrs[loopIndex].readyTime < (simtime() + 1e-9)) && (schedPool[loopIndex].state == ONU_SCHED_INACTIVE))
        {
            schedPool[loopIndex].state = ONU_SCHED_ACTIVE;
            schedPool[loopIndex].poolTime = simtime();
            schedPool[loopIndex].rounds = 0;
            schedPoolCount++;
        }
#ifdef DEBUG_TRC_HI
        if(schedPool[loopIndex].state == ONU_SCHED_ACTIVE)
        {
            if((simtime()+onuAttrs[loopIndex].rtt +(64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME) < next_lambda_time)
            {
                printf("ONU #%d available before lambda [%10.5e]!!!\n",loopIndex,(simtime()+onuAttrs[loopIndex].rtt +(64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME));
            }
        }
#endif
    }
    
    /*
       If the scheduling pool is empty, wait for first available ONU
     */
    if(schedPoolCount == 0)
    {
#ifdef DEBUG_TRC_LO
        printf("[%10.5e] Scheduling pool is empty wait until [%10.5e]\n",simtime(),next_avail_onu);
        fflush(NULL);
#endif
        /* wait for first available ONU */
        if(next_avail_onu > simtime())
        {
            hold(next_avail_onu-simtime());
        }
        /* Return to the top */
        return;
    }

    /*
     * Record the the difference between minimum and maximum lambda available 
     * times as load balancing measure
     */
    record(latest_lambda_time-next_lambda_time,loadBalanceMeasure);

#ifdef DEBUG_TRC_HI
    trcSum = 0;
    for(trcIdx=0; trcIdx < simParams.NUM_ONU; trcIdx++)
    {
        trcSum += schedPool[trcIdx].gateLength;
    }
    if(trcSum < (simParams.MAX_PROP_DELAY*2))
    {
        printf("Last round of grants was less than a maximum RTT: [%10.5e]\n",trcSum);
    }
#endif

    /* 
     * Determine grant sizing
     */
    for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        /* Skip over INACTIVE ONUs */
        if(schedPool[loopIndex].state == ONU_SCHED_INACTIVE)
            continue;
        /* Determine grant size for this ONU */
        grant_sizing(loopIndex);
        schedPool[loopIndex].gateLength = onuAttrs[loopIndex].grantLen*simParams.TIME_PER_BYTE;
    }
    /* Record the number of ONUs available for scheduling */
    record(schedPoolCount,numONUSched);

#ifdef DEBUG_TRC_LO
    printf("[%10.5e] Schedule the ONUs in the pool\n",simtime());
    if(schedPoolCount > 1)
    {
        printf("Scheduling Pool contains more than one ONU : %d\n",schedPoolCount);
    }
    fflush(NULL);

    printf("[%10.5e]Scheduling Pool: %d\n",simtime(),schedPoolCount);
    fflush(NULL);
    trcFlag = 0;
    for(trcIdx = 0; trcIdx < simParams.NUM_ONU; trcIdx++)
    {
        if(schedPool[trcIdx].state != ONU_SCHED_INACTIVE)
        {
            printf("ONU #%d(prop = [%10.5e]): state = %d, rounds = %d\n",trcIdx, onuAttrs[trcIdx].latency, 
                   schedPool[trcIdx].state, schedPool[trcIdx].rounds);
            fflush(NULL);
            trcFlag = 1;
        }
    }
    if(trcFlag == 0)
    {
        printf("Empty.\n");
        fflush(NULL);
    }
#endif

    /*
     * Immediately schedule ONUs that deserve preferrential treatment
     */
    if(simParams.NUM_PREFERRED_ONU > 0)
    {
        for(loopIndex=0; loopIndex < simParams.NUM_PREFERRED_ONU; loopIndex++)
        {
            if(schedPool[loopIndex].state != ONU_SCHED_INACTIVE)
            {
#ifdef DEBUG_TRC_LO
                printf("ONU #%d receives preferrential scheduling\n",loopIndex);
                fflush(NULL);
#endif
                /* schedule this ONU's transmission */
                schedule_onu(loopIndex,LAMBDA_NULL);
                /* Remove it from the pool */
                schedPool[loopIndex].state = ONU_SCHED_INACTIVE;
                schedPool[loopIndex].poolTime = -1;
                schedPoolCount--;
            }
        }
    }

    /*
       If the scheduling pool is empty, wait for first available ONU
     */
    if(schedPoolCount == 0)
    {
#ifdef DEBUG_TRC_LO
        printf("[%10.5e] Scheduling pool is empty wait until [%10.5e]\n",simtime(),next_avail_onu);
        fflush(NULL);
#endif
        /* wait for first available ONU */
        if(next_avail_onu > simtime())
        {
            hold(next_avail_onu-simtime());
        }
        /* Return to the top */
        return;
    }

    /*
     * Initialize data structures needed for scheduling
     */
    mSETUP_TEMP_LAMBDA_FREE(loopIndex);
    for(loopIndex=0; loopIndex < MAX_LAMBDAS; loopIndex++)
    {
        scheduleList[loopIndex] = NULL;
    }

    /*
       Schedule the ONUs in the pool (with LFJ-LPT, LFJ-SPT, or WBM)
     */
    switch(simParams.OLT_TYPE)
    {
    case OLT_ONLINE_INTERVAL_LFJ_LPT:
    case OLT_ONLINE_INTERVAL_LFJ_LNF:
    case OLT_ONLINE_INTERVAL_LFJ_SPT:
    case OLT_ONLINE_INTERVAL_EAF:
    case OLT_ONLINE_INTERVAL_EAAF:
        /*
           For ACTIVE ONUs determine grant size and insert into a list sorted according to
           LFJ-LPT, LFJ-LNF, or LFJ-SPT
         */
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            /* Skip over INACTIVE ONUs */
            if(schedPool[loopIndex].state == ONU_SCHED_INACTIVE)
                continue;

            /* Fill out ONU list record for ONU */
            onuListItem.onuNum = loopIndex;
            onuListItem.grantLen = onuAttrs[loopIndex].grantLen;
            onuListItem.poolTime = schedPool[loopIndex].poolTime;
            onuListItem.numFrames = onuAttrs[loopIndex].rptQueueNum;
            onuListItem.minArrivalTime = onuAttrs[loopIndex].minArrivalTime;
            onuListItem.avgArrivalTime = onuAttrs[loopIndex].avgArrivalTime;
            onuListItem.latency = onuAttrs[loopIndex].latency;
            /* Insert ONU list record according to desired scheduling order */
            if(simParams.OLT_TYPE == OLT_ONLINE_INTERVAL_LFJ_SPT)
            {
                scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_LEN,SORT_POOL_TIME,&onuListItem,scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_INTERVAL_LFJ_LPT)
            {
                scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_DESCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_LEN,SORT_POOL_TIME,&onuListItem,scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_INTERVAL_LFJ_LNF)
            {
                scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_DESCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_NUM_FRAMES,SORT_POOL_TIME,&onuListItem,scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_INTERVAL_EAF)
            {
                scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_MIN_ARRIVAL,SORT_POOL_TIME,&onuListItem,scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_INTERVAL_EAAF)
            {
                scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_AVG_ARRIVAL,SORT_POOL_TIME,&onuListItem,scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1]);
            }
        }

#ifdef DEBUG_TRC_LO
        printf("Ordered List of ONUs in Scheduling Pool\n");
        for(trcIdx = 0; trcIdx < simParams.NUM_LAMBDAS; trcIdx++)
        {
            printf("Supported Lambdas = %d\n",trcIdx);
            onu_list_print(scheduleList[trcIdx]);
            fflush(NULL);
        }
#endif

        /*
           Tentatively Schedule the ONUs
         */

        /* Build linear schedule list according to LFJ */
        onuListCnt = 0;
        for(loopIndex=0; loopIndex < MAX_LAMBDAS; loopIndex++)
        {
            while(scheduleList[loopIndex] != NULL)
            {
                scheduleList[loopIndex] = onu_list_pop(scheduleList[loopIndex],&onuListItem);
                onuList[onuListCnt++] = onuListItem.onuNum;
            }
        }

        /* Update all Lambdas with available time < current time to current time */
        mUPDATE_LAMBDA_FREE(loopIndex,lambdaFreeTemp);

        /* Schedule the available ONUs according to generated list */
        for(loopIndex=0; loopIndex < onuListCnt; loopIndex++)
        {
            schedule_onu(onuList[loopIndex],LAMBDA_NULL); /* Schedule ONU tentatively with no pre-selected lambda */
            /* Remove it from the pool */
            schedPool[onuList[loopIndex]].state = ONU_SCHED_INACTIVE;
            schedPool[onuList[loopIndex]].poolTime = -1;
            schedPoolCount--;
        }
        break;

    case OLT_ONLINE_INTERVAL_WBM:
    case OLT_ONLINE_INTERVAL_WBM_LPT:
    case OLT_ONLINE_INTERVAL_WBM_LNF:
    case OLT_ONLINE_INTERVAL_WBM_EAAF:
        /* 
        * Determine dimensions of cost matrix given ONUs in Scheduling Pool 
        */
        onuListCnt = 0;
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            /* Skip over INACTIVE ONUs */
            if(schedPool[loopIndex].state == ONU_SCHED_INACTIVE)
                continue;
            onuList[onuListCnt++] = loopIndex;
        }
        /* cost_matrix_dim = onuListCnt*simParams.NUM_LAMBDAS; */
		/*
		 * Randomly Permute the ONU list and a list of lambdas
		 */
        rand_permute_int(onuList, onuListCnt);
        for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
        {
            lambdaPerm[loopIndex] = loopIndex;
        }
        rand_permute_int(lambdaPerm, simParams.NUM_LAMBDAS);

        /* 
         * Construct cost matrix
         */
		cost_matrix = (int**)calloc(onuListCnt,sizeof(int*));
		for(loopIndex = 0; loopIndex < onuListCnt; loopIndex++)
		{
			cost_matrix[loopIndex] = (int*)calloc(onuListCnt*simParams.NUM_LAMBDAS,sizeof(int));
		}

        for(loopIndex=0; loopIndex < onuListCnt; loopIndex++)
        {
            /* calculate earliest schedule time for this ONU */
            earliest_sched_time = simtime()+onuAttrs[onuList[loopIndex]].rtt +(64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME;
            /* Prepare WBM matrix for this ONU */
            for(lambdaNum=0;lambdaNum < simParams.NUM_LAMBDAS; lambdaNum++)
            {
                /* Compute availability cost for this lambda and ONU combination */
                avail_cost = abs((lambdaFree[lambdaPerm[lambdaNum]]*(1/simParams.TIME_PER_BYTE)) - 
                                 (earliest_sched_time*(1/simParams.TIME_PER_BYTE)));
                /* Normalize the avail_cost by 64 */
                avail_cost = simParams.AVAIL_COST_WEIGHT*(avail_cost/64);
#ifdef DEBUG_TRC_HI
                printf("avail_cost:ONU#%2d:Lam#%d = [%10.5e]:%d\n",loopIndex,lambdaNum,(lambdaFree[lambdaPerm[lambdaNum]] - earliest_sched_time),avail_cost);
                fflush(NULL);
#endif
                for(posNum=0;posNum < onuListCnt; posNum++)
                {
                    if(onuAttrs[onuList[loopIndex]].supportedLambdasMap[lambdaPerm[lambdaNum]] == LAMBDA_FALSE)
                    {
                        /* lambda not supported */
                        cost_matrix[loopIndex][lambdaNum*onuListCnt+posNum] = COST_INFINITE;
                    }
                    else
                    {
                        /* lambda supported */
                        if(onuAttrs[onuList[loopIndex]].grantLen > 0)
                        {
                            /* never allow the cost to be higher than a number slightly smaller than COST_INFINITE */
                            /* also, normalize the cost by 64 */
                            cost_matrix[loopIndex][lambdaNum*onuListCnt+posNum] = int_min(((onuListCnt-(posNum))*(onuAttrs[onuList[loopIndex]].grantLen/64+1) 
                                + avail_cost),(COST_INFINITE-100));
                        }
                        else
                        {
                            cost_matrix[loopIndex][lambdaNum*onuListCnt+posNum] = (onuListCnt-(posNum)) + 
                                avail_cost /* priority cost */;
                        }
                    }
                }
            }
        }

#ifdef DEBUG_TRC
        debug_print_cost_matrix(cost_matrix,onuListCnt,onuListCnt*simParams.NUM_LAMBDAS);
#endif
        /* dump_cost_matrix(); */
        /* Solve weighted bipartite matching */
#ifdef DEBUG_TRC
        printf("Solve Matching\n");
		fflush(NULL);
#endif

        hungarian_init(&myHungProb,cost_matrix,onuListCnt,onuListCnt*simParams.NUM_LAMBDAS,HUNGARIAN_MODE_MINIMIZE_COST);
		hungarian_solve(&myHungProb);

#ifdef DEBUG_TRC_HI
        printf("WBM Solved\n");
		fflush(NULL);
#endif

        /* 
         * Turn WBM solution into a tentative schedule 
         */
        /* Turn hungarian_solution structure into scheduleList structure */
        for(loopIndex=0;loopIndex < onuListCnt;loopIndex++)
        {
            colNum    = hungarian_assignment(&myHungProb,loopIndex);
            lambdaNum = lambdaPerm[colNum / onuListCnt];
            posNum    = colNum % onuListCnt;

#ifdef DEBUG_TRC_HI
			printf("Assigning Pos %d on Lambda %d to ONU %d:%ld\n", posNum, lambdaNum, onuList[loopIndex], onuAttrs[onuList[loopIndex]].grantLen);
			fflush(NULL);
#endif
            onuListItem.onuNum = onuList[loopIndex];
            onuListItem.posNum = posNum;
            onuListItem.grantLen = onuAttrs[onuListItem.onuNum].grantLen;
            onuListItem.numFrames = onuAttrs[onuListItem.onuNum].rptQueueNum;
            onuListItem.grantTime = 0;
            onuListItem.poolTime = schedPool[onuListItem.onuNum].poolTime;
            switch(simParams.OLT_TYPE)
            {
            case OLT_ONLINE_INTERVAL_WBM:
                scheduleList[lambdaNum] = onu_list_insert(SORT_ASCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_GRANT_LEN, SORT_POOL_TIME, &onuListItem, scheduleList[lambdaNum]);
                break;
            case OLT_ONLINE_INTERVAL_WBM_LPT:
                scheduleList[lambdaNum] = onu_list_insert(SORT_DESCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_GRANT_LEN, SORT_POOL_TIME, &onuListItem, scheduleList[lambdaNum]);
                break;
            case OLT_ONLINE_INTERVAL_WBM_LNF:
                scheduleList[lambdaNum] = onu_list_insert(SORT_DESCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_NUM_FRAMES, SORT_POOL_TIME, &onuListItem, scheduleList[lambdaNum]);
                break;
            case OLT_ONLINE_INTERVAL_WBM_EAAF:
                scheduleList[lambdaNum] = onu_list_insert(SORT_ASCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_AVG_ARRIVAL, SORT_POOL_TIME, &onuListItem, scheduleList[lambdaNum]);
                break;

            }
        }
		
#ifdef DEBUG_TRC_LO
		printf("WBM Schedule\n");
		for(trcIdx=0;trcIdx<simParams.NUM_LAMBDAS;trcIdx++)
		{
			printf("Lambda #%d\n",trcIdx);
			fflush(NULL);
			onu_list_print(scheduleList[trcIdx]);
			fflush(NULL);
		}
#endif
        /* Tentatively schedule all the active ONUs according to WBM solution */
        for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
        {
            while(scheduleList[loopIndex] != NULL)
            {
                scheduleList[loopIndex] = onu_list_pop(scheduleList[loopIndex],&onuListItem);
                /* Check if the wavelength assignment is valid */
                if(onuAttrs[onuListItem.onuNum].supportedLambdasMap[loopIndex] == LAMBDA_TRUE)
                {
                    /* tentatively schedule this ONU on the selected wavelength */
                    schedule_onu(onuListItem.onuNum,loopIndex);
                }
                else
                {
                    /* tentatively schedule this ONU on supported channel */
                    schedule_onu(onuListItem.onuNum,LAMBDA_NULL);
                }
                /* Remove it from the pool */
                schedPool[onuListItem.onuNum].state = ONU_SCHED_INACTIVE;
                schedPool[onuListItem.onuNum].poolTime = -1;
                schedPoolCount--;
            }
        }
        hungarian_free(&myHungProb);
		for(loopIndex = 0; loopIndex < onuListCnt; loopIndex++)
		{
			free(cost_matrix[loopIndex]);
		}
        free(cost_matrix);
        break;

    default:
        /* Error */
        break;

    }
}

/* 
 * FUNCTION: online_jit()
 * DESCRIPTION: Online OLT scheduler that employs just in time tentative scheduling policy
 *              different heuristics can be used for the actual scheduling
 *
 * NOTE: The scheduling is driven by wavelengths that become available for transmission,
 *       Maybe we do not even need WBM, we simply select the supported ONU with the highest
 *       priority? (Basically, we are scheduling one machine at a time)
 *       If there are no ONUs in the scheduling pool, when a wavelength is available. The next
 *       available supported ONU is scheduled on the wavelength.
 *
       Update ONU priorities based on:

       1) Time in scheduling pool
       2) Class of traffic
       3) Fairness metric
       4) Transmission size (incorporated in scheduling outside of priority?)
       5) ONU wavelength flexibility (incorporated in scheduling outside of priority?)

       P_{ij} = \alpha*poolTime + \beta*j + \omega*fairnessMetric, where i is the ONU number, j is the traffic class, \alpha is the pool time weight,
       \beta is the traffic class weight, and \omega is the fairness metric weight

       \alpha, \beta, and \omega are system tuning parameters that need to be optimized, either using a linear program or through experimentation

    priority[poolTrav->onuNum] = simParams.AGE_WEIGHT*(simtime() - schedPool[loopIndex].poolTime)  + simParams.CLASS_WEIGHT*onuAttrs[poolTrav->onuNum].class
        + simParams.FAIRNESS_WEIGHT*onuAttrs[poolTrav->onuNum].fairness;
 *
 */
void online_jit()
{
    int lambdaNum,posNum;
    int loopIndex;
    double next_lambda_time, next_sched_time, next_avail_onu, max_rtt;
    double latest_lambda_time;
    double earliest_sched_time;
    int	onuList[MAX_ONU], onuListCnt;
    int avail_cost;
    sONU_LIST onuListItem;
	int **cost_matrix;
    hungarian_problem_t myHungProb;
	int colNum;
    int lambdaPerm[MAX_LAMBDAS];

#ifdef DEBUG_TRC_LO
    int trcIdx, trcFlag;
#endif
#ifdef DEBUG_TRC
    sONU_LIST trcOnuListItem;
#endif
#ifdef DEBUG_TRC_HI
    double trcSum;
#endif

#ifdef DEBUG_TRC_LO
    printf("\n\n[%10.5e] Online JIT OLT scheduler\n",simtime());
    fflush(NULL);
    for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        printf("ONU #%d ready at [%10.5e]\n",loopIndex,onuAttrs[loopIndex].readyTime);
    }
    fflush(NULL);
    for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
    {
        printf("Lambda #%d free at [%10.5e]\n",loopIndex,lambdaFree[loopIndex]);
    }
    fflush(NULL);
#endif

    /* Initialize data structures, should possibly move these to global area */
    for(loopIndex = 0; loopIndex < MAX_ONU; loopIndex++)
    {
        onuList[loopIndex] = -1;
    }
	
    /*
       Determine when next available wavelength is free
     */
    next_lambda_time = 1e200;
    latest_lambda_time = 0;
    for(loopIndex = 0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
    {
        if(lambdaFree[loopIndex] < next_lambda_time)
        {
            next_lambda_time = lambdaFree[loopIndex];
        }
        if(lambdaFree[loopIndex] > latest_lambda_time)
        {
            latest_lambda_time = lambdaFree[loopIndex];
        }
    }
    /* Determine maximum RTT */
    max_rtt = 5e6;
    if(schedPoolCount > 0)
    {
        for(loopIndex = 0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            if((schedPool[loopIndex].state == ONU_SCHED_ACTIVE) && (onuAttrs[loopIndex].rtt < max_rtt))
            {
                max_rtt = onuAttrs[loopIndex].rtt;
            }
        }
    }
    else
    {
        //max_rtt = simParams.ACTUAL_MAX_PROP_DELAY*2;
        max_rtt = (6.68e-6)*2;
    }

    /* Determine schedule time based on wavelength availability and maximum RTT */
    next_sched_time = next_lambda_time - (max_rtt +(64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME);
#ifdef DEBUG_TRC_LO
    printf("[%10.5e] Wait for next scheduling time = [%10.5e], next_lambda_time=[%10.5e], max_rtt=[%10.5e]\n",simtime(),
           next_sched_time,next_lambda_time,max_rtt);
    fflush(NULL);
#endif
    /* Wait for next scheduling time */
    if(next_sched_time > simtime())
    {
        hold(next_sched_time-simtime());
    }

    /*
       Determine new ONUs that are ready for scheduling and add them to the scheduling pool
     */
    next_avail_onu = 1e200;
    for(loopIndex = 0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        if(onuAttrs[loopIndex].readyTime < (next_avail_onu + 1e-9))
        {
            next_avail_onu = onuAttrs[loopIndex].readyTime;
        }
        /* If ONU is ready and not already in pool, add it */
        if((onuAttrs[loopIndex].readyTime < (simtime() + 1e-9)) && (schedPool[loopIndex].state == ONU_SCHED_INACTIVE))
        {
            schedPool[loopIndex].state = ONU_SCHED_ACTIVE;
            schedPool[loopIndex].poolTime = simtime();
            schedPool[loopIndex].rounds = 0;
            schedPoolCount++;
        }
#ifdef DEBUG_TRC
        if(schedPool[loopIndex].state == ONU_SCHED_ACTIVE)
        {
            if((simtime()+onuAttrs[loopIndex].rtt +(64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME) < next_lambda_time)
            {
                printf("ONU #%d available before lambda [%10.5e]!!!\n",loopIndex,(simtime()+onuAttrs[loopIndex].rtt +(64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME));
            }
        }
#endif
    }
    
    /*
       If the scheduling pool is empty, wait for first available ONU
     */
    if(schedPoolCount == 0)
    {
#ifdef DEBUG_TRC
        printf("[%10.5e] Scheduling pool is empty wait until [%10.5e]\n",simtime(),next_avail_onu);
        fflush(NULL);
#endif
        /* wait for first available ONU */
        if(next_avail_onu > simtime())
        {
            hold(next_avail_onu-simtime());
        }
        /* Return to the top */
        return;
    }

    /*
     * Record the the difference between minimum and maximum lambda available 
     * times as load balancing measure
     */
    record(latest_lambda_time-next_lambda_time,loadBalanceMeasure);

#ifdef DEBUG_TRC_LO
    printf("[%10.5e] Schedule the ONUs in the pool\n",simtime());
    if(schedPoolCount > 1)
    {
        printf("Scheduling Pool contains more than one ONU : %d\n",schedPoolCount);
    }
    fflush(NULL);

    printf("[%10.5e]Scheduling Pool: %d\n",simtime(),schedPoolCount);
    fflush(NULL);
    trcFlag = 0;
    for(trcIdx = 0; trcIdx < simParams.NUM_ONU; trcIdx++)
    {
        if(schedPool[trcIdx].state != ONU_SCHED_INACTIVE)
        {
            printf("ONU #%d(prop = [%10.5e]): state = %d, poolTime = [%10.5e], rounds = %d\n",trcIdx, onuAttrs[trcIdx].latency, 
                   schedPool[trcIdx].state, schedPool[trcIdx].poolTime, schedPool[trcIdx].rounds);
            fflush(NULL);
            trcFlag = 1;
        }
    }
    if(trcFlag == 0)
    {
        printf("Empty.\n");
        fflush(NULL);
    }
#endif

    /* 
     * Determine grant sizing
     */
    for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        /* Skip over INACTIVE ONUs */
        if(schedPool[loopIndex].state == ONU_SCHED_INACTIVE)
            continue;
        /* Determine grant size for this ONU */
        grant_sizing(loopIndex);
        schedPool[loopIndex].gateLength = onuAttrs[loopIndex].grantLen*simParams.TIME_PER_BYTE;
    }

    /*
     * Firmly schedule ONUs that have exceeded the starvation threshold
     */
    for(loopIndex = 0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        if(schedPool[loopIndex].state != ONU_SCHED_INACTIVE)
        {
            schedPool[loopIndex].rounds++;
            if(schedPool[loopIndex].rounds > simParams.STARVE_THRESH)
            {
#ifdef DEBUG_TRC_LO
                printf("ONU %d has exceeded it's starvation threshold: rounds = %d !!!\n",loopIndex,schedPool[loopIndex].rounds);
#endif
                schedule_onu(loopIndex,LAMBDA_NULL);
                /* Remove it from the pool */
                schedPool[loopIndex].state = ONU_SCHED_INACTIVE;
                schedPool[loopIndex].poolTime = -1;
                schedPoolCount--;
            }
        }
    }

    /*
        Check scheduling pool again
       If the scheduling pool is empty, wait for first available ONU
     */
    if(schedPoolCount == 0)
    {
#ifdef DEBUG_TRC
        printf("[%10.5e] Scheduling pool is empty wait until [%10.5e]\n",simtime(),next_avail_onu);
        fflush(NULL);
#endif
        /* wait for first available ONU */
        if(next_avail_onu > simtime())
        {
            hold(next_avail_onu-simtime());
        }
        /* Return to the top */
        return;
    }

#ifdef DEBUG_TRC_HI
    trcSum = 0;
    for(trcIdx=0; trcIdx < simParams.NUM_ONU; trcIdx++)
    {
        trcSum += schedPool[trcIdx].gateLength;
    }
    if(trcSum < (simParams.MAX_PROP_DELAY*2))
    {
        printf("Last round of grants was less than a maximum RTT: [%10.5e]\n",trcSum);
    }
#endif

    /* Record the number of ONUs available for scheduling */
    record(schedPoolCount,numONUSched);

    /*
     * Immediately schedule ONUs that deserve preferrential treatment
     */
    if(simParams.NUM_PREFERRED_ONU > 0)
    {
        for(loopIndex=0; loopIndex < simParams.NUM_PREFERRED_ONU; loopIndex++)
        {
            if(schedPool[loopIndex].state != ONU_SCHED_INACTIVE)
            {
#ifdef DEBUG_TRC_LO
                printf("ONU #%d receives preferrential scheduling\n",loopIndex);
                fflush(NULL);
#endif
                /* schedule this ONU's transmission */
                schedule_onu(loopIndex,LAMBDA_NULL);
                /* Remove it from the pool */
                schedPool[loopIndex].state = ONU_SCHED_INACTIVE;
                schedPool[loopIndex].poolTime = -1;
                schedPoolCount--;
            }
        }
    }

    /*
        Check scheduling pool again
       If the scheduling pool is empty, wait for first available ONU
     */
    if(schedPoolCount == 0)
    {
#ifdef DEBUG_TRC
        printf("[%10.5e] Scheduling pool is empty wait until [%10.5e]\n",simtime(),next_avail_onu);
        fflush(NULL);
#endif
        /* wait for first available ONU */
        if(next_avail_onu > simtime())
        {
            hold(next_avail_onu-simtime());
        }
        /* Return to the top */
        return;
    }

    /*
     * Initialize data structures needed for scheduling
     */
    mSETUP_TEMP_LAMBDA_FREE(loopIndex);
    for(loopIndex=0; loopIndex < MAX_LAMBDAS; loopIndex++)
    {
        scheduleList[loopIndex] = NULL;
    }

    /*
       Schedule the ONUs in the pool (with LFJ-LPT, LFJ-SPT, or WBM)
     */
    switch(simParams.OLT_TYPE)
    {
    case OLT_ONLINE_JIT_LFJ_LPT:
    case OLT_ONLINE_JIT_LFJ_LNF:
    case OLT_ONLINE_JIT_LFJ_SPT:
    case OLT_ONLINE_JIT_EAF:
    case OLT_ONLINE_JIT_EAAF:
    case OLT_ONLINE_JIT_SPD:
    case OLT_ONLINE_JIT_LPD:
        /*
           For ACTIVE ONUs determine grant size and insert into a list sorted according to
           LFJ-LPT, LFJ-LNF, or LFJ-SPT
         */
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            /* Skip over INACTIVE ONUs */
            if(schedPool[loopIndex].state == ONU_SCHED_INACTIVE)
                continue;

            /* Fill out ONU list record for ONU */
            onuListItem.onuNum = loopIndex;
            onuListItem.grantLen = onuAttrs[loopIndex].grantLen;
            onuListItem.poolTime = schedPool[loopIndex].poolTime;
            onuListItem.numFrames = onuAttrs[loopIndex].rptQueueNum;
            onuListItem.minArrivalTime = onuAttrs[loopIndex].minArrivalTime;
            onuListItem.avgArrivalTime = onuAttrs[loopIndex].avgArrivalTime;
            onuListItem.latency = onuAttrs[loopIndex].latency;
            /* Insert ONU list record according to desired scheduling order */
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_LFJ_SPT)
            {
                scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_LEN,SORT_POOL_TIME,&onuListItem,scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_LFJ_LPT)
            {
                scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_DESCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_LEN,SORT_POOL_TIME,&onuListItem,scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_LFJ_LNF)
            {
                scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_DESCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_NUM_FRAMES,SORT_POOL_TIME,&onuListItem,scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_EAF)
            {
                scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_MIN_ARRIVAL,SORT_POOL_TIME,&onuListItem,scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_EAAF)
            {
                scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_AVG_ARRIVAL,SORT_POOL_TIME,&onuListItem,scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_SPD)
            {
                scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_PROP_DELAY,SORT_ONU_NUM,&onuListItem,scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1]);
            }
            if(simParams.OLT_TYPE == OLT_ONLINE_JIT_LPD)
            {
                scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1] = 
                    onu_list_insert(SORT_DESCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_PROP_DELAY,SORT_ONU_NUM,&onuListItem,scheduleList[onuAttrs[loopIndex].numSupportedLambdas-1]);
            }
        }

#ifdef DEBUG_TRC
        printf("Ordered List of ONUs in Scheduling Pool\n");
        for(trcIdx = 0; trcIdx < simParams.NUM_LAMBDAS; trcIdx++)
        {
            printf("Supported Lambdas = %d\n",trcIdx);
            onu_list_print(scheduleList[trcIdx]);
            fflush(NULL);
        }
#endif

        /*
           Tentatively Schedule the ONUs
         */

        /* Build linear schedule list according to LFJ */
        onuListCnt = 0;
        for(loopIndex=0; loopIndex < MAX_LAMBDAS; loopIndex++)
        {
            while(scheduleList[loopIndex] != NULL)
            {
                scheduleList[loopIndex] = onu_list_pop(scheduleList[loopIndex],&onuListItem);
                onuList[onuListCnt++] = onuListItem.onuNum;
            }
        }

        /* Update all Lambdas with available time < current time to current time */
        mUPDATE_LAMBDA_FREE(loopIndex,lambdaFreeTemp);

        /* Schedule the available ONUs according to generated list */
        for(loopIndex=0; loopIndex < onuListCnt; loopIndex++)
        {
            tentatively_schedule_onu(onuList[loopIndex],LAMBDA_NULL); /* Schedule ONU tentatively with no pre-selected lambda */
        }
#ifdef DEBUG_TRC
        printf("[%10.5e]Scheduling Pool: %d\n",simtime(),schedPoolCount);
        for(trcIdx = 0; trcIdx < simParams.NUM_ONU; trcIdx++)
        {
            if(schedPool[trcIdx].state != ONU_SCHED_INACTIVE)
            {
                printf("ONU #%d: state = %d, poolTime = [%10.5e], gateLambda = %d, gateStart = [%10.5e], "
                       "gateLength = [%10.5e], gateLength = %d bytes, rounds =%d\n",trcIdx, schedPool[trcIdx].state,
                       schedPool[trcIdx].poolTime, schedPool[trcIdx].gateLambda, schedPool[trcIdx].gateStart,
                       schedPool[trcIdx].gateLength, (int)(schedPool[trcIdx].gateLength/simParams.TIME_PER_BYTE),
                       schedPool[trcIdx].rounds);
                fflush(NULL);
                trcFlag = 1;
            }
        }
#endif
        break;

    case OLT_ONLINE_JIT_WBM:
    case OLT_ONLINE_JIT_WBM_LPT:
    case OLT_ONLINE_JIT_WBM_LNF:
    case OLT_ONLINE_JIT_WBM_EAAF:
        /* 
        * Determine dimensions of cost matrix given ONUs in Scheduling Pool 
        */
        onuListCnt = 0;
        for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            /* Skip over INACTIVE ONUs */
            if(schedPool[loopIndex].state == ONU_SCHED_INACTIVE)
                continue;
            onuList[onuListCnt++] = loopIndex;
        }
        /* cost_matrix_dim = onuListCnt*simParams.NUM_LAMBDAS; */
		/*
		 * Randomly Permute the ONU list and a list of lambdas
		 */
        rand_permute_int(onuList, onuListCnt);
        for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
        {
            lambdaPerm[loopIndex] = loopIndex;
        }
        rand_permute_int(lambdaPerm, simParams.NUM_LAMBDAS);

        /* 
         * Construct cost matrix
         */
		cost_matrix = (int**)calloc(onuListCnt,sizeof(int*));
		for(loopIndex = 0; loopIndex < onuListCnt; loopIndex++)
		{
			cost_matrix[loopIndex] = (int*)calloc(onuListCnt*simParams.NUM_LAMBDAS,sizeof(int));
		}

        for(loopIndex=0; loopIndex < onuListCnt; loopIndex++)
        {
            /* calculate earliest schedule time for this ONU */
            earliest_sched_time = simtime()+onuAttrs[onuList[loopIndex]].rtt +(64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME;
            /* Prepare WBM matrix for this ONU */
            for(lambdaNum=0;lambdaNum < simParams.NUM_LAMBDAS; lambdaNum++)
            {
                /* Compute availability cost for this lambda and ONU combination */
                avail_cost = abs((lambdaFree[lambdaPerm[lambdaNum]]*(1/simParams.TIME_PER_BYTE)) - 
                                 (earliest_sched_time*(1/simParams.TIME_PER_BYTE)));
                /* Normalize the avail_cost by 64 */
                avail_cost = simParams.AVAIL_COST_WEIGHT*(avail_cost/64);
#ifdef DEBUG_TRC_HI
                printf("avail_cost:ONU#%2d:Lam#%d = [%10.5e]:%d\n",loopIndex,lambdaNum,(lambdaFree[lambdaPerm[lambdaNum]] - earliest_sched_time),avail_cost);
                fflush(NULL);
#endif
                for(posNum=0;posNum < onuListCnt; posNum++)
                {
                    if(onuAttrs[onuList[loopIndex]].supportedLambdasMap[lambdaPerm[lambdaNum]] == LAMBDA_FALSE)
                    {
                        /* lambda not supported */
                        cost_matrix[loopIndex][lambdaNum*onuListCnt+posNum] = COST_INFINITE;
                    }
                    else
                    {
                        /* lambda supported */
                        if(onuAttrs[onuList[loopIndex]].grantLen > 0)
                        {
                            /* never allow the cost to be higher than a number slightly smaller than COST_INFINITE */
                            /* also, normalize the cost by 64 */
                            cost_matrix[loopIndex][lambdaNum*onuListCnt+posNum] = int_min(((onuListCnt-(posNum))*(onuAttrs[onuList[loopIndex]].grantLen/64+1) 
                                + avail_cost),(COST_INFINITE-100));
                        }
                        else
                        {
                            cost_matrix[loopIndex][lambdaNum*onuListCnt+posNum] = (onuListCnt-(posNum)) + 
                                avail_cost /* priority cost */;
                        }
                    }
                }
            }
        }

#ifdef DEBUG_TRC
        debug_print_cost_matrix(cost_matrix,onuListCnt,onuListCnt*simParams.NUM_LAMBDAS);
#endif
        /* dump_cost_matrix(); */
        /* Solve weighted bipartite matching */
#ifdef DEBUG_TRC
        printf("Solve Matching\n");
		fflush(NULL);
#endif

        hungarian_init(&myHungProb,cost_matrix,onuListCnt,onuListCnt*simParams.NUM_LAMBDAS,HUNGARIAN_MODE_MINIMIZE_COST);
		hungarian_solve(&myHungProb);

#ifdef DEBUG_TRC
        printf("WBM Solved\n");
		fflush(NULL);
#endif

        /* 
         * Turn WBM solution into a tentative schedule 
         */
        /* Turn hungarian_solution structure into scheduleList structure */
        for(loopIndex=0;loopIndex < onuListCnt;loopIndex++)
        {
            colNum    = hungarian_assignment(&myHungProb,loopIndex);
            lambdaNum = lambdaPerm[colNum / onuListCnt];
            posNum    = colNum % onuListCnt;

#ifdef DEBUG_TRC_HI
			printf("Assigning Pos %d on Lambda %d to ONU %d:%ld\n", posNum, lambdaNum, onuList[loopIndex], onuAttrs[onuList[loopIndex]].grantLen);
			fflush(NULL);
#endif
            onuListItem.onuNum = onuList[loopIndex];
            onuListItem.posNum = posNum;
            onuListItem.grantLen = onuAttrs[onuListItem.onuNum].grantLen;
            onuListItem.numFrames = onuAttrs[onuListItem.onuNum].rptQueueNum;
            onuListItem.grantTime = 0;
            onuListItem.poolTime = schedPool[onuListItem.onuNum].poolTime;
            switch(simParams.OLT_TYPE)
            {
            case OLT_ONLINE_JIT_WBM:
                scheduleList[lambdaNum] = onu_list_insert(SORT_ASCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_GRANT_LEN, SORT_POOL_TIME, &onuListItem, scheduleList[lambdaNum]);
                break;
            case OLT_ONLINE_JIT_WBM_LPT:
                scheduleList[lambdaNum] = onu_list_insert(SORT_DESCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_GRANT_LEN, SORT_POOL_TIME, &onuListItem, scheduleList[lambdaNum]);
                break;
            case OLT_ONLINE_JIT_WBM_LNF:
                scheduleList[lambdaNum] = onu_list_insert(SORT_DESCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_NUM_FRAMES, SORT_POOL_TIME, &onuListItem, scheduleList[lambdaNum]);
                break;
            case OLT_ONLINE_JIT_WBM_EAAF:
                scheduleList[lambdaNum] = onu_list_insert(SORT_ASCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_AVG_ARRIVAL, SORT_POOL_TIME, &onuListItem, scheduleList[lambdaNum]);
                break;

            }
        }
		
#ifdef DEBUG_TRC
		printf("WBM Schedule\n");
		for(trcIdx=0;trcIdx<simParams.NUM_LAMBDAS;trcIdx++)
		{
			printf("Lambda #%d\n",trcIdx);
			fflush(NULL);
			onu_list_print(scheduleList[trcIdx]);
			fflush(NULL);
		}
#endif
        /* Tentatively schedule all the active ONUs according to WBM solution */
        for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
        {
            while(scheduleList[loopIndex] != NULL)
            {
                scheduleList[loopIndex] = onu_list_pop(scheduleList[loopIndex],&onuListItem);
                /* Check if the wavelength assignment is valid */
                if(onuAttrs[onuListItem.onuNum].supportedLambdasMap[loopIndex] == LAMBDA_TRUE)
                {
                    /* tentatively schedule this ONU on the selected wavelength */
                    tentatively_schedule_onu(onuListItem.onuNum,loopIndex);
                }
                else
                {
                    /* tentatively schedule this ONU on supported channel */
                    tentatively_schedule_onu(onuListItem.onuNum,LAMBDA_NULL);
                }
            }
        }
        hungarian_free(&myHungProb);
		for(loopIndex = 0; loopIndex < onuListCnt; loopIndex++)
		{
			free(cost_matrix[loopIndex]);
		}
        free(cost_matrix);
        break;

    default:
        /* Error */
        break;

    }

#ifdef DEBUG_TRC
    printf("Schedule:\n");
    fflush(NULL);
    for(trcIdx=0; trcIdx < simParams.NUM_LAMBDAS; trcIdx++)
    {
        trcList[trcIdx] = NULL;
    }
    for(trcIdx=0; trcIdx < simParams.NUM_ONU; trcIdx++)
    {
        if((schedPool[trcIdx].state == ONU_SCHED_ACTIVE))
        {
            /* Place this ONU into wavelength list */
            trcOnuListItem.onuNum = trcIdx;
            trcOnuListItem.grantLen = onuAttrs[trcIdx].grantLen;
            trcOnuListItem.grantTime = schedPool[trcIdx].gateStart;
            trcOnuListItem.numFrames = onuAttrs[trcIdx].rptQueueNum;
            trcOnuListItem.poolTime = schedPool[trcIdx].poolTime;
            trcList[schedPool[trcIdx].gateLambda] = onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_TIME,SORT_POOL_TIME,&trcOnuListItem,trcList[schedPool[trcIdx].gateLambda]);
        }
    }
    for(trcIdx=0; trcIdx < simParams.NUM_LAMBDAS; trcIdx++)
    {
        printf("lambda %d: ",trcIdx);
        while(trcList[trcIdx] != NULL)
        {
            trcList[trcIdx] = onu_list_pop(trcList[trcIdx],&trcOnuListItem);
            printf("ONU #%d Start: %10.5e Length: %10.5e", trcOnuListItem.onuNum, trcOnuListItem.grantTime, (double)(trcOnuListItem.grantLen*simParams.TIME_PER_BYTE));
            if(trcList[trcIdx] == NULL)
            {
                printf("\n");
            }
            else
            {
                printf(", ");
            }
        }
    }

#endif

#ifdef DEBUG_TRC
        printf("Setup transmission for pending ONUs\n");
        fflush(NULL);
#endif
    /*
     * Firmly schedule imminent ONUs
     */

    /* Generate an ONU list of imminent ONUs in time order */
    onuListCnt = 0;
    scheduleList[0] = NULL;
    for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        if((schedPool[loopIndex].state == ONU_SCHED_ACTIVE) && 
           ((schedPool[loopIndex].gateStart - simtime()) < (onuAttrs[loopIndex].rtt +(64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME)))
        {
#ifdef DEBUG_TRC
            printf("ONU #%d has an imminent transmission\n",loopIndex);
            fflush(NULL);
#endif
            /* Record number of scheduling rounds this ONU participated in */
            record(schedPool[loopIndex].rounds,schedRounds);
            schedPool[loopIndex].state = ONU_SCHED_IMMINENT;
            /* Insert into ordered ONU list */
            onuListCnt++;
            onuListItem.onuNum = loopIndex;
            onuListItem.grantLen = onuAttrs[loopIndex].grantLen;
            onuListItem.grantTime = schedPool[loopIndex].gateStart;
            onuListItem.numFrames = onuAttrs[loopIndex].rptQueueNum;
            onuListItem.poolTime = schedPool[loopIndex].poolTime;
            scheduleList[0] = onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_TIME,SORT_POOL_TIME,&onuListItem,scheduleList[0]);
        }
    }

#ifdef DEBUG_TRC
    if(scheduleList[0] != NULL)
    {
        if(scheduleList[0]->grantTime > lambdaFree[0])
        {
            printf("Gap in lambda usage: grant time = [%10.5e], lambdaFree = [%10.5e]\n",scheduleList[0]->grantTime,lambdaFree[0]); 
        }
        printf("Imminent ONU List: %d\n",onuListCnt);
        onu_list_print(scheduleList[0]);
    }
    else
    {
        printf("Imminent ONU List: empty\n");
    }
    fflush(NULL);
#endif

    /* Setup transmission for imminent ONUs */
    for(loopIndex=0; loopIndex < onuListCnt; loopIndex++)
    {
        if(scheduleList[0] != NULL)
        {
            scheduleList[0] = onu_list_pop(scheduleList[0],&onuListItem);
            firmly_schedule_onu(onuListItem.onuNum);
        }
    }
}

/* 
 * FUNCTION: online_jit_test()
 * DESCRIPTION: Online OLT scheduler that employs just in time tentative scheduling policy
 *              different heuristics can be used for the actual scheduling
 *
 */
void online_jit_test()
{
    int lambdaNum,posNum;
    int loopIndex;
    double next_lambda_time, next_sched_time, next_avail_onu, max_rtt;
    double latest_lambda_time;
    int	onuList[MAX_ONU], onuListCnt;
    int lambda_avail_cost[MAX_LAMBDAS];
    sONU_LIST onuListItem;
	int **cost_matrix;
    hungarian_problem_t myHungProb;
	int colNum;
    int lambdaPerm[MAX_LAMBDAS];
    unsigned long avgGrantSize;

#ifdef DEBUG_TRC_LO
    int trcIdx;
    sONU_LIST trcOnuListItem;
#endif

#ifdef DEBUG_TRC_LO
    printf("\n\n[%10.5e] Online JIT OLT scheduler\n",simtime());
    fflush(NULL);
#endif

    /* Initialize data structures, should possibly move these to global area */
    for(loopIndex = 0; loopIndex < MAX_ONU; loopIndex++)
    {
        onuList[loopIndex] = -1;
    }
	
    /*
       Determine when next available wavelength is free
     */
    next_lambda_time = 1e200;
    latest_lambda_time = 0;
    for(loopIndex = 0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
    {
        if(lambdaFree[loopIndex] < next_lambda_time)
        {
            next_lambda_time = lambdaFree[loopIndex];
        }
        if(lambdaFree[loopIndex] > latest_lambda_time)
        {
            latest_lambda_time = lambdaFree[loopIndex];
        }
    }
    /* Determine maximum RTT */
    max_rtt = 0;
    if(schedPool > 0)
    {
        for(loopIndex = 0; loopIndex < simParams.NUM_ONU; loopIndex++)
        {
            if((schedPool[loopIndex].state == ONU_SCHED_ACTIVE) && (onuAttrs[loopIndex].rtt > max_rtt))
            {
                max_rtt = onuAttrs[loopIndex].rtt;
            }
        }
    }
    else
    {
        max_rtt = simParams.MAX_PROP_DELAY*2;
    }

    /* Determine schedule time based on wavelength availability and maximum RTT */
    next_sched_time = next_lambda_time - (max_rtt +(64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME);

    /* Wait for next scheduling time */
    if(next_sched_time > simtime())
    {
        hold(next_sched_time-simtime());
    }

    /*
       Determine new ONUs that are ready for scheduling and add them to the scheduling pool
     */
    next_avail_onu = 1e200;
    for(loopIndex = 0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        if(onuAttrs[loopIndex].readyTime < (next_avail_onu + 1e-9))
        {
            next_avail_onu = onuAttrs[loopIndex].readyTime;
        }
        /* If ONU is ready and not already in pool, add it */
        if((onuAttrs[loopIndex].readyTime < (simtime() + 1e-9)) && (schedPool[loopIndex].state == ONU_SCHED_INACTIVE))
        {
            schedPool[loopIndex].state = ONU_SCHED_ACTIVE;
            schedPool[loopIndex].poolTime = simtime();
            schedPool[loopIndex].rounds = 0;
            schedPoolCount++;
        }
    }

#ifdef DEBUG_TRC_LO
    printf("# of ONUs: %d\n", schedPoolCount);
    fflush(NULL);
#endif
    
    /*
       If the scheduling pool is empty, wait for first available ONU
     */
    if(schedPoolCount == 0)
    {
        /* wait for first available ONU */
        if(next_avail_onu > simtime())
        {
            hold(next_avail_onu-simtime());
        }
        /* Return to the top */
        return;
    }

    /*
     * Record the the difference between minimum and maximum lambda available 
     * times as load balancing measure
     */
    record(latest_lambda_time-next_lambda_time,loadBalanceMeasure);

    /*
     * Firmly schedule ONUs that have exceeded the starvation threshold
     */
    for(loopIndex = 0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        if(schedPool[loopIndex].state != ONU_SCHED_INACTIVE)
        {
            schedPool[loopIndex].rounds++;
            if(schedPool[loopIndex].rounds > simParams.STARVE_THRESH)
            {
                schedule_onu(loopIndex,LAMBDA_NULL);
                /* Remove it from the pool */
                schedPool[loopIndex].state = ONU_SCHED_INACTIVE;
                schedPool[loopIndex].poolTime = -1;
                schedPoolCount--;
            }
        }
    }

    /* 
     * Determine grant sizing
     */
    for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        /* Skip over INACTIVE ONUs */
        if(schedPool[loopIndex].state == ONU_SCHED_INACTIVE)
            continue;
        /* Determine grant size for this ONU */
        grant_sizing(loopIndex);
        schedPool[loopIndex].gateLength = onuAttrs[loopIndex].grantLen*simParams.TIME_PER_BYTE;
    }
    /* Record the number of ONUs available for scheduling */
    record(schedPoolCount,numONUSched);

    /*
     * Initialize data structures needed for scheduling
     */
    for(loopIndex=0; loopIndex < MAX_LAMBDAS; loopIndex++)
    {
        scheduleListLfjSpt[loopIndex] = NULL;
        scheduleListLfjSpt[loopIndex] = NULL;
    }

    /*
       Schedule the ONUs in the pool according to LFJ-LPT, LFJ-SPT and WBM
     */
    /*
       For ACTIVE ONUs determine grant size and insert into a list sorted according to
       LFJ-LPT, LFJ-LNF, or LFJ-SPT
     */
    for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        /* Skip over INACTIVE ONUs */
        if(schedPool[loopIndex].state == ONU_SCHED_INACTIVE)
            continue;

        /* Fill out ONU list record for ONU */
        onuListItem.onuNum = loopIndex;
        onuListItem.grantLen = onuAttrs[loopIndex].grantLen;
        onuListItem.poolTime = schedPool[loopIndex].poolTime;
        onuListItem.numFrames = onuAttrs[loopIndex].rptQueueNum;
        onuListItem.minArrivalTime = onuAttrs[loopIndex].minArrivalTime;
        onuListItem.avgArrivalTime = onuAttrs[loopIndex].avgArrivalTime;
        onuListItem.latency = onuAttrs[loopIndex].latency;
        /* Insert ONU list record according to desired scheduling order */
        /* LFJ-SPT */
        scheduleListLfjSpt[onuAttrs[loopIndex].numSupportedLambdas-1] = 
            onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_LEN,SORT_POOL_TIME,&onuListItem,scheduleListLfjSpt[onuAttrs[loopIndex].numSupportedLambdas-1]);
        scheduleListLfjLpt[onuAttrs[loopIndex].numSupportedLambdas-1] = 
            onu_list_insert(SORT_DESCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_LEN,SORT_POOL_TIME,&onuListItem,scheduleListLfjLpt[onuAttrs[loopIndex].numSupportedLambdas-1]);
    }

    /*
       Tentatively Schedule the ONUs
     */

    /* Setup schedule for LFJ-SPT */
    mSETUP_TEMP_LAMBDA_FREE(loopIndex);
    /* Build linear schedule list */
    onuListCnt = 0;
    for(loopIndex=0; loopIndex < MAX_LAMBDAS; loopIndex++)
    {
        while(scheduleListLfjSpt[loopIndex] != NULL)
        {
            scheduleListLfjSpt[loopIndex] = onu_list_pop(scheduleListLfjSpt[loopIndex],&onuListItem);
            onuList[onuListCnt++] = onuListItem.onuNum;
        }
    }

    /* Update all Lambdas with available time < current time to current time */
    mUPDATE_LAMBDA_FREE(loopIndex,lambdaFreeTemp);

    /* Schedule the available ONUs according to generated list */
    for(loopIndex=0; loopIndex < onuListCnt; loopIndex++)
    {
        tentatively_schedule_onu(onuList[loopIndex],LAMBDA_NULL); /* Schedule ONU tentatively with no pre-selected lambda */
    }

    /* Dump schedule to file */

#ifdef DEBUG_TRC_LO
    printf("LFJ-SPT Schedule:\n");
    fflush(NULL);
    for(trcIdx=0; trcIdx < simParams.NUM_LAMBDAS; trcIdx++)
    {
        trcList[trcIdx] = NULL;
    }
    for(trcIdx=0; trcIdx < simParams.NUM_ONU; trcIdx++)
    {
        if((schedPool[trcIdx].state == ONU_SCHED_ACTIVE))
        {
            /* Place this ONU into wavelength list */
            trcOnuListItem.onuNum = trcIdx;
            trcOnuListItem.grantLen = onuAttrs[trcIdx].grantLen;
            trcOnuListItem.grantTime = schedPool[trcIdx].gateStart;
            trcOnuListItem.numFrames = onuAttrs[trcIdx].rptQueueNum;
            trcOnuListItem.poolTime = schedPool[trcIdx].poolTime;
            trcList[schedPool[trcIdx].gateLambda] = onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_TIME,SORT_POOL_TIME,&trcOnuListItem,trcList[schedPool[trcIdx].gateLambda]);
        }
    }
    for(trcIdx=0; trcIdx < simParams.NUM_LAMBDAS; trcIdx++)
    {
        printf("lambda %d:\n",trcIdx);
        while(trcList[trcIdx] != NULL)
        {
            trcList[trcIdx] = onu_list_pop(trcList[trcIdx],&trcOnuListItem);
            printf("ONU #%2d Start: %10.5e Length: %10.5e\n", trcOnuListItem.onuNum, trcOnuListItem.grantTime, (double)(trcOnuListItem.grantLen*simParams.TIME_PER_BYTE));
        }
    }

#endif


    /*
     * Initialize data structures needed for scheduling
     */
    mSETUP_TEMP_LAMBDA_FREE(loopIndex);
    for(loopIndex=0; loopIndex < MAX_LAMBDAS; loopIndex++)
    {
        scheduleList[loopIndex] = NULL;
    }

    /* 
    * Determine dimensions of cost matrix given ONUs in Scheduling Pool 
    */
    onuListCnt = 0;
    for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        /* Skip over INACTIVE ONUs */
        if(schedPool[loopIndex].state == ONU_SCHED_INACTIVE)
            continue;
        onuList[onuListCnt++] = loopIndex;
    }
    /* cost_matrix_dim = onuListCnt*simParams.NUM_LAMBDAS; */
    /*
     * Randomly Permute the ONU list and a list of lambdas
     */
    rand_permute_int(onuList, onuListCnt);
    for(loopIndex=0; loopIndex < simParams.NUM_LAMBDAS; loopIndex++)
    {
        lambdaPerm[loopIndex] = loopIndex;
    }
    rand_permute_int(lambdaPerm, simParams.NUM_LAMBDAS);

    /* 
     * Construct cost matrix
     */
    cost_matrix = (int**)calloc(onuListCnt,sizeof(int*));
    avgGrantSize = 0;
    for(loopIndex = 0; loopIndex < onuListCnt; loopIndex++)
    {
        cost_matrix[loopIndex] = (int*)calloc(onuListCnt*simParams.NUM_LAMBDAS,sizeof(int));
        avgGrantSize += onuAttrs[onuList[loopIndex]].grantLen+64;
    }
    avgGrantSize = avgGrantSize/onuListCnt;

    for(lambdaNum=0;lambdaNum < simParams.NUM_LAMBDAS; lambdaNum++)
    {
        /* Compute availability cost for wavelength (byte times until wavelength available) */
        lambda_avail_cost[lambdaNum] = MAX(((lambdaFree[lambdaNum] -  (simtime()+(max_rtt +(64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME)))*(1/simParams.TIME_PER_BYTE)),0);
    }
    for(loopIndex=0; loopIndex < onuListCnt; loopIndex++)
    {
        /* Prepare WBM matrix for this ONU */
        for(lambdaNum=0;lambdaNum < simParams.NUM_LAMBDAS; lambdaNum++)
        {
            for(posNum=0;posNum < onuListCnt; posNum++)
            {
                if(onuAttrs[onuList[loopIndex]].supportedLambdasMap[lambdaPerm[lambdaNum]] == LAMBDA_FALSE)
                {
                    /* lambda not supported */
                    cost_matrix[loopIndex][lambdaNum*onuListCnt+posNum] = COST_INFINITE;
                }
                else
                {
                    /* lambda supported */
                    if(onuAttrs[onuList[loopIndex]].grantLen > 0)
                    {
                        /* never allow the cost to be higher than a number slightly smaller than COST_INFINITE */
                        cost_matrix[loopIndex][lambdaNum*onuListCnt+posNum] = int_min(((onuListCnt-(posNum))*(onuAttrs[onuList[loopIndex]].grantLen+64) 
                            + lambda_avail_cost[lambdaPerm[lambdaNum]]),(COST_INFINITE-100));
                    }
                    else
                    {
                        cost_matrix[loopIndex][lambdaNum*onuListCnt+posNum] = (onuListCnt-(posNum))*64 + 
                            lambda_avail_cost[lambdaPerm[lambdaNum]] /* priority cost */;
                    }
                }
            }
        }
    }

    /* Solve weighted bipartite matching */
    hungarian_init(&myHungProb,cost_matrix,onuListCnt,onuListCnt*simParams.NUM_LAMBDAS,HUNGARIAN_MODE_MINIMIZE_COST);
    hungarian_solve(&myHungProb);

    /* 
     * Turn WBM solution into a tentative schedule 
     */
    /* Turn hungarian_solution structure into scheduleList structure */
    for(loopIndex=0;loopIndex < onuListCnt;loopIndex++)
    {
        colNum    = hungarian_assignment(&myHungProb,loopIndex);
        lambdaNum = lambdaPerm[colNum / onuListCnt];
        posNum    = colNum % onuListCnt;

        onuListItem.onuNum = onuList[loopIndex];
        onuListItem.posNum = posNum;
        onuListItem.grantLen = onuAttrs[onuListItem.onuNum].grantLen;
        onuListItem.numFrames = onuAttrs[onuListItem.onuNum].rptQueueNum;
        onuListItem.grantTime = 0;
        onuListItem.poolTime = schedPool[onuListItem.onuNum].poolTime;
        scheduleList[lambdaNum] = onu_list_insert(SORT_ASCENDING_ORDER, SORT_ASCENDING_ORDER, SORT_POS_NUM, SORT_POOL_TIME, &onuListItem, scheduleList[lambdaNum]);
    }

    /* Update all Lambdas with available time < current time to current time */
    mUPDATE_LAMBDA_FREE(loopIndex,lambdaFreeTemp);
    
    /* Tentatively schedule all the active ONUs according to WBM solution */
    for(loopIndex=0; loopIndex < MAX_LAMBDAS; loopIndex++)
    {
        while(scheduleList[loopIndex] != NULL)
        {
            scheduleList[loopIndex] = onu_list_pop(scheduleList[loopIndex],&onuListItem);
            /* Check if the wavelength assignment is valid */
            if(onuAttrs[onuListItem.onuNum].supportedLambdasMap[loopIndex] == LAMBDA_TRUE)
            {
                /* tentatively schedule this ONU on the selected wavelength */
                tentatively_schedule_onu(onuListItem.onuNum,loopIndex);
            }
            else
            {
                /* tentatively schedule this ONU on NASC */
                tentatively_schedule_onu(onuListItem.onuNum,LAMBDA_NULL);
            }
        }
    }
    hungarian_free(&myHungProb);
    for(loopIndex = 0; loopIndex < onuListCnt; loopIndex++)
    {
        free(cost_matrix[loopIndex]);
    }
    free(cost_matrix);

#ifdef DEBUG_TRC_LO
    printf("WBM Schedule:\n");
    fflush(NULL);
    for(trcIdx=0; trcIdx < simParams.NUM_LAMBDAS; trcIdx++)
    {
        trcList[trcIdx] = NULL;
    }
    for(trcIdx=0; trcIdx < simParams.NUM_ONU; trcIdx++)
    {
        if((schedPool[trcIdx].state == ONU_SCHED_ACTIVE))
        {
            /* Place this ONU into wavelength list */
            trcOnuListItem.onuNum = trcIdx;
            trcOnuListItem.grantLen = onuAttrs[trcIdx].grantLen;
            trcOnuListItem.grantTime = schedPool[trcIdx].gateStart;
            trcOnuListItem.numFrames = onuAttrs[trcIdx].rptQueueNum;
            trcOnuListItem.poolTime = schedPool[trcIdx].poolTime;
            trcList[schedPool[trcIdx].gateLambda] = onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_TIME,SORT_POOL_TIME,&trcOnuListItem,trcList[schedPool[trcIdx].gateLambda]);
        }
    }
    for(trcIdx=0; trcIdx < simParams.NUM_LAMBDAS; trcIdx++)
    {
        printf("lambda %d:\n",trcIdx);
        fflush(NULL);
        while(trcList[trcIdx] != NULL)
        {
            trcList[trcIdx] = onu_list_pop(trcList[trcIdx],&trcOnuListItem);
            printf("ONU #%2d Start: %10.5e Length: %10.5e\n", trcOnuListItem.onuNum, trcOnuListItem.grantTime, (double)(trcOnuListItem.grantLen*simParams.TIME_PER_BYTE));
            fflush(NULL);
        }
    }

#endif

    /*
     * Firmly schedule imminent ONUs
     */

    /* Generate an ONU list of imminent ONUs in time order */
    onuListCnt = 0;
    scheduleList[0] = NULL;
    for(loopIndex=0; loopIndex < simParams.NUM_ONU; loopIndex++)
    {
        if((schedPool[loopIndex].state == ONU_SCHED_ACTIVE) && 
           ((simtime() - schedPool[loopIndex].gateStart) < (onuAttrs[onuList[loopIndex]].rtt +(64*simParams.TIME_PER_BYTE)+simParams.PREAMBLE_IPG_TIME)))
        {
            schedPool[loopIndex].state = ONU_SCHED_IMMINENT;
            /* Insert into ordered ONU list */
            onuListCnt++;
            onuListItem.onuNum = loopIndex;
            onuListItem.grantLen = onuAttrs[loopIndex].grantLen;
            onuListItem.grantTime = schedPool[loopIndex].gateStart;
            onuListItem.numFrames = onuAttrs[loopIndex].rptQueueNum;
            onuListItem.poolTime = schedPool[loopIndex].poolTime;
            scheduleList[0] = onu_list_insert(SORT_ASCENDING_ORDER,SORT_ASCENDING_ORDER,SORT_GRANT_TIME,SORT_POOL_TIME,&onuListItem,scheduleList[0]);
        }
    }

    /* Setup transmission for imminent ONUs */
    for(loopIndex=0; loopIndex < onuListCnt; loopIndex++)
    {
        if(scheduleList[0] != NULL)
        {
            scheduleList[0] = onu_list_pop(scheduleList[0],&onuListItem);
        }
        firmly_schedule_onu(onuListItem.onuNum);
    }

}

/* 
 * FUNCTION: olt()
 * DESCRIPTION: Process model of an OLT
 *
 */
void olt()
{
	create("OLT");
#ifdef DEBUG_TRC_HI
	printf("OLT started\n");
#endif
	TSprint("OLT started\n");

	// Test Variables
	status_processes_print();
	
	/* Permanent OLT behavior */
	while(!terminateSim)
	{
		//~ TSprint("OLT round started\n");
		switch(simParams.OLT_TYPE)
		{
			case OLT_LFJ:
			case OLT_LFJ_LPT:
			case OLT_LFJ_SPT:
			case OLT_LFJ_LNF:
			case OLT_EAF:
			case OLT_EAAF:
			case OLT_WBM:
			case OLT_WBM_LPT:
			case OLT_WBM_LNF:
			case OLT_WBM_EAAF:
			case OLT_SPD:
			case OLT_LPD:
			case OLT_SPD_LPT:
				offline();
				break;
			case OLT_IPACT:
			case OLT_WDM_IPACT:
			case OLT_ONLINE_NASC:
			case OLT_LEAST_ASSIGNED:
			case OLT_IPACT_PSF:
				online();
				break;
			case OLT_ONLINE_INTERVAL_LFJ_LPT:
			case OLT_ONLINE_INTERVAL_LFJ_SPT:
			case OLT_ONLINE_INTERVAL_LFJ_LNF:
			case OLT_ONLINE_INTERVAL_WBM:
			case OLT_ONLINE_INTERVAL_WBM_LPT:
			case OLT_ONLINE_INTERVAL_WBM_LNF:
			case OLT_ONLINE_INTERVAL_EAF:
			case OLT_ONLINE_INTERVAL_EAAF:
			case OLT_ONLINE_INTERVAL_WBM_EAAF:
				online_interval();
				break;
			case OLT_ONLINE_JIT_LFJ_LPT:
			case OLT_ONLINE_JIT_LFJ_LNF:
			case OLT_ONLINE_JIT_LFJ_SPT:
			case OLT_ONLINE_JIT_WBM:
			case OLT_ONLINE_JIT_WBM_LPT:
			case OLT_ONLINE_JIT_WBM_LNF:
			case OLT_ONLINE_JIT_EAF:
			case OLT_ONLINE_JIT_EAAF:
			case OLT_ONLINE_JIT_WBM_EAAF:
			case OLT_ONLINE_JIT_SPD:
			case OLT_ONLINE_JIT_LPD:
				online_jit2();
				break;
			case OLT_ONLINE_JIT_TEST:
				online_jit_test();
				break;
			default:
				printf("FATAL ERROR: Invalid OLT Type!!\n");
				break;
		}
		//~ TSprint("OLT round completed\n");

	}	
	
}
