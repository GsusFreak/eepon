/* try moving average */

#include "csim.h"
#include <stdio.h>

TABLE t1, t2;

void sim()
{
	int i, tc;

	t1 = histogram("moving", 10L, 1.0, 1001.0);
	t2 = table("summary");
	set_moving_table(t1, 100L);
	for(i = 1, tc = 1; i <= 1000; i++, tc++) {
		record((float) i, t1);
		record((float) i, t2);
		if(tc == 100) {
			report_tables();
			tc = 0;
			}
		}
	printf("\nTable %s is a %ld point moving window\n",
		table_name(t1), table_window_size(t1));
	printf("Table %s is a %ld point moving window\n",
		table_name(t2), table_window_size(t2));
}
