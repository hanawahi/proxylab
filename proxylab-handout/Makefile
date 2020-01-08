# Makefile for Proxy Lab 
#
CC = gcc
CFLAGS = -Og -g -Wall -Wuninitialized

all: proxycache proxy tiny

csapp.o: csapp.c csapp.h
	$(CC) $(CFLAGS) -c csapp.c

proxy.o: proxy.c csapp.h cache.h
	$(CC) $(CFLAGS) -c proxy.c

cache.o: cache.c csapp.h cache.h
	$(CC) $(CFLAGS) -c cache.c

proxycache: proxy.o csapp.o cache.o
	$(CC) $(CFLAGS) proxy.o csapp.o cache.o -o proxycache

proxy: proxy.o csapp.o libcache.a
	$(CC) $(CFLAGS) proxy.o csapp.o -o proxy -L. -lcache

main.o: main.c cache.h

cache: cache.o main.o csapp.o 
	$(CC) $(CFLAGS) -o $@ $^

.PHONY:testcache
testcache: cache
	valgrind --leak-check=full ./cache

.PHONY:tiny
tiny:
	(cd tiny; make clean; make)
	(cd tiny/cgi-bin; make clean; make)

clean:
	rm -f *~ *.o proxy core 
	(cd tiny; make clean)
	(cd tiny/cgi-bin; make clean)
