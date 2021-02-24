
# Makefile for the Malloc Lab
#
CC = gcc
CFLAGS = -Wall -g -std=gnu99

OBJS = mdriver.o mm.o memlib.o fsecs.o fcyc.o clock.o ftimer.o

all: clean mdriver

mdriver: CFLAGS += -O3
mdriver: $(OBJS) 
	$(CC) $(CFLAGS) -o mdriver $(OBJS)

mdriver.o: mdriver.c fsecs.h fcyc.h clock.h memlib.h config.h mm.h
memlib.o: memlib.c memlib.h
mm.o: mm.c mm.h memlib.h
fsecs.o: fsecs.c fsecs.h config.h
fcyc.o: fcyc.c fcyc.h
ftimer.o: ftimer.c ftimer.h config.h
clock.o: clock.c clock.h

debug: clean $(OBJS)
	$(CC) $(CFLAGS) -o mdriver $(OBJS)

handin:
	@USER=whoami
	python3 submission-client.py $(USER)

clean:
	rm -f *~ *.o mdriver


