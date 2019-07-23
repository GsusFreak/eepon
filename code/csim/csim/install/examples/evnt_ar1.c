
/* test array of events */

#include "csim.h" 
#include <stdio.h>

#define N 50l

EVENT arr[N];
EVENT go;

int ct;
void init();
void job();

void sim()
{
	int i;

	create("sim");
	init();
	for(i = 0; i < N; i++) {
		job(i);
		hold(1.0);
		}
	for(i = 0; i < N; i++)
		set(arr[i]);
	wait(go);
	report();
}

void init()
{
	event_set(arr, "arr", N);
	go = event("go");
	ct = N;
}

void job(n) int n;
{
	long i;

	create("job");
	i = queue_any(arr);
	printf("job.%d, saw event_arr[%d]\n", n, i);
	if(--ct == 0)
		set(go);
}
