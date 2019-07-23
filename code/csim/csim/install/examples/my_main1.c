/* illustrate use of my main */

#include "csim.h"
#include <stdio.h>

#define N 10l
 
FACILITY cpu;
FACILITY disk[N];
TABLE resp_tm;
TABLE overall;
FILE *fp;

void sim();
void job();
void doio();

int main(argc, argv)
{
	proc_csim_args(&argc, &argv);
	fp = fopen("xxx.out", "w");
	set_output_file(fp);
	overall = permanent_table("overall resp");
	sim();
	sim();
	report_table(overall);
	return 0;
}

void sim()
{
	int i;

	cpu = facility("cpu");
	facility_set(disk,"disk", N);
	resp_tm = table("response time");
	create("sim");
	for(i = 0; i < N; i++) {
		job(i);
		hold(expntl(1.0));
		}
	wait(event_list_empty);
	record(table_mean(resp_tm), overall);
	report();
	rerun();
	reset_prob(1l);
}

void job(n)
int n;
{
	EVENT iodone;
	TIME t;

	create("job");
	t = clock;
	iodone = event("iodone");
	doio(iodone);
	use(cpu, expntl(0.7));
	wait(iodone);
	record(clock-t, resp_tm);
}

void doio(ev)
EVENT ev;
{
	long i;

	create("io");
	i = random(0l, N-1);
	use(disk[i], expntl(1.0));
	set(ev);
}
