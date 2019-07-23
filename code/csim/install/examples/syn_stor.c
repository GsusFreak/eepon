/* test clock synchronized facility */

#include "csim.h"

#define NJ 3
#define NC 2

STORE s;

void job();

void sim()
{
	int i;

	create("sim");
	s = storage("s", 1l);
	synchronous_storage(s, 0.5, 1.0);

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
	for(i = 0; i < NC; i++) {
		allocate(1l, s);
			hold(0.250);
		deallocate(1l, s);
		}
}
