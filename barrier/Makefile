# Makefile
CC = gcc
CFLAGS = -Wall -pthread
LDFLAGS = -pthread

all: jones_family benchmark

jones_family: jones_family.o barrier.o
	$(CC) $(LDFLAGS) -o jones_family jones_family.o barrier.o

benchmark: benchmark.o barrier.o
	$(CC) $(LDFLAGS) -o benchmark benchmark.o barrier.o

jones_family.o: jones_family.c barrier.h
	$(CC) $(CFLAGS) -c jones_family.c

benchmark.o: benchmark.c barrier.h
	$(CC) $(CFLAGS) -c benchmark.c

barrier.o: barrier.c barrier.h
	$(CC) $(CFLAGS) -c barrier.c

clean:
	rm -f *.o jones_family benchmark