/*
	TITLE: 		EPON Simulator Traffic Models Function Definitions
	AUTHOR: 	Michael P. McGarry
	DATE:
	VERSION:	1.0
	
	NOTES:
	
*/

#ifndef EPONSIM_TRAFFIC_H
#define EPONSIM_TRAFFIC_H

#define LINE_LENGTH	256

 /* Traffic models */
void traffic_src_poisson(int onuNum);
void traffic_src_self_similar(int onuNum, int streamNum);
void traffic_agg_self_similar(int onuNum);
void traffic_src_video(int onuNum, char[]);
void traffic_scalable_video(int onuNum, char[]);
int get_line(FILE *input, char *line, int maxlen);
int get_line2(FILE *input, char *line, int maxlen);

#endif
