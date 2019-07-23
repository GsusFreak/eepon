/* illustrate the reset statement */

#include "csim.h"
#include <stdio.h>

#define NUM_SERVERS 2l
#define WARM_UP 100.0
#define BATCH  1000.0
#define NUM_BATCHES 10

FACILITY f;

TABLE t;
TABLE batch_means;

void init();
void generator();
void job();

void sim()
{
	int ibatch;
	char str[36];

	create("sim");
	init();
	generator();
	hold(WARM_UP);
	reset();
	for(ibatch = 1; ibatch <= NUM_BATCHES; ibatch++) {
		hold(BATCH);
		sprintf(str, "batch %d", ibatch);
		set_model_name(str);
		record(table_mean(t), batch_means);
		report();
		reset();
		}
	report_table(batch_means);
}

void init()
{
	f = facility_ms("f", NUM_SERVERS);
	t = table("resp tm");
	batch_means = permanent_table("btch mns");
}

void generator()
{
	create("generator");
	do {
		hold(expntl(1.0));
		job();
	} while(1);
}

void job()
{
	TIME t1;
	
	create("job");
	t1 = clock;
	use(f, uniform(0.5, 1.5));
	record(clock - t1, t);
}
