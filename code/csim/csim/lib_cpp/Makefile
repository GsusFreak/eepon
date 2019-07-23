AR = ar

comp: cpp.o cpp_main.o buffer.o event.o facil.o machdep.o mbox.o meter.o monitor.o process.o qtable.o queueStat.o random.o reports.o store.o subs.o table.o utilHold.o xmain.o
	rm -f csim.cpp.a
	
	$(AR) q csim.cpp.a cpp.o cpp_main.o buffer.o event.o facil.o machdep.o mbox.o meter.o monitor.o process.o qtable.o queueStat.o random.o reports.o store.o subs.o table.o utilHold.o xmain.o
	ranlib csim.cpp.a
	

clean:
	rm -f csim.cpp.a core 
