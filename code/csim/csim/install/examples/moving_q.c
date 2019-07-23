/* test qtables */

#include "csim.h"
#include <stdio.h>

#define NJOBS 100

QTABLE qt, qt_perm;
FACILITY f;
EVENT done;
int nact;
void job();
FILE *fp;

void sim()
{
	int i;
    
	fp = fopen("xxx.out", "w");
	set_output_file(fp);
	create("sim");
	qt = qhistogram("qlen", 100l);
	qt_perm = qtable("ql perm");
	set_moving_qtable(qt, 10l);
	f = facility("f");
	done = event("done");

	nact = NJOBS;
	for(i = 1; i <= NJOBS; i++) {
		hold(1.0);
		job(i);
		}
	wait(done);
	report();
	fprintf(fp, "\nQhistogram %s is %ld point moving window qhistogram\n",
		qtable_name(qt), qtable_window_size(qt));
	fprintf(fp, "Qtable %s is %ld point moving window qtable\n",
		qtable_name(qt_perm), qtable_window_size(qt_perm));
}

void job(n) int n;
{
	create("job");
	note_entry(qt); note_entry(qt_perm);
		use(f, (float) n);
	note_exit(qt); note_exit(qt_perm);
	if(n % 50 == 0) {
		report_qtables();
		}
	if(--nact == 0)
		set(done);
}
