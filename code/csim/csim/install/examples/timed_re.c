/* timed reserve */

#include "csim.h"
#include <stdio.h>

#define RUNTIME 10.0
#define INARTM 1.0
#define SERVTM 2.0

FACILITY f;
EVENT done;
int num_act;
int num_rej;

void call();

void sim()
{
	create("sim");
	done = event("done");
	f = facility("f");
	num_rej = 0;
	num_act = 0;
	while(clock < RUNTIME) {
		num_act++;
		call();
		hold((INARTM));
		}
	wait(done);
	report();
	printf("\nNumber of rejects = %d\n", num_rej);
}

void call()
{
	create("call");
	if(timed_reserve(f, 1.0) != TIMED_OUT) {
		hold((SERVTM));
		release(f);
		}
	else {
		num_rej++;
		}
	if(--num_act == 0 && clock >= RUNTIME)
		set(done);
}
