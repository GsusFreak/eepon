#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

#define LINE_LENGTH	256

void traffic_src_video();
int getline(FILE *input, char *line, int maxlen);

void main()
{
 traffic_src_video();
}

/* Process model of a Video traffic generator */
void traffic_src_video()
{
	int pktSize, flag, wordCount = 0;
	char mystring[LINE_LENGTH];
	//sENTITY_PKT *pktPtr;
	double frameTimeStamp;
	char frameType;
	int forecastSize, forecastPktNumber;
	int i, FFBI = 0;
	
	
	/* Initialize the process */
	//procId[0] = '\0';
	//sprintf(procId,"TrafficGen #%d",onuNum);
	//create(procId);

	char fileName[] = "Shortverbose.data.FFBI.encap";
	
	for (i=0; i < strlen(fileName) - 4; i++)
	{
		if (fileName[i] == 'F' && fileName[i+1] == 'F' && fileName[i+2] == 'B' && fileName[i+3] == 'I') 
		{
			FFBI = 1;
			break;
		}
	}
	
	
	FILE* inFilePtr = fopen(fileName,"r");
	if (inFilePtr == NULL)
	{
		printf("Error opening %s \n",fileName);
	}
	
	flag = getline(inFilePtr, mystring, LINE_LENGTH);
	
	while(flag > 0 && FFBI == 0)
	{
		
		printf("Current Flag:%d\n",flag);
		
		if (flag == 500) wordCount += 1;
		else if (flag == 1000 || flag == 2000) wordCount += 1;
		
		if (wordCount == 2) frameTimeStamp = atof(mystring);
		else if (wordCount == 3) frameType = mystring[0];
		else if (wordCount == 4) pktSize = atoi(mystring);
		
		if (wordCount == 8)
		{
			wordCount = 0;
			forecastSize = atoi(mystring);
			forecastPktNumber = ceil(forecastSize/1460.0);
			//pktPtr = create_a_video_packet(pktSize, frameType, frameTimeStamp, forecastSize);
			//assign_packet(pktPtr, onuNum);
			//hold(1.0/30.0);
		
			printf("timeStamp:%lf\n",frameTimeStamp);
			printf("frameType:%c\n",frameType);
			printf("pktSize:%d\n",pktSize);
			printf("forecastSize:%d\n",forecastSize);
			printf("forecastPktNumber:%d\n",forecastPktNumber);
		
		}
		
		
		if (flag != 2000) flag = getline(inFilePtr, mystring, LINE_LENGTH);
		else flag = 0;
	}
	
	
	while(flag > 0 && FFBI == 1)
	{
		
		printf("Current Flag:%d\n",flag);
		
		if (flag == 500) wordCount += 1;
		else if (flag == 1000 || flag == 2000) wordCount += 1;
		
		if (wordCount == 2) frameTimeStamp = atof(mystring);
		else if (wordCount == 3) frameType = mystring[0];
		else if (wordCount == 4) pktSize = atoi(mystring);
		else if (wordCount == 6) 
		{
			forecastSize = atoi(mystring);
			forecastPktNumber = ceil(forecastSize/1460.0);
		}
		
		if (wordCount == 8)
		{
			wordCount = 0;
			
			//pktPtr = create_a_video_packet(pktSize, frameType, frameTimeStamp, forecastSize);
			//assign_packet(pktPtr, onuNum);
			//hold(1.0/30.0);
		
			printf("timeStamp:%lf\n",frameTimeStamp);
			printf("frameType:%c\n",frameType);
			printf("pktSize:%d\n",pktSize);
			printf("forecastSize:%d\n",forecastSize);
			printf("forecastPktNumber:%d\n",forecastPktNumber);
		
		}
		
		
		if (flag != 2000) flag = getline(inFilePtr, mystring, LINE_LENGTH);
		else flag = 0;
	}
	
}

/*
   getline() takes and array and maximum length, reads in one line of input 
   - but not to exceed the maximum length - places the input in the array 
   and returns the length of the input.
*/
   
int getline(FILE *input, char *line, int maxlen)
{
	char c;              /* c is the next character, i the loop counter */
        int i;               /* read in until we have EOF or end-of-line    */
	
	for (i = 0; (i < maxlen-1) && ((c = fgetc(input)) != EOF) && (c != '\n'); i++)
        
		if (c != '\t')	line[i] = c;
		else 
		{
			line[i] = '\0';          /* terminate the string */
			return 500;
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
