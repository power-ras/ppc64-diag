#
# Makefile for common utilities test scripts.
#

CC	= gcc
CFLAGS	= -g -Wall
INCLUDE = -I../

PROGS	= spopen

all : $(PROGS)

spopen : spopen.o ../utils.o
	$(CC) $(CFLAGS) $(INCLUDE) spopen.o ../utils.o -o spopen

%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE) -c -o $@ $<

clean:
	rm -f *.o $(PROGS)
