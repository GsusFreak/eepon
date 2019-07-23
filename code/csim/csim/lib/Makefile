AR = ar

comp: buffer.o event.o facil.o machdep.o main.o mbox.o meter.o monitor.o process.o qtable.o queueStat.o random.o reports.o store.o subs.o table.o utilHold.o xmain.o
	rm -f csim.a
	$(AR) q csim.a buffer.o event.o facil.o machdep.o main.o mbox.o meter.o monitor.o process.o qtable.o queueStat.o random.o reports.o store.o subs.o table.o utilHold.o xmain.o
	ranlib csim.a

clean:
	rm -f csim.a core
