/* example of preempt-resume service discipline */

#include "csim.h"

FACILITY f;
void job();

void sim()
{
	int i;

	create("sim");
	f = facility_ms("f", 2l);
	set_servicefunc(f, pre_res);
	for(i = 0; i < 1000; i++) {
		job(i);
		hold(expntl(2.0));
		}
	wait(event_list_empty);
	report();
}

void job(n)
int n;
{
	create("job");
	set_priority((long)n);
	use(f, expntl(1.5));
}
