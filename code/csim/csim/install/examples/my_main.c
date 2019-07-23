/* illustrate use of my main */

#include "csim.h"
#include <stdio.h>

#define N 10l
#define POP 10
 
FACILITY cpu;
FACILITY disk[N];
CLASS job_cl;
CLASS io_cl;
FILE *fp;

void sim();
void job();
void doio();

int main(argc, argv)
int argc; char* argv[];
{
	fp = fopen("xxx.out", "w");
	set_output_file(fp);
	set_trace_file(fp);
        set_error_file(fp);
	proc_csim_args(&argc, &argv);
	sim();
	sim();
	sim();

	return 0;
}

void sim()
{
	int i;

	create("sim");
	cpu = facility("cpu");
	facility_set(disk,"disk", N);
	job_cl = process_class("job class");
	io_cl = process_class("io class");
	collect_class_facility_all();
	for(i = 0; i < POP; i++) {
		job(i);
		hold(expntl(1.0));
		}
	wait(event_list_empty);
	report();
	rerun();
	reset_prob(1l);
}

void job(n)
int n;
{
	EVENT iodone;

	create("job");
	set_process_class(job_cl);
	iodone = event("iodone");
	doio(iodone);
	use(cpu, expntl(0.7));
	wait(iodone);
}

void doio(ev)
EVENT ev;
{
	long i;

	create("io");
	set_process_class(io_cl);
	i = random(0l, N-1);
	use(disk[i], expntl(1.0));
	set(ev);
}
