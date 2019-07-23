/* confidence intervals */

#include "csim.h"

TABLE tbl;
void gen();

void sim()
{
	create("sim");
	tbl = table("tbl");
	table_histogram(tbl, 10L, 0.0, 10.0);
	table_confidence(tbl);
	table_run_length(tbl, 0.05, 0.95, 10.0);
	gen();
	wait(converged);
	report();
}

void gen()
{
	create("gen");
	while(1) {
		record(expntl(1.0), tbl);
		hold(0.0);
		}
}
