#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <sgtty.h>

#define LINE_LENGTH	256

void traffic_src_video();
int getline(FILE *input, char *line, int maxlen);

int main()
{
 traffic_src_video();
}

/* Process model of a Video traffic generator */
void traffic_src_video()
{
	int pktSize, pktTemp, flag, wordCount = 0;
	char mystring[LINE_LENGTH];
	//sENTITY_PKT *pktPtr;
	double frameTimeStamp;
	char frameType;
	int forecastSize, forecastPktNumber;
	
	
	/* Initialize the process */
	//procId[0] = '\0';
	//sprintf(procId,"TrafficGen #%d",onuNum);
	//create(procId);

	char fileName[] = "shorttestTrace.data.FFBI.encap";
	
	FILE* inFilePtr = fopen(fileName,"r");
	if (inFilePtr == NULL)
	{
		printf("Error opening %s \n",fileName);
	}
	
	flag = getline(inFilePtr, mystring, LINE_LENGTH);
	
	
	while(flag > 0)
	{
		printf("Current Flag:%d\n",flag);
		if (flag == 500 || flag == 1000 || flag == 2000)
		{
			wordCount += 1;
			printf("Word Count:%d\n",wordCount);		
			if (wordCount == 2) frameTimeStamp = atof(mystring);
			else if (wordCount == 3) frameType = mystring[0];
			else if (wordCount == 4) pktSize = atoi(mystring);
			else if (wordCount == 5) pktSize = pktSize + atoi(mystring);
			else if (wordCount == 6)
			{
				pktTemp = atoi(mystring);
				if (flag == 1000)
				{
					wordCount = 0;
					forecastSize = 0;
					forecastPktNumber = 0;
					printf("timeStamp:%lf\n",frameTimeStamp);
					printf("frameType:%c\n",frameType);
					printf("pktSize:%d\n",pktSize);
					printf("forecastSize:%d\n",forecastSize);
					printf("forecastPktNumber:%d\n",forecastPktNumber);
					tty_getchar();

				}
			}
			else if (wordCount == 7) forecastSize = pktTemp + atoi(mystring);
			else if (wordCount == 8) forecastPktNumber = atoi(mystring);
			else if (wordCount == 9)
			{
				wordCount = 0;
				printf("timeStamp:%lf\n",frameTimeStamp);
				printf("frameType:%c\n",frameType);
				printf("pktSize:%d\n",pktSize);
				printf("forecastSize:%d\n",forecastSize);
				printf("forecastPktNumber:%d\n",forecastPktNumber);
				tty_getchar();
			}
		}
		if (flag != 2000) flag = getline(inFilePtr, mystring, LINE_LENGTH);
		else 
		{
			flag = 0;
		}

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
        {
        	//printf("c = %c,	i = %d\n",c,i);
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

int tty_getchar()
{
	return getchar();
}
	
