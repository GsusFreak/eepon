/* illustrate use of process classes and simulate an M/M/1 queue
  (an open queue with exponential service times and interarrival intervals)
*/

#include "csim.h"
#include <stdio.h>

#define	SVTM	1.0	/*mean of service time distribution */
#define IATM	2.0	/*mean of inter-arrival time distribution */
#define NARS	5000	/*number of arrivals to be simulated*/

FACILITY f;		/*pointer for facility */
EVENT done;		/*pointer for counter */
TABLE tbl;		/*pointer for table */
QTABLE qtbl;		/*pointer for qhistogram */
CLASS cl[2];		/* pointers to classes */
int cnt;		/*number of active tasks*/
FILE *fp;
void cust();
void make_facility_report();

void sim()				/*1st process - named sim */
{
	int i;

	fp = fopen("xxx.out", "w");
	set_output_file(fp);
	set_model_name("M/M/1 Queue");
	create("sim");				/*required create statement*/

	f = facility_ms("facility", 2l);		/*initialize facility*/
	collect_class_facility(f);		/*collect class statistics*/
	done = event("done");			/*initialize event*/
	tbl = table("resp tms");		/*initialize table */
	qtbl = qhistogram("num in sys", 10l);	/*initialize qhistogram*/
	cl[0] = process_class("class 0");	/*initialize cust class */
	cl[1] = process_class("class 1");

	cnt = NARS;				/*initialize cnt*/
	for(i = 1; i <= NARS; i++) {
		hold(expntl(IATM));		/*hold interarrival*/
		cust();				/*initiate process cust*/
		}
	wait(done);				/*wait until all done*/
	report();				/*print report*/
	make_facility_report(f);
}

void cust()				/*process customer*/
{
	double t1;
 
	create("cust");				/*required create statement*/
	set_process_class(cl[random(0l,1l)]);

	t1 = clock;				/*time of request */
	note_entry(qtbl);			/*note arrival */
	reserve(f);				/*reserve facility f*/
		hold(expntl(SVTM));		/*hold service time*/
	release(f);				/*release facility f*/
	record(clock-t1, tbl);			/*record response time*/
	note_exit(qtbl);			/*note departure */
	cnt--;					/*decrement cnt*/
	if(cnt == 0)
		set(done);			/*if last arrival, signal*/
}

void make_facility_report(f)
FACILITY f;
{
	long i, n;

	fprintf(fp, "\n\tfacility %s\n", facility_name(f));
	fprintf(fp, "service disp    %s\n", service_disp(f));
	fprintf(fp, "service time    %.3f\n", serv(f));
	fprintf(fp, "utilization     %.3f\n", util(f));
	fprintf(fp, "throughput      %.3f\n", tput(f));
	fprintf(fp, "queue length    %.3f\n", qlen(f));
	fprintf(fp, "response time   %.3f\n", resp(f));
	fprintf(fp, "completions     %ld\n",  completions(f));
	fprintf(fp, "preempts        %ld\n", preempts(f));
	n = num_servers(f);
	fprintf(fp, "num servers     %d\n", n);
	if(n > 1) {
		for(i = 0; i < n; i++) {
			fprintf(fp, "\nserver %ld\n", i);
			fprintf(fp, "    service time    %.3f\n",
				server_serv(f, i));
			fprintf(fp, "    utilization     %.3f\n",
				server_util(f, i));	
			fprintf(fp, "    throughput      %.3f\n",
				server_tput(f, i));
			fprintf(fp, "    completions     %ld\n",
				server_completions(f, i));
			}
		}
	for(i = 0; i < 2; i++) {
		fprintf(fp, "\nclass %ld\n", i);
		fprintf(fp, "    service time     %.3f\n", class_serv(f, cl[i]));
		fprintf(fp, "    utilization      %.3f\n", class_util(f, cl[i]));
		fprintf(fp, "    throughput       %.3f\n", class_tput(f, cl[i]));
		fprintf(fp, "    queue length     %.3f\n", class_qlen(f, cl[i]));
		fprintf(fp, "    response time    %.3f\n", class_resp(f, cl[i]));
		fprintf(fp, "    completions      %ld\n", class_completions(f, cl[i]));
		}
}

