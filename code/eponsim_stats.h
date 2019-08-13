/*
	TITLE: 		EPON Simulator Statistics Function Definitions
	AUTHOR: 	Michael P. McGarry
	DATE:
	VERSION:	1.0
	
	NOTES:
	
*/

#ifndef EPONSIM_STATS_H
#define EPONSIM_STATS_H

/* Packet measurement functions */
void record_packet_stats_finish(sENTITY_PKT *pkt);
void record_packet_stats_dequeue(int onuNum);
void record_packet_stats_dequeue_tx_time(int onuNum);
void record_stats_queue_length(int onuNum);

void record_packet_stats_dequeue_minus_prop(int onuNum);
double rj_fairness_index(double * a, double * o, int n);
void reset_throughput_counters();
void onu_throughput_calc();


#endif
