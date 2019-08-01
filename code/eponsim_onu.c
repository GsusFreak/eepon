
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
 * FUNCTION: online()
 * DESCRIPTION: Online OLT scheduler that employs Online (or one ONU at a time) scheduling
 *              Follows "scheduling jobs one at a time" online classification
 *
 */

void onu()
{
	create("ONU");
	// printf("OLT started\n");

	// Test Variables
	status_processes_print();
	
	// /* Permanent OLT behavior */
	// while(!terminateSim)
	// {
  //   online();
	// }	
}


