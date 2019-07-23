/* test array of events */

#include "csim.h"
#include <stdio.h>
FILE *fp;

#define N 50l

EVENT arr[N];
EVENT go;

int ct;
void init();
void job();

void sim()
{
	int i;

	fp = fopen("xxx.out", "w");
	set_output_file(fp);
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
	event_set_monitor(arr);
	go = event("go");
	ct = N;
}

void job(n) int n;
{
	long i;

	create("job");
	i = wait_any(arr);
	fprintf(fp, "job.%d, saw event_arr[%d]\n", n, i);
	if(--ct == 0)
		set(go);
}
