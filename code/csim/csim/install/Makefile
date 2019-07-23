CC = gcc
LDFLAGS =
CFLAGS =
CPPFLAGS =
MAKE = make

all: ex1 ex2

lib/csim.a:
	(cd lib && $(MAKE))

ex1: ex1.c lib/csim.a
	$(CC) -Ilib $(LDFLAGS) $(CFLAGS) $(CPPFLAGS) -o ex1 ex1.c lib/csim.a -lm

ex2: ex2.c lib/csim.a
	$(CC) -Ilib $(LDFLAGS) $(CFLAGS) $(CPPFLAGS) -o ex2 ex2.c lib/csim.a -lm

clean:
	rm -f ex1 ex2
