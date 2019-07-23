/* round robin service without priorities */

#include "csim.h"

FACILITY f;
TABLE st;

void high_task();
void low_task();

void sim()
{
	int i;
	
	create("sim");

	f = facility("f");
	set_servicefunc(f, rnd_rob);
	set_timeslice(f, 0.5);
	st = table("serv tm");

	high_task();
	for(i = 0; i < 3; i++)
		low_task();
	wait(event_list_empty);
	report();
}

void high_task()
{
	int i;

	create("high");
	set_priority(2L);
	for(i = 0; i < 5; i++) {
		hold(1.0);
		use(f, 1.0);
		record(1.0, st);
		}
}

void low_task()
{
	int i;

	create("low");
	set_priority(1L);
	for(i = 0; i < 5; i++) {
		use(f, 1.0);
		record(1.0, st);
		hold(1.0);
		}
}
