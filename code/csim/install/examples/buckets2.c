/* test buckets, etc. */

#include "csim.h"

#define RES_LIM 16l
#define PROC_LIM 1024l

FACILITY f;
QTABLE fac_q;
QTABLE hold_q;
EVENT go;
void job();

void sim()
{
	long i;

	create("sim");
	max_processes(2048l);
	fac_q = qtable("busy q");
	hold_q = qtable("hold q");
	f = facility("f");
	go = event("go");
	hold(5000000.0);
	for(i = 0; i < PROC_LIM; i++) {
		hold(expntl(0.001));
		job(i);
		}
	set(go);
	wait(event_list_empty);
	report();
}

void job(n) long n;
{
	long i;

	create("job");
	hold(expntl(0.001));
	for(i = 0; i < RES_LIM; i++) {
		note_entry(fac_q);	
		use(f, expntl(0.001));
		note_exit(fac_q);
		note_entry(hold_q);
		hold(expntl(2.0));
		note_exit(hold_q);
		}
}
