/*  process classes and current class */

#include "csim.h"
#include <stdio.h>

#define N 3L
#define NJ 9

CLASS cl[N];
FILE *fp;

void job();

void sim()
{
	long i;
	char str[12];

	create("sim");
	fp = fopen("xxx.out", "w");
	set_output_file(fp);
	for(i = 0; i < N; i++) {
		sprintf(str, "class.%d", i);
		cl[i]  = process_class(str);
		}
	for(i = 0; i < NJ; i++)
		job(i);
	wait(event_list_empty);
	report();
}

void job(n)
long n;
{
	long c;

	create("job");
	c = random(0L, N-1);
	set_process_class(cl[c]);
	fprintf(fp, "%10.3f - start job.%ld: c = %ld, class = %s\n",
		clock, n, c, class_name(current_class()));
	hold(1.0);
	fprintf(fp, "%10.3f - end job.%ld: c = %ld, class = %s\n",
		clock, n, c, class_name(current_class()));
}
