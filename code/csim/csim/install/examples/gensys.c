/* CSIM Example of General Computer System */
/* The model simulates a system with memory,
	a cpu, and disk drives.  Jobs arrive (are
	generated), queue for memory, and then compete
	for the cpu and disk drives.
*/

#include "csim.h"
#include <stdio.h>
#define NJOBS	2000L		/*number of jobs*/
#define NCPUS	2L			/*number of cpu's*/
#define NDISKS	4L			/*number of disks*/
#define AMTMEM	100L		/*amount of memory*/

#define INTARV	1.0			/*job interarrival interval*/
#define MNCPU	0.25		/*mean cpu interval*/
#define MNDSK	0.030		/*mean disk service interval*/

#define TOTCPU	1.0			/*total cpu time*/
#define EPS	0.0005			/*tolerance */

FACILITY cpu;				/*pointer to cpu facility */
FACILITY disk[NDISKS];		/*pointers to disk drives*/
STORE mem;					/*pointer to memory */
EVENT done;					/*pointer to event */
TABLE resp_tm;				/*pointer to statistics table */
QTABLE sys_q;				/*pointer to system qlen table */
int act;					/*count of active jobs */
FILE *fp;
void job();
void io();

void sim()					/*1st process - job generator */
{
	long i;

	fp = fopen("xxx.out", "w");
	set_output_file(fp);
	create("sim");

	/* declare resources and tables */

	cpu = facility_ms("cpu", NCPUS);
	set_servicefunc(cpu, pre_res);
	facility_set(disk, "disk", NDISKS);
	mem = storage("memory", AMTMEM);
	done = event("done");
	resp_tm = table("resp time");
	sys_q = qtable("system que");
	max_events(1000l);

	/* generate arrivals */

	act = NJOBS;
	for(i = 1; i <= NJOBS; i++) {
		job(i);					/*initiate job process */
		hold(expntl(INTARV));	/*hold interarrival interval */
		}
	wait(done); 				/* wait until all jobs completed */
	report();					/* print report */
}

void job(i)						/* a job process */
long i;
{
	long j, amt;
	EVENT iodone;
	double cpt, x;
	TIME t;

	create("job");
	set_priority(i);			/*set job priority */
	iodone = event("iodone");	/*declare local event */

	t = clock;					/*save start time */
	amt = random(5l, AMTMEM/2);	/*select size memory req. */
	note_entry(sys_q);			/*enter system queue */
	allocate(amt, mem);			/*request (allocate) memory*/
	cpt = erlang(TOTCPU, 0.5*TOTCPU); /*select cpu time req.*/
	while(cpt > EPS) {
		j = random(0l, NDISKS-1);/*select disk drive */
		io(j, iodone);			/*initiate I/O process */
		x = hyperx(MNCPU, 4.0*MNCPU); /*select next cpu intrvl*/
		cpt -= x;
		use(cpu,x);				/*use cpu x time units*/
		wait(iodone);			/*wait for I/O to complete*/
		}
	deallocate(amt, mem);		/*return memory*/
	record(clock - t, resp_tm);	/*record response time */
	note_exit(sys_q);			/*exit system queue */
	if(--act == 0)
		set(done);				/*if last job, signal sim*/
}

void io(d, ev)			/* I/O process */
long d; EVENT ev;
{
	create("io");

	reserve(disk[d]);			/*reserve disk */
		hold(expntl(MNDSK));	/*disk service interval*/
	release(disk[d]);			/*release disk*/
	set(ev);					/*signal I/O complete*/
}
