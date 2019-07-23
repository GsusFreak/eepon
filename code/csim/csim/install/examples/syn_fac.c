/* test clock synchronized facility */

#include "csim.h"

#define NJ 3
#define NC 2

FACILITY f;

void job();

void sim()
{
	int i;

	create("sim");
	f = facility_ms("f", 1l);
	synchronous_facility(f, 0.5, 1.0);

	for(i = 1; i <= NJ; i++)
		job(i);
	wait(event_list_empty);
	report();
}

void job(n)
int n;
{
	int i;

	create("job");
	set_priority((long)n);
	for(i = 0; i < NC; i++)
		use(f, 0.250);
}
