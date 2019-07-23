CC = gcc -m32 -Wall -g
CSIM = -I csim/lib

hungarian.o: hungarian.c hungarian.h
	$(CC) -c hungarian.c -o hungarian.o

eponsim_onu.o: eponsim_onu.c eponsim_onu.h eponsim.h eponsim_util.h eponsim_stats.h
	$(CC) $(CSIM) -c eponsim_onu.c -o eponsim_onu.o

eponsim_onu_trc_lo.o: eponsim_onu.c eponsim_onu.h eponsim.h eponsim_util.h eponsim_stats.h
	$(CC) $(CSIM) -DDEBUG_TRC_LO -c eponsim_onu.c -o eponsim_onu_trc_lo.o

eponsim_onu_trc.o: eponsim_onu.c eponsim_onu.h eponsim.h eponsim_util.h eponsim_stats.h
	$(CC) $(CSIM) -DDEBUG_TRC -c eponsim_onu.c -o eponsim_onu_trc.o

eponsim_onu_trc_hi.o: eponsim_onu.c eponsim_onu.h eponsim.h eponsim_util.h eponsim_stats.h
	$(CC) $(CSIM) -DDEBUG_TRC_HI -c eponsim_onu.c -o eponsim_onu_trc_hi.o

eponsim_olt.o: eponsim_olt.c eponsim_olt.h eponsim.h eponsim_util.h hungarian.h
	$(CC) $(CSIM) -c eponsim_olt.c -o eponsim_olt.o

eponsim_olt_trc_lo.o: eponsim_olt.c eponsim_olt.h eponsim.h eponsim_util.h hungarian.h
	$(CC) $(CSIM) -DDEBUG_TRC_LO -c eponsim_olt.c -o eponsim_olt_trc_lo.o

eponsim_olt_trc.o: eponsim_olt.c eponsim_olt.h eponsim.h eponsim_util.h hungarian.h
	$(CC) $(CSIM) -DDEBUG_TRC -c eponsim_olt.c -o eponsim_olt_trc.o

eponsim_olt_trc_hi.o: eponsim_olt.c eponsim_olt.h eponsim.h eponsim_util.h hungarian.h
	$(CC) $(CSIM) -DDEBUG_TRC_HI -c eponsim_olt.c -o eponsim_olt_trc_hi.o

eponsim_traffic.o: eponsim_traffic.c eponsim_traffic.h eponsim.h eponsim_util.h
	$(CC) $(CSIM) -c eponsim_traffic.c -o eponsim_traffic.o

eponsim_traffic_trc_lo.o: eponsim_traffic.c eponsim_traffic.h eponsim.h eponsim_util.h
	$(CC) $(CSIM) -DDEBUG_TRC_LO -c eponsim_traffic.c -o eponsim_traffic_trc_lo.o

eponsim_traffic_trc.o: eponsim_traffic.c eponsim_traffic.h eponsim.h eponsim_util.h
	$(CC) $(CSIM) -DDEBUG_TRC -c eponsim_traffic.c -o eponsim_traffic_trc.o

eponsim_traffic_trc_hi.o: eponsim_traffic.c eponsim_traffic.h eponsim.h eponsim_util.h
	$(CC) $(CSIM) -DDEBUG_TRC_HI -c eponsim_traffic.c -o eponsim_traffic_trc_hi.o
	
eponsim_prop.o: eponsim_prop.c eponsim_prop.h eponsim.h
	$(CC) $(CSIM) -c eponsim_prop.c -o eponsim_prop.o

eponsim_util.o: eponsim_util.c eponsim_util.h eponsim.h
	$(CC) $(CSIM) -c eponsim_util.c -o eponsim_util.o

eponsim_stats.o: eponsim_stats.c eponsim_stats.h eponsim.h
	$(CC) $(CSIM) -c eponsim_stats.c -o eponsim_stats.o

eponsim.o: eponsim.c eponsim.h eponsim_onu.h eponsim_olt.h eponsim_traffic.h eponsim_prop.h eponsim_util.h eponsim_stats.h
	$(CC) $(CSIM) -c eponsim.c -o eponsim.o

eponsim_trc_lo.o: eponsim.c eponsim.h eponsim_onu.h eponsim_olt.h eponsim_traffic.h eponsim_prop.h eponsim_util.h eponsim_stats.h
	$(CC) $(CSIM) -DDEBUG_TRC_LO -c eponsim.c -o eponsim_trc_lo.o

eponsim_trc.o: eponsim.c eponsim.h eponsim_onu.h eponsim_olt.h eponsim_traffic.h eponsim_prop.h eponsim_util.h eponsim_stats.h
	$(CC) $(CSIM) -DDEBUG_TRC -c eponsim.c -o eponsim_trc.o

eponsim_trc_hi.o: eponsim.c eponsim.h eponsim_onu.h eponsim_olt.h eponsim_traffic.h eponsim_prop.h eponsim_util.h eponsim_stats.h
	$(CC) $(CSIM) -DDEBUG_TRC_HI -c eponsim.c -o eponsim_trc_hi.o

eponsim: eponsim.o eponsim_onu.o eponsim_olt.o eponsim_traffic.o eponsim_prop.o eponsim_util.o eponsim_stats.o hungarian.o
	$(CC) eponsim.o eponsim_onu.o eponsim_olt.o eponsim_traffic.o eponsim_prop.o eponsim_util.o eponsim_stats.o hungarian.o csim/lib/csim.a -lm -o eponsim

eponsim_trc_lo: eponsim_trc_lo.o eponsim_onu_trc_lo.o eponsim_olt_trc_lo.o eponsim_traffic_trc_lo.o eponsim_prop.o eponsim_util.o eponsim_stats.o hungarian.o
	$(CC) eponsim_trc_lo.o eponsim_onu_trc_lo.o eponsim_olt_trc_lo.o eponsim_traffic_trc_lo.o eponsim_prop.o eponsim_util.o eponsim_stats.o hungarian.o csim/lib/csim.a -lm -o eponsim_trc_lo

eponsim_trc: eponsim_trc.o eponsim_onu_trc.o eponsim_olt_trc.o eponsim_traffic_trc.o eponsim_prop.o eponsim_util.o eponsim_stats.o hungarian.o
	$(CC) eponsim_trc.o eponsim_onu_trc.o eponsim_olt_trc.o eponsim_traffic_trc.o eponsim_prop.o eponsim_util.o eponsim_stats.o hungarian.o csim/lib/csim.a -lm -o eponsim_trc

eponsim_trc_hi: eponsim_trc_hi.o eponsim_onu_trc_hi.o eponsim_olt_trc_hi.o eponsim_traffic_trc_hi.o eponsim_prop.o eponsim_util.o eponsim_stats.o hungarian.o
	$(CC) eponsim_trc_hi.o eponsim_onu_trc_hi.o eponsim_olt_trc_hi.o eponsim_traffic_trc_hi.o eponsim_prop.o eponsim_util.o eponsim_stats.o hungarian.o csim/lib/csim.a -lm -o eponsim_trc_hi

clean: 
	rm *.o; rm eponsim
