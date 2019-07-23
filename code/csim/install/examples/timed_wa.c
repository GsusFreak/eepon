/* demonstrate timed_queue() */

#include "csim.h"
#include <stdio.h>

EVENT ev;
FILE *fp;

void proc();

void sim()
{
	long i;
    
    fp = fopen("xxx.out", "w");
    set_output_file(fp);
	create("sim");
	ev = event("ev");
	for(i = 0; i < 10; i++)
		proc(i);
	hold(5.0);
	set(ev);
	dump_status();
	hold(1.0);
}

void proc(n)
long n;
{
	long st;

	create("proc");
	st = timed_wait(ev, n*1.0);
	fprintf(fp, "proc.%ld: event %s\n",
		n, (st == EVENT_OCCURRED) ? "occurred" : "timed out");
}
