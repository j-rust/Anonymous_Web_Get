#
#	Makefile for Project 2
#	Jonathan Rust and Zach Osman
#	CS457
#

CC = /usr/bin/gcc

all:	ss awget

ss:	ss.c ss.h
	$(CC) -g -o ss ss.c

target:
	tar cf project2.tar ss.c ss.h Makefile README.md

clean:
	rm ss awget project2.tar *.o
