/* test register variables */

#include "csim.h"
EVENT go;

sim()
{
	register int i, *k;
	register float jx;
	int aa;

	create("sim");
	go = event("go");
	i = 0; jx = 2000.0; aa = 3000; k = &aa;
	prt_vals("before loop", i, jx, aa, k);
	for(i = 0; i < 3; i++)
		proc(i);
	i = 0;
	prt_vals("after loop", i, jx, aa, k);
	hold(1.0);
	set(go);
	wait(event_list_empty);
	prt_vals("fini", i, jx, aa, k);
}

proc(n)
int n;
{
	register int i, *k;
	register float jx;
	int aa;

	create("proc");
	i = n; jx = 2000.0 + n; aa = 3000 + n; k = &aa;
	prt_vals("  proc: before wait", i, jx, aa, k);
	wait(go);
	prt_vals("  proc: after wait", i, jx, aa, k);
	hold(100.0);
	prt_vals("  proc: after hold", i, jx, aa, k);
}

prt_vals(s, i, jx, aa, k)
char *s; int i, aa, *k; float jx;
{
	printf("%s: i = %d, jx = %.1f, aa = %d, *k = %d\n",
		s, i, jx, aa, *k);
}
