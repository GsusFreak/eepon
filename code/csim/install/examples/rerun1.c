/* illustrates use of the rerun statement */

#include "csim.h"
#include <stdio.h>

#define NUM_RUNS 5
#define RUNTIME 1000.0

FACILITY f;
TABLE t;
TABLE perm_t;
FILE *fp;

void job();

void sim()
{
	int irun;

	fp = fopen("xxx.out", "w");
	set_output_file(fp);
	perm_t = permanent_table("overall resp");
	for(irun = 1; irun <= NUM_RUNS; irun++) {
		create("sim");
		f = facility("f");
		t = table("resp tm");
		while(clock < RUNTIME) {
			hold(expntl(1.0));
			job();
			}
		record(table_mean(t), perm_t);
		report();
		rerun();
		}
	report_hdr();
	report_table(perm_t);
}	

void job()
{
	TIME t1;

	create("job");
	t1 = clock;
	reserve(f);
		hold(hyperx(0.5, 0.75));
	release(f);
	record(clock - t1, t);
}
