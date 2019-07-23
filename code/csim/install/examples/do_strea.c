/* random number streams */

#include "csim.h"

STREAM s1;
STREAM s2;
HIST t1, t2;

#define EXP

void init();

void sim()
{
	int i;
	double x1, x2;
	int i1, i2;

	create("sim");
	init();
	for(i = 0; i < 1000; i++) {
#ifdef EXP
		x1 = stream_expntl(s1, 1.0);
		x2 = stream_expntl(s2, 1.0);
#endif
#ifdef UNI
		x1 = stream_uniform(s1, 1.0, 2.0);
		x2 = stream_uniform(s2, 1.0, 2.0);
#endif
#ifdef ERL
		x1 = stream_erlang(s1, 2.0, 5.0);
		x2 = stream_erlang(s2, 2.0, 5.0);
#endif
#ifdef HYP
		x1 = stream_hyperx(s1, 1.0, 4.0);
		x2 = stream_hyperx(s2, 1.0, 4.0);
#endif
#ifdef NOR
		x1 = stream_normal(s1, 0.0, 1.0);
		x2 = stream_normal(s2, 0.0, 1.0);
#endif
#ifdef RAN
		i1 = stream_random(s1, 1l, 10l);
		i2 = stream_random(s2, 1l, 10l);
		x1 = (double) i1;
		x2 = (double) i2;
#endif
		record(x1, t1);
		record(x2, t2);
		}
	report_tables();
}

void init()
{
	t1 = histogram("t1", 10l, 0.0, 5.0);
	t2 = histogram("t2", 10l, 0.0, 5.0);
	s1 = stream_init1(1l);
	s2 = stream_init1(2l);
}
