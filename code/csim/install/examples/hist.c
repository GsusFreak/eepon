/*  example of using lots of histogram functions */

#include "csim.h"
#include <stdio.h>

HIST h1;
FILE *fp;

void make_histogram();
void make_table();

void sim()
{
	HIST h2;
	int i; double x;

	fp = fopen("xxx.out", "w");
	set_output_file(fp);
	create("sim");
	h1 = histogram("h1", 10l, 0.0, 10.0);
	h2 = histogram("h2", 10l, 0.0, 10.0);
	for(i = 0; i < 1000; i++) {
		x = expntl(1.0);
		record(x, h1);
		record(x, h2);
		}
	report_tables();
	make_table(h1);
	make_histogram(h1);
	make_table(h2);
	make_histogram(h2);
}

void make_table(t)
TABLE t;
{
	long ct;
	double sum, sumsq, minv, maxv;
	double mn, var;

	ct = table_cnt(t);
	sum = table_sum(t);
	sumsq = table_sum_square(t);
	minv = table_min(t);
	maxv = table_max(t);
	mn = (ct <= 0) ? 0.0 : sum/ct;
	var = (ct <= 1) ? 0.0 : (sumsq - ct*mn*mn)/(ct - 1);
	fprintf(fp, "\n\ttable %s\n", table_name(t));
	fprintf(fp, "number of entries %d\n", ct);
	fprintf(fp, "sum of values     %.3f\n", sum);
	fprintf(fp, "sum of squares    %.3f\n", sumsq);
	fprintf(fp, "mean              %.3f\n", mn);
	fprintf(fp, "variance          %.3f\n", var);
	fprintf(fp, "minimum           %.3f\n", minv);
	fprintf(fp, "maximum           %.3f\n", maxv);
}

void make_histogram(h)
HIST h;
{
	long i, n, sum, c;
	double high, low, width, b;

	n = histogram_num(h);
	sum = table_cnt(h);
	low = histogram_low(h);
	high = histogram_high(h);
	width = histogram_width(h);

	fprintf(fp, "\n\thistogram %s\n", table_name(h));
	fprintf(fp, "number of buckets %d\n", n);
	fprintf(fp, "value of lowest   %.3f\n", low);
	fprintf(fp, "value of highest  %.3f\n", high);
	fprintf(fp, "width of bucket   %.3f\n", width);
	fprintf(fp, "number of entries %d\n", sum);
	b = low;
	for(i = 0; i < n+1; i++) {
		c = histogram_bucket(h, i);;
		fprintf(fp, "number < %.3lf = %ld (%.3lf %%)\n",
			b, c, 100.0*c/sum);
		b += width;
		}
	c = histogram_bucket(h, n+1);
	fprintf(fp, "number >= %.3f = %ld (%.3f %%)\n",
		high, c, (100.0*c)/sum);
}
