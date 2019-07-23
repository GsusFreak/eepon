/* example using qhistogram */

#include "csim.h"

#define N 1000

QTABLE ql;
FACILITY f;

void make_qhist();

void sim()
{
	int i;
	void job();

	create("sim");

	ql = qtable("qlength");
	qtable_histogram(ql,  10L, 0L, 10L);
	f = facility("f");
	for(i = 0; i < N; i++) {
		hold(expntl(2.0));
		job();
		}
	report_hdr();
	make_qhist(ql);
	report_qtables();
}

void job()
{
	create("job");

	note_entry(ql);
		use(f, expntl(1.5));
	note_exit(ql);
}

void make_qhist(q) QHIST q;

{
	long i;
	long num, cnt;
	double  b_cnt, sum;
	double e_time;
	char str[8];

	printf("\n\tqhistogram: %s\n", qtable_name(q));
	num = qhistogram_num(q);
	cnt = qtable_cnt(q);
	e_time = qhistogram_time(q);
	printf("elapsed time      %.3f\n", e_time);
	printf("current number    %ld\n", qtable_cur(q));
	printf("mean queue length %.3f\n", qtable_qlen(q));
	printf("max queue length  %ld\n", qtable_max(q));
	printf("number of entries %ld\n", cnt);
	printf("number of buckets %ld\n", num);
	sum = 0;
	for(i = 0; i <= num+1; i++) {
		b_cnt = qtable_histogram_bucket(q, i);
		sum += b_cnt;
		if(i < num+1)
			sprintf(str,"%5ld", i-1);
		else
			sprintf(str, ">= %ld", num);
		printf("%5s\t%.3lf\t%.3lf\t%.3lf\n",
			str, b_cnt, b_cnt/e_time, sum/e_time);
		}
	printf("sum = \t%.3lf\n", sum);
}

