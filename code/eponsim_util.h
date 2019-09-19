/*
	TITLE: 		EPON Simulator Utility Function Definitions
	AUTHOR: 	Michael P. McGarry
	DATE:
	VERSION:	1.0
	
	NOTES:
	
*/

#ifndef EPONSIM_UTIL_H
#define EPONSIM_UTIL_H

/* General utility functions */
int int_max(int A, int B);
int int_min(int A, int B);
void rand_permute_int(int *a, int size);
void rand_permute_double(double *a, int size);
void bsort_int_ascend(int numbers[], int array_size);
void bsort_int_descend(int numbers[], int array_size);
void bsort_double_ascend(double numbers[], int array_size);
void bsort_double_descend(double numbers[], int array_size);

int randrange(int N);
void grant_trace_flush();

void changeState(int onuNum, eONU_STATE stateNew);

/* Packet and GATE message utility functions */
sENTITY_PKT *create_a_packet(int size, int onuNum);
void remove_packet(int onuNum);
void remove_all_packets();
int  get_OLT_queue_size();
int  get_ONU_queue_size(int onuNum);


void grantCycle();
/* ONU list utility functions */
sONU_LIST *onu_list_insert(eSORT_METHOD sortMethod1, eSORT_METHOD sortMethod2, eSORT_CRITERIA sortCriteria1, 
	eSORT_CRITERIA sortCriteria2, sONU_LIST *newEntry, sONU_LIST *currentList);
sONU_LIST *onu_list_pop(sONU_LIST *currentList, sONU_LIST *poppedItem);
void onu_list_print(sONU_LIST *currentList);
eSORT_EQUAL check_equal(eSORT_CRITERIA sortCriteria, sONU_LIST *node1, sONU_LIST *node2);

/* Weighted Bipartite Matching Cost Matrix Utility Functions */
void debug_print_cost_matrix(int **cost_matrix, int rows, int cols);
void dump_cost_matrix(int **cost_matrix, int rows, int cols);


#endif
