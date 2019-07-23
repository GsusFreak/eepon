/* test setjmp, longjmp */

#include "csim.h"
#include <setjmp.h>
#include <stdio.h>
jmp_buf sjbuf;
void my_err();
EVENT done;

void sim();
void job();
void my_err();

int main(argc, argv) int argc; char *argv[];
{
	int st;
	proc_csim_args(&argc, &argv);
	set_err_handler(my_err);
	if((st = setjmp(sjbuf)) == 0) {
		sim();
		}
	else {
		printf("error: st = %d: ", st);
		printf("%s\n", csim_error_msg(st));
		}

	return 0;
}

void sim()
{
	create("sim");
	done = event("done");
	hold(1.0);
	job();
	wait(done);
}

void job()
{
	create("job");
	hold(1.0);
	wait(done);
}

void my_err(n)
int n;
{
	printf("csim error %d\n", n);
	set_error_file(stdout);
	print_csim_error(n);
	longjmp(sjbuf, n);
}
