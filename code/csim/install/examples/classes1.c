/* another examples using classes */

#include "csim.h"
#include <stdio.h>

#define SIMTIME 10000.0

#define N 3l
#define K 3l

FACILITY f[N];
FACILITY term;
CLASS c[K];

double  serv_tm[K][N] =
	{ {0.1, 0.2, 0.3},
	  {0.4, 0.8, 0.0},
	  {0.6, 0.0, 1.2} };

double term_tm[K] = {5.0, 10.0, 15.0};

double branch_prob[K][N][N] =
	{ { {0.5, 0.3, 0.2}, {1.0, 0.0, 0.0}, {1.0, 0.0, 0.0} },
	  { {0.1, 0.9, 0.0}, {1.0, 0.0, 0.0}, {0.0, 0.0, 0.0} },
	  { {0.2, 0.0, 0.8}, {0.0, 0.0, 0.0}, {1.0, 0.0, 0.0} } };

long n[K] = {20, 30, 40};

char str[48];

void job();
void print_parms();
void make_class_report();

void sim()
{
	long i, k;

	create("sim");
	facility_set(f, "f", N);
	term = facility("term");
	set_servicefunc(term, inf_srv);
	for(k = 0; k < K; k++) {
		sprintf(str, "c%d", (int)k);
		c[k] = process_class(str);
		}
	collect_class_facility_all();
	print_parms();
	for(k = 0; k < K; k++)
		for(i = 0; i < n[k]; i++)
			job(k);
	wait(event_list_empty);
	report();
	make_class_report();
	mdlstat();
}

void job(k)
long k;
{
	long i, j;
	long select();

	create("job");
	set_process_class(c[k]);
	j = 0;
	do {
		i = j;
		use(f[i], serv_tm[k][i]);
		j = select(i, k);
		if(j == 0)
			use(term, term_tm[k]);
	} while (clock < SIMTIME);
}

long select(i, k)
long i, k;
{
	long j;
	double x, y;

	x = prob();
	j = 0;
	y = branch_prob[k][i][j];
	while(y < x) {
		j++;
		y += branch_prob[k][i][j];
		}
	return(j);
}

void print_parms()
{
	long i, j, k;

	printf("service times\n");
	for(k = 0; k < K; k++) {
		printf("class %d: ", k);
		for(i = 0; i < N; i++)
			printf("%5.3f ", serv_tm[k][i]);
		printf("\n");
		}
	printf("\nbranching probabilities\n");
	for(k = 0; k < K; k++) {
		printf("class %d:\n", k);
		for(i = 0; i < N; i++) {
			printf("from dev[%d] to: ", i);
			for(j = 0; j < N; j++)
				printf("%5.3f ", branch_prob[k][i][j]);
			printf("\n");
			}
		}
	printf("\nclass populations: ");
	for(k = 0; k < K; k++)
		printf("%5d ", n[k]);
	printf("\n");
	printf("\nterminal delays: ");
	for(k = 0; k < K; k++)
		printf("%6.3f ", term_tm[k]);
	printf("\n");
}

void make_class_report()
{
	long k, ct;

	printf("\n\tClass Report\n");
	for(k = 0; k < K; k++) {
		printf("%ld %s\n", class_id(c[k]), class_name(c[k]));
		ct = class_cnt(c[k]);
		printf("    number of instances     %d\n", ct);
		printf("    hold count per instance %.3f\n", (double)
			class_holdcnt(c[k])/ct);
		printf("    life time per instance  %.3f\n",
			class_lifetime(c[k])/ct);
		printf("    hold time per instance  %.3f\n",
			class_holdtime(c[k])/ct);
		printf("    wait time per instance  %.3f\n",
			(class_lifetime(c[k]) - class_holdtime(c[k]))/ct);
		printf("\n");
		}
}
