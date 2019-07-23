/*  example using storages  */

#include "csim.h"
#include <stdio.h>

#define N 100

#define MAXSTORE 0x1000l
#define MINREQ 0x100l
#define MAXREQ 0x800l

STORE s;
TABLE amt_tbl;
TABLE srv_tm;
QTABLE wait_tm;
FILE *fp;

void job();
void make_storage_report();

void sim()
{
	int i;
    
    fp = fopen("xxx.out", "w");
    set_output_file(fp);
	create("sim");
	s = storage("s", MAXSTORE);
	amt_tbl = table("amt requested");
	srv_tm = table("service times");
	wait_tm = qtable("waiting times");
	for(i = 0; i < N; i++) {
		job();
		hold(expntl(10.0));
		}
	wait(event_list_empty);
	report();
	make_storage_report();
	mdlstat();
}

void job()
{
	long amt;
	double x;

	create("job");
	amt = random(MINREQ, MAXREQ);
	record((double) amt, amt_tbl);
	note_entry(wait_tm);
	allocate(amt, s);
		x = expntl(20.0);
		record(x, srv_tm);
		hold(x);
	deallocate(amt, s);
	note_exit(wait_tm);
}

void make_storage_report()
{
	long req_cnt, cap;
	double t, w;

	fprintf(fp, "\n\tStorage Report: %s\n", storage_name(s));
	cap = storage_capacity(s);
	fprintf(fp, "    capacity      %ld\n", cap);
	req_cnt = storage_request_cnt(s);
	fprintf(fp, "    amt           %.3f\n",
		 (double)storage_request_amt(s)/req_cnt);
	t = storage_time(s);
	fprintf(fp, "    util          %.3f\n", storage_busy_amt(s)/(t*cap));
	fprintf(fp, "    srv_tm        %.3f\n",
		(double)storage_number_amt(s)/req_cnt);
	w = storage_waiting_amt(s);
	fprintf(fp, "    qlen          %.3f\n", w/t);
	fprintf(fp, "    resp          %.3f\n",
		(double)w/req_cnt);
	fprintf(fp, "    completes     %ld\n", storage_release_cnt(s));
	fprintf(fp, "    queued        %ld\n", storage_queue_cnt(s));
}
	
