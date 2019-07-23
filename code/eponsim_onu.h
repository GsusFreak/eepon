/*
	TITLE: 		EPON Simulator ONU Models Function Definitions
	AUTHOR: 	Michael P. McGarry
	DATE:
	VERSION:	1.0
	
	NOTES:
	
*/

#ifndef EPONSIM_ONU_H
#define EPONSIM_ONU_H

/* ONU model */
void onu(int onuNum, int lambdaNum);

void check_data_packet_list(int onuNum);
void check_video_packet_list(int onuNum);

#endif
